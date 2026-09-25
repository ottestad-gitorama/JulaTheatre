#pragma once
#include "constants.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "utils.h"


void ioSetup();
void ioUpdate();
void ledGreen();
void ledRed();
void ledOff();
extern float pots[4];

