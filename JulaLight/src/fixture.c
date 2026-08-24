#include "fixture.h"

fixture_config_t fixture_config;
uint8_t channelValues[4];

ledc_channel_config_t ledc_channel[PWM_CHANNEL_MAX_COUNT] = {
    {
        .gpio_num   = PIN_LED_RED,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_LED_RED,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    },
    {
        .gpio_num   = PIN_LED_GREEN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_LED_GREEN,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    },
    {
        .gpio_num   = PIN_LED_BLUE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_LED_BLUE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    },
    {
        .gpio_num   = PIN_LED_WHITE,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_LED_WHITE,
        .timer_sel  = LEDC_TIMER_0,
        .duty       = 0,
        .hpoint     = 0,
    },
};

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
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
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
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
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
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGI(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}


void setFixtureDefaults(){
    fixture_config.channel_count = 4; // Default
    fixture_config.dmx_address = 1; // Default
    fixture_config.personality = 0; // Default
    fixture_config.gamma[0] =2.0;
    fixture_config.gamma[1] =2.0;
    fixture_config.gamma[2] =2.0;
    fixture_config.gamma[3] =2.0;
}

void setFixtureConfig(config_enum parameter, uint16_t value){
  switch(parameter){
    case CFG_ADDRESS:
        if (value >= DMX_UNIVERSE_SIZE) {printf("Illegal address: %i\n", value); return;}
        fixture_config.dmx_address = value;
        saveSettings();
        break;
    case CFG_CHANNEL_COUNT:
        if ((value!=1) && (value!=3) && (value!=4)) {printf("Illegal ch count: %i\n", value); return;}
        fixture_config.channel_count = value;
        saveSettings();
        break;
    case CFG_PERSONALITY:
        if (value!=0) {printf("Illegal personality: %i\n", value); return;}
        fixture_config.personality = value;
        saveSettings();
        break;
    case CFG_GAMMA_0:
        // I'll skip guard for gamme correction. Knock yourselves out!
        fixture_config.gamma[0] = value/1000.0;
        break;
    case CFG_GAMMA_1:
        fixture_config.gamma[1] = value/1000.0;
        break;
    case CFG_GAMMA_2:
        fixture_config.gamma[2] = value/1000.0;
        break;
    case CFG_GAMMA_3:
        fixture_config.gamma[3] = value/1000.0;
        break;
  }
}

uint16_t getFixtureConfig(config_enum parameter){
  switch(parameter){
    case CFG_ADDRESS:
        return fixture_config.dmx_address;
        break;
    case CFG_CHANNEL_COUNT:
        return (uint16_t) fixture_config.channel_count;
        break;
    case CFG_PERSONALITY:
        return (uint16_t) fixture_config.personality;
        break;
    case CFG_GAMMA_0:
        return (uint16_t) (fixture_config.gamma[0]*1000.0);
        break;
    case CFG_GAMMA_1:
        return (uint16_t) (fixture_config.gamma[1]*1000.0);
        break;
    case CFG_GAMMA_2:
        return (uint16_t) (fixture_config.gamma[2]*1000.0);
        break;
    case CFG_GAMMA_3:
        return (uint16_t) (fixture_config.gamma[3]*1000.0);
        break;
  }
  printf("Parameter type not found!\n");
  return 0;
}


void fixtureInit(){
    // Fixture init
    loadSettings();

    // DMX init
    for (int i=0; i < DMX_UNIVERSE_SIZE; i++){
        dmx_universe[i] = 0;
    }

    // PWM init
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,           // timer mode
        .timer_num = LEDC_TIMER_0,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);

    for (int ch = 0; ch < PWM_CHANNEL_MAX_COUNT; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    ledc_fade_func_install(0);
    setRGBW(0, 0, 0, 0);

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
    adc_oneshot_config_channel(adc_handle, BATTERY_VOLTAGE_CHANNEL, &channel_config);
    adc_calibration_init(ADC_UNIT_1, BATTERY_VOLTAGE_CHANNEL, ADC_ATTEN_DB_11, &adc1_cali_chan0_handle);    // adc1_config_width(ADC_WIDTH_BIT_12);      

}

