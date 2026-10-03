#pragma once
#include "driver/gpio.h"

#define MAX_PEERS 100

#define VERSION 100 // 1.00

#define SWITCH_1A   GPIO_NUM_10
#define SWITCH_1B   GPIO_NUM_11
#define SWITCH_2A   GPIO_NUM_6
#define SWITCH_2B   GPIO_NUM_7
#define POT1        ADC_CHANNEL_3
#define POT2        ADC_CHANNEL_0
#define POT3        ADC_CHANNEL_1
#define POT4        ADC_CHANNEL_2
#define LED_GREEN   GPIO_NUM_19
#define LED_RED     GPIO_NUM_18

#define DMX_IN  GPIO_NUM_20

// Layout:
//       A       A
//      SW1 LED SW2     POT1
//       B       B
//
//      POT2    POT3    POT4
