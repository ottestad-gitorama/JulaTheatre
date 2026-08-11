#pragma once
#include "constants.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common.h"
#define PROTOCOL_VERSION 100 // 1.00


void radioSetup();
void sendLightFrame();
void print_mac_address();
void sendCommand(uint8_t peerNo, uint8_t message, uint16_t par);
void sendDiscoverRequest();