#pragma once
#include "constants.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common.h"
#include "utils.h"


void radioSetup();
void sendLightFrame();
void print_mac_address();
// void sendCommand(uint8_t peerNo, uint8_t message, uint16_t par);
void sendMessage(const uint8_t *peer_address,message_enum msg_type, uint16_t parameter, uint16_t value);
// void sendDiscoverRequest();
bool waitForReply();