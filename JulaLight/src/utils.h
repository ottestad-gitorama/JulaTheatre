#ifndef UTILS_H
#define UTILS_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

float fmap(float x, float in_min, float in_max, float out_min, float out_max);
void delay(int time);
uint32_t millis();
uint32_t micros();
#endif