#define NORMALIZE_FACTOR_255 (1.0 / 255.0)
#define MAX_VAL_AT_12BIT 4095
#define MAX_VAL_AT_13BIT 2*4095
#define MAX_VAL_AT_14BIT 4*4095
void setLedChannel(ledc_channel_t pwm_channel, uint8_t value, float gamma){
    // Set selected led output to value range 0-255, and apply gamma
    // Normalize
    float l = value * NORMALIZE_FACTOR_255;
    // Gamma correction
    l = pow(l, gamma);
    // Set pwm
    uint16_t out = (uint16_t)(l*MAX_VAL_AT_13BIT);
    // printf("led ch %i to val %i => %f => %i\n", pwm_channel, value, l, out);
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, pwm_channel, out, 10);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, pwm_channel, LEDC_FADE_NO_WAIT);

}

void setW(uint8_t w){
    channelValues[0] = 0;
    channelValues[1] = 0;
    channelValues[2] = 0;
    channelValues[3] = w;
    setLedChannel(PWM_LED_WHITE, w, fixture_config.gamma[0]);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b){
    channelValues[0] = r;
    channelValues[1] = g;
    channelValues[2] = b;
    channelValues[3] = 0;
    setLedChannel(PWM_LED_RED, r, fixture_config.gamma[0]);
    setLedChannel(PWM_LED_GREEN, g, fixture_config.gamma[1]);
    setLedChannel(PWM_LED_BLUE, b, fixture_config.gamma[2]);
}

void setRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w){
    channelValues[0] = r;
    channelValues[1] = g;
    channelValues[2] = b;
    channelValues[3] = w;
    setLedChannel(PWM_LED_RED, r, fixture_config.gamma[0]);
    setLedChannel(PWM_LED_GREEN, g, fixture_config.gamma[1]);
    setLedChannel(PWM_LED_BLUE, b, fixture_config.gamma[2]);
    setLedChannel(PWM_LED_WHITE, w, fixture_config.gamma[3]);
}

void fixtureIdentify(){
    // Identify fixture visually. Will block main loop during.
    for (int i=0; i<4; i++){
        setRGBW(255, 255, 255, 255);
        delay(50);
        setRGBW(0, 0, 0, 0);
        delay(50);
    }
}

void fixtureSetDmxAddress(uint16_t address){
    fixture_config.dmx_address = address;
    saveSettings();
}

void fixtureSetChannelCount(uint16_t channelCount){
    fixture_config.channel_count = channelCount;
    saveSettings();
}

void fixtureSetPersonality(uint16_t personality){
    fixture_config.personality = personality;
    saveSettings();
}

void fixtureSetGamma(uint8_t ch, float gamma){
    fixture_config.gamma[ch] = gamma;
    saveSettings();
}

void fixtureUpdate(){
    // Updates fixture outputs according to dmx universe
    switch(fixture_config.channel_count){
        case 1: // White
            setW(dmx_universe[fixture_config.dmx_address]);
        break;
        case 3: // RGB
            setRGB(dmx_universe[fixture_config.dmx_address], 
                   dmx_universe[fixture_config.dmx_address+1],
                   dmx_universe[fixture_config.dmx_address+2]
            );
        break;
        case 4: // RGBW
            setRGBW(dmx_universe[fixture_config.dmx_address], 
                   dmx_universe[fixture_config.dmx_address+1],
                   dmx_universe[fixture_config.dmx_address+2],
                   dmx_universe[fixture_config.dmx_address+3]
            );
        break;
    }
    readBatteryVoltage();
}

float batteryVoltage = 0;
float batteryVoltageMax = -1;
float batteryVoltageMin = 1000;
void readBatteryVoltage(){
    int battery_raw;
    adc_oneshot_read(adc_handle, BATTERY_VOLTAGE_CHANNEL, &battery_raw); 
    if (battery_raw != 0){
        float v = (float) battery_raw * BATTERY_VOLTAGE_FACTOR;
        // printf("Battery raw: %i\n", battery_raw);
        // printf("Battery volt: %0.2f\n", v);
        // delay(100);
        batteryVoltage += ((float) v - batteryVoltage)*BATTERY_FILTER_FACTOR;
    }
}

