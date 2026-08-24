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




void radioSetup();
void sendMessage(message_enum, uint16_t parameter, uint16_t value);
void sendStatus();
void sendConfig();
void print_mac_address();

extern bool doDiscoverBeaconing;

