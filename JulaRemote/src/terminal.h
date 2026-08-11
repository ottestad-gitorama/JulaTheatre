#include "driver/uart.h"
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "radio.h"

void terminalSetup();
void terminalUpdate();
void terminalParse(char *data);
extern uint8_t peerList[MAX_PEERS][6];
extern uint16_t peerCount;