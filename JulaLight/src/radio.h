#pragma once
#include "constants.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common.h"
#include "fixture.h"
#include "esp_random.h"

#define PROTOCOL_VERSION 100 // 1.00



void radioSetup();
void sendDiscoveryMessage();
void sendReply(uint16_t battery_mv);
void sendConfig();
void print_mac_address();
extern void doCommand(uint16_t command, uint16_t parameter);

