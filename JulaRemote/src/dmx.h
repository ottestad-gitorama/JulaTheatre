#ifndef DMX_C6_H
#define DMX_C6_H

#include <stdbool.h>
#include <stdint.h>

/* Override using build_flags, e.g. -DDMX_RX_GPIO=20. UART1 is reserved here. */
#ifndef DMX_RX_GPIO
#define DMX_RX_GPIO 20
#endif
#define DMX_CHANNEL_COUNT 512

#ifdef __cplusplus
extern "C" {
#endif

/* Channel 1 = index 0. Written only by dmxUpdate(), never by the ISR. */
extern uint8_t dmx_universe[DMX_CHANNEL_COUNT];

typedef struct {
    uint32_t frames_received;
    uint32_t frames_superseded; /* New frame replaced one not yet read. */
    uint32_t breaks_received;
    uint32_t framing_errors;   /* Includes framing errors caused by breaks. */
    uint32_t fifo_overflows;
    uint32_t parity_errors;
    uint32_t boundary_drops;   /* FIFO not empty when break serviced. */
    uint32_t rejected_frames;
    uint16_t slots_received;   /* Most recent accepted frame, 1..512. */
    uint32_t age_ms;           /* UINT32_MAX until first accepted frame. */
} dmx_status_t;

/* Call once from a task. Uses ESP_ERROR_CHECK on initialization failure. */
void dmxSetup(void);

/* Nonblocking. Publishes latest complete frame; true if array updated.
 * Frame completes at the NEXT break. Short frames retain higher channels.
 * Call this from one task; that task also owns reads of dmx_universe.
 */
bool dmxUpdate(void);

/* Thread-safe diagnostics; the counter fields wrap naturally. */
void dmxGetStatus(dmx_status_t *status);

#ifdef __cplusplus
}
#endif
#endif
