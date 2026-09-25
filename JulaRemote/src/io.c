#include "io.h"

float pots[4];

adc_oneshot_unit_handle_t adc_handle;
adc_cali_handle_t adc1_cali_chan0_handle = NULL;

#define TAG "ADC_CALI"
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        printf("calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        printf(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        printf("Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        printf("eFuse not burnt, skip software calibration");
    } else {
        printf("Invalid arg or no memory");
    }

    return calibrated;
}


void ioSetup(){
    // Digital output
    gpio_reset_pin(LED_GREEN);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LED_RED);
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);

    // Digital input
    gpio_set_direction(SWITCH_1A, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_1A, GPIO_PULLUP_ONLY);
    gpio_set_direction(SWITCH_1B, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_1B, GPIO_PULLUP_ONLY);
    gpio_set_direction(SWITCH_2A, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_2A, GPIO_PULLUP_ONLY);
    gpio_set_direction(SWITCH_2B, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_2B, GPIO_PULLUP_ONLY);



   // ADC Setup

    adc_oneshot_unit_init_cfg_t adc_unit_config = {
        .unit_id = ADC_UNIT_1,
    };   
    adc_oneshot_new_unit(&adc_unit_config, &adc_handle);
    // Battery analog setup
    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,
    };
    adc_oneshot_config_channel(adc_handle, POT1, &channel_config);
    adc_calibration_init(ADC_UNIT_1, POT1, ADC_ATTEN_DB_11, &adc1_cali_chan0_handle);   
    adc_oneshot_config_channel(adc_handle, POT2, &channel_config);
    adc_calibration_init(ADC_UNIT_1, POT2, ADC_ATTEN_DB_11, &adc1_cali_chan0_handle);   
    adc_oneshot_config_channel(adc_handle, POT3, &channel_config);
    adc_calibration_init(ADC_UNIT_1, POT3, ADC_ATTEN_DB_11, &adc1_cali_chan0_handle);   
    adc_oneshot_config_channel(adc_handle, POT4, &channel_config);
    adc_calibration_init(ADC_UNIT_1, POT4, ADC_ATTEN_DB_11, &adc1_cali_chan0_handle);   

}

void ioUpdate(){
    int pot1;
    adc_oneshot_read(adc_handle, POT1, &pot1); 
    int pot2;
    adc_oneshot_read(adc_handle, POT2, &pot2); 
    int pot3;
    adc_oneshot_read(adc_handle, POT3, &pot3); 
    int pot4;
    adc_oneshot_read(adc_handle, POT4, &pot4); 
    pots[0] = (float)pot1/3316;
    pots[1] = (float)pot2/3316;
    pots[2] = (float)pot3/3316;
    pots[3] = (float)pot4/3316;
    // printf("P1: %.2f\tP2: %.2f\tP3: %.2f\tP4: %.2f\n", pots[0], pots[1], pots[2], pots[3]);
    bool sw1a = !gpio_get_level(SWITCH_1A);
    bool sw1b = !gpio_get_level(SWITCH_1B);
    bool sw2a = !gpio_get_level(SWITCH_2A);
    bool sw2b = !gpio_get_level(SWITCH_2B);
    // printf("S1A: %i\tS1B: %i\tS2A: %i\tS2B: %i\n", sw1a, sw1b, sw2a, sw2b);

}

void ledGreen(){
    gpio_set_level(LED_RED, 0);
    gpio_set_level(LED_GREEN, 1);
}

void ledRed(){
    gpio_set_level(LED_RED, 1);
    gpio_set_level(LED_GREEN, 0);
}
void ledOff(){
    gpio_set_level(LED_RED, 0);
    gpio_set_level(LED_GREEN, 0);
}
