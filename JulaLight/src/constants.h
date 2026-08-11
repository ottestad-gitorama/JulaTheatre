#pragma once
#include <stdio.h>
#include "driver/ledc.h"

#define PWM_CHANNEL_MAX_COUNT 4
#define PIN_LED_RED GPIO_NUM_18 // PWM1 - nærmest PCB edge, +V er nærmest midten
#define PIN_LED_GREEN GPIO_NUM_5 // PWM2
#define PIN_LED_BLUE GPIO_NUM_6 // PWM3
#define PIN_LED_WHITE GPIO_NUM_7 // PWM4
#define BATTERY_VOLTAGE_CHANNEL ADC_CHANNEL_0 // TODO: Change
#define BATTERY_VOLTAGE_FACTOR  3.768/1100.0 // TODO: Change
#define BATTERY_FILTER_FACTOR 1.0
#define GAMMA_CORRECTION_FACTOR 2