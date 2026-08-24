#pragma once
#include <stdio.h>
#include "driver/ledc.h"

#define FIRMWARE_VERSION 101    // Format 100=>1.01
#define PWM_CHANNEL_MAX_COUNT 4
#define PIN_LED_RED GPIO_NUM_18 // PWM1 - nærmest PCB edge, +V er nærmest midten
#define PIN_LED_GREEN GPIO_NUM_5 // PWM2
#define PIN_LED_BLUE GPIO_NUM_6 // PWM3
#define PIN_LED_WHITE GPIO_NUM_7 // PWM4
#define BATTERY_VOLTAGE_CHANNEL ADC_CHANNEL_2
#define BATTERY_VOLTAGE_FACTOR  1/812.5 // 4.015V=>3262 Divider 8k2 + 12k
#define BATTERY_FILTER_FACTOR 0.01
