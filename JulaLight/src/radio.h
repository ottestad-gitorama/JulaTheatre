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
void changeWifiChannel(uint8_t wifiChannel);
bool isLegalWifiChannel(uint8_t wifiChannel);
extern bool doDiscoverBeaconing;
extern int16_t rssi;
extern uint16_t packet_loss;
