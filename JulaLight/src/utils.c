#include "utils.h"

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void delay(int time){
    vTaskDelay(pdMS_TO_TICKS(time)); // Delay 
}

uint32_t millis(){
  return esp_timer_get_time() / 1000;
}

uint32_t micros(){
  return esp_timer_get_time();
}