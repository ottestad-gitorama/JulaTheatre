#pragma once
#include <stdio.h>
#include "driver/ledc.h"
#include "constants.h"
#include <math.h>
#include "utils.h"
#include "storage.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "common.h"


#define PWM_LED_RED LEDC_CHANNEL_0
#define PWM_LED_GREEN LEDC_CHANNEL_1
#define PWM_LED_BLUE LEDC_CHANNEL_2
#define PWM_LED_WHITE LEDC_CHANNEL_3





void fixtureInit();
void setFixtureDefaults();
void setFixtureConfig(config_enum parameter, uint16_t value);
uint16_t getFixtureConfig(config_enum parameter);
void fixtureUpdate();
void setLedChannel(ledc_channel_t pwm_channel, uint8_t value, float gamma);
void setW(uint8_t w);
void setRGB(uint8_t r, uint8_t g, uint8_t b);
void setRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void readBatteryVoltage();

void fixtureIdentify();
void fixtureSetDmxAddress(uint16_t address);
void fixtureSetChannelCount(uint16_t channelCount);
void fixtureSetPersonality(uint16_t personality);
void fixtureSetGamma(uint8_t ch, float gamma);

extern float batteryVoltage;
extern fixture_config_t fixture_config;
extern uint8_t channelValues[4];
