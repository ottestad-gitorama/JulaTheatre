#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "radio.h"

void terminalSetup();
void terminalUpdate();
void terminalParse(char *data);
extern uint8_t broadcastAddress[];
extern uint8_t peerList[MAX_PEERS][6];
extern uint16_t peerCount;
extern status_reply_t status_reply;
extern uint16_t config_reply_parameter_no;
extern uint16_t config_reply_value;