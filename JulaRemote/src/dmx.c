/* ESP32-C6 / ESP-IDF 5.5, receive-only DMX prototype.
 * Uses the IDF UART clock/pin setup and a custom, IRAM-safe receive ISR.
 * No esp_dmx dependency and no direct peripheral clock/reset manipulation.
 */
#include "dmx.h"
#include <stddef.h>
#include "sdkconfig.h"
#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "hal/uart_ll.h"
#include "soc/uart_periph.h"
#if CONFIG_PM_ENABLE
#include "esp_pm.h"
#endif

#if !CONFIG_IDF_TARGET_ESP32C6
#error "This receiver targets ESP32-C6; validate UART behavior before porting."
#endif
#if defined(CONFIG_ESP_CONSOLE_UART_NUM) && CONFIG_ESP_CONSOLE_UART_NUM == 1
#error "UART1 is configured as the console; select UART0 or USB for the console."
#endif

#define DMX_UART UART_NUM_1
#define RX_INTERRUPTS (UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT | \
                       UART_INTR_BRK_DET | UART_INTR_FRAM_ERR | \
                       UART_INTR_RXFIFO_OVF | UART_INTR_PARITY_ERR)

typedef enum { WAIT_BREAK, WAIT_START, RECEIVE_SLOTS, IGNORE_PACKET } rx_state_t;


uint8_t dmx_universe[DMX_CHANNEL_COUNT];
static DRAM_ATTR uint8_t working[DMX_CHANNEL_COUNT];
static DRAM_ATTR uint8_t pending[DMX_CHANNEL_COUNT];
static DRAM_ATTR rx_state_t rx_state = WAIT_BREAK;
static DRAM_ATTR uint16_t working_slots;
static DRAM_ATTR uint16_t pending_slots;
static DRAM_ATTR bool awaiting_break;
static DRAM_ATTR bool ready;
static DRAM_ATTR dmx_status_t counters;
static DRAM_ATTR int64_t last_frame_us;
static DRAM_ATTR portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static bool initialized;
static intr_handle_t interrupt_handle;
#if CONFIG_PM_ENABLE
static esp_pm_lock_handle_t sleep_lock;
#endif

/* The parser and mailbox are protected by mux in both task and ISR context. */
static void IRAM_ATTR consume_byte(uint8_t byte)
{
    if (awaiting_break) {
        /* Data after a framing error without a break means corruption. */
        if (rx_state == RECEIVE_SLOTS) counters.rejected_frames++;
        rx_state = WAIT_BREAK;
        working_slots = 0;
        return;
    }
    if (rx_state == WAIT_START) {
        rx_state = byte == 0 ? RECEIVE_SLOTS : IGNORE_PACKET;
    } else if (rx_state == RECEIVE_SLOTS) {
        if (working_slots < DMX_CHANNEL_COUNT) {
            working[working_slots++] = byte;
        } else {
            counters.rejected_frames++;
            rx_state = IGNORE_PACKET;
            working_slots = 0;
        }
    }
}

static void IRAM_ATTR accept_break(void)
{
    counters.breaks_received++;
    if (rx_state == RECEIVE_SLOTS && working_slots != 0) {
        if (ready) counters.frames_superseded++;
        for (unsigned i = 0; i < working_slots; ++i) pending[i] = working[i];
        pending_slots = working_slots;
        ready = true;
        counters.frames_received++;
        counters.slots_received = working_slots;
        last_frame_us = esp_timer_get_time();
    }
    working_slots = 0;
    awaiting_break = false;
    rx_state = WAIT_START;
}

static void IRAM_ATTR drain_fifo(uart_dev_t *hw, bool parse)
{
    /* Snapshot and bounded loop: ISR never waits for serial input. */
    unsigned count = uart_ll_get_rxfifo_len(hw);
    for (unsigned i = 0; i < count; ++i) {
        uint8_t byte;
        uart_ll_read_rxfifo(hw, &byte, 1);
        if (parse) consume_byte(byte);
    }
}

static void IRAM_ATTR rx_isr(void *arg)
{
    (void)arg;
    uart_dev_t *hw = UART_LL_GET_HW(DMX_UART);
    portENTER_CRITICAL_ISR(&mux);
    /* Limit work per interrupt even if more data arrive during servicing. */
    for (unsigned pass = 0; pass < 4; ++pass) {
        uint32_t status = uart_ll_get_intsts_mask(hw) & RX_INTERRUPTS;
        if (!status) break;

        if (status & UART_INTR_FRAM_ERR) counters.framing_errors++;
        if (status & UART_INTR_PARITY_ERR) counters.parity_errors++;
        if (status & UART_INTR_RXFIFO_OVF) counters.fifo_overflows++;

        if (status & (UART_INTR_RXFIFO_OVF | UART_INTR_PARITY_ERR)) {
            counters.rejected_frames++;
            rx_state = WAIT_BREAK;
            working_slots = 0;
            awaiting_break = false;
            drain_fifo(hw, false);
            if (status & UART_INTR_BRK_DET) counters.breaks_received++;
        } else if (status & UART_INTR_BRK_DET) {
            /* A break flag does not mark a position in the FIFO. If bytes
             * remain, their side of the break is unknown: discard safely.
             * Error-byte discard prevents the break's dummy zero entering
             * the FIFO. The framing-error ISR drains preceding valid bytes.
             */
            if (uart_ll_get_rxfifo_len(hw) != 0) {
                counters.breaks_received++;
                counters.boundary_drops++;
                counters.rejected_frames++;
                drain_fifo(hw, false);
                working_slots = 0;
                awaiting_break = false;
                rx_state = WAIT_BREAK;
            } else {
                accept_break();
            }
        } else {
            drain_fifo(hw, true);
            if (status & UART_INTR_FRAM_ERR) awaiting_break = true;
        }
        uart_ll_clr_intsts_mask(hw, status);
    }
    portEXIT_CRITICAL_ISR(&mux);
}

void dmxSetup(void)
{
    if (initialized) return;
    /* Never share a UART between IDF's driver ISR and this ISR. */
    ESP_ERROR_CHECK(uart_is_driver_installed(DMX_UART)
                    ? ESP_ERR_INVALID_STATE : ESP_OK);
    const uart_config_t config = {
        .baud_rate = 250000,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_2,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_XTAL,
    };
    ESP_ERROR_CHECK(uart_param_config(DMX_UART, &config));
    uart_dev_t *hw = UART_LL_GET_HW(DMX_UART);
    uart_ll_disable_intr_mask(hw, UINT32_MAX);
    uart_ll_clr_intsts_mask(hw, UINT32_MAX);
    /* Discard characters with bad stop bits, including a break's dummy 0.
     * This configuration is crucial: a payload value 0 remains valid data.
     */
    uart_ll_discard_error_data(hw, true);
    ESP_ERROR_CHECK(uart_set_pin(DMX_UART, UART_PIN_NO_CHANGE, DMX_RX_GPIO,
                                UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
#if CONFIG_PM_ENABLE
    ESP_ERROR_CHECK(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "dmx_rx", &sleep_lock));
    ESP_ERROR_CHECK(esp_pm_lock_acquire(sleep_lock));
#endif
    ESP_ERROR_CHECK(esp_intr_alloc(uart_periph_signal[DMX_UART].irq,
                   ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3,
                   rx_isr, NULL, &interrupt_handle));
    const uart_intr_config_t interrupts = {
        .intr_enable_mask = RX_INTERRUPTS,
        .rx_timeout_thresh = 2,
        .rxfifo_full_thresh = 1,
        .txfifo_empty_intr_thresh = 0,
    };
    ESP_ERROR_CHECK(uart_intr_config(DMX_UART, &interrupts));
    initialized = true;
}

bool dmxUpdate(void)
{
    bool updated = false;
    portENTER_CRITICAL(&mux);
    if (ready) {
        for (unsigned i = 0; i < pending_slots; ++i) dmx_universe[i] = pending[i];
        ready = false;
        updated = true;
    }
    portEXIT_CRITICAL(&mux);
    return updated;
}

void dmxGetStatus(dmx_status_t *status)
{
    if (!status) return;
    portENTER_CRITICAL(&mux);
    *status = counters;
    int64_t timestamp = last_frame_us;
    bool seen_frame = counters.slots_received != 0;
    portEXIT_CRITICAL(&mux);
    if (!seen_frame) {
        status->age_ms = UINT32_MAX;
    } else {
        uint64_t age = (uint64_t)(esp_timer_get_time() - timestamp) / 1000;
        status->age_ms = age >= UINT32_MAX ? UINT32_MAX - 1 : (uint32_t)age;
    }
}
