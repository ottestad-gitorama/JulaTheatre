#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "common.h"

void terminalSetup();
void terminalUpdate();
void terminalParse(char *data);

extern fixture_config_t fixture_config;
extern float batteryVoltage;
extern int16_t rssi;
extern uint16_t packet_loss;
extern uint8_t channelValues[4];
