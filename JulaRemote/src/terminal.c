#include "terminal.h"
#include "common.h"

void terminalSetup(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_0, &uart_config);
}



#define BUF_SIZE 255
char terminalData[BUF_SIZE];
int terminalLength = 0;
void terminalUpdate(){
    int len = uart_read_bytes(UART_NUM_0, terminalData + terminalLength, BUF_SIZE - terminalLength, pdMS_TO_TICKS(0));
    if (len > 0) {
        terminalLength += len;
        if (terminalData[terminalLength-1] == '\n') {
            // Null-terminate the string
            terminalData[terminalLength] = '\0';
            terminalParse(terminalData);
            terminalLength = 0;
        }
    }
}


void printHelp() {
    printf("\n");
    printf("Wrong input. These are legal:  \n");
    printf("dmx                             List dmx values\n");
    printf("dmx [ch] [val]                   Set dmx value\n");
    printf("discover                      \n");
    printf("list                          list discovered units\n");
    printf("adr [unit] [channel]     Set unit dmx address\n");
    printf("count [unit] [count]         Set unit channel count\n");
    printf("pers [unit] [pers]           Set unit personality\n");
    printf("status [unit]                 Get status of unit\n");
    printf("config [unit]                Get config of unit\n");
    printf("ident [unit]                  Identify unit\n");
}



void removeTrailingNewline(char *str) {
    for (int i=0; i<2; i++){ //do twice since there might be both \r and \n
        int len = strlen(str);
        if ((len > 0 && str[len - 1] == '\n') || (len > 0 && str[len - 1] == '\r')) {
            str[len - 1] = '\0'; // Replace the newline or return with a null terminator
        }
    }
}

//  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄  ▄▄▄▄▄▄▄▄▄▄▄ 
// ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
// ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀▀▀ 
// ▐░▌       ▐░▌▐░▌       ▐░▌▐░▌       ▐░▌▐░▌          ▐░▌          
// ▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ ▐░█▄▄▄▄▄▄▄▄▄ 
// ▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
// ▐░█▀▀▀▀▀▀▀▀▀ ▐░█▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀█░█▀▀  ▀▀▀▀▀▀▀▀▀█░▌▐░█▀▀▀▀▀▀▀▀▀ 
// ▐░▌          ▐░▌       ▐░▌▐░▌     ▐░▌            ▐░▌▐░▌          
// ▐░▌          ▐░▌       ▐░▌▐░▌      ▐░▌  ▄▄▄▄▄▄▄▄▄█░▌▐░█▄▄▄▄▄▄▄▄▄ 
// ▐░▌          ▐░▌       ▐░▌▐░▌       ▐░▌▐░░░░░░░░░░░▌▐░░░░░░░░░░░▌
//  ▀            ▀         ▀  ▀         ▀  ▀▀▀▀▀▀▀▀▀▀▀  ▀▀▀▀▀▀▀▀▀▀▀ 
#define MAX_PIECES 3
void terminalParse(char *str){
    // printf("Parsing: %s\n", str); //(char*)
    char *pieces[MAX_PIECES];
    int pieceCount = 0;

    // Use strtok to split the string by space
    char *piece = strtok(str, " ");
    while (piece != NULL && pieceCount < MAX_PIECES) {
        removeTrailingNewline(piece);        
        pieces[pieceCount++] = piece;
        // Continue to tokenize the string
        piece = strtok(NULL, " ");
    }


    // Now for the actuall parsing
    bool parsed = false;
    if (pieceCount == 1){
        if (!strcmp(pieces[0], "dmx")){
            for (int i=0; i<DMX_UNIVERSE_SIZE; i++){
                printf("%i\t", dmx_universe[i]);
                if (i%16==15) printf("\n");
            }
            printf("\n");
            parsed = true;
        }
        if (!strcmp(pieces[0], "discover")){
            sendDiscoverRequest();
            parsed = true;
        }
        if (!strcmp(pieces[0], "list")){
            // List peers
            printf("Discovered peers: \n");
            for (int i=0; i<peerCount; i++){
                printf("%i:\t", i);
                printf("%02x:%02x:%02x:%02x:%02x:%02x\n", 
                 peerList[i][0], peerList[i][1], peerList[i][2], peerList[i][3], peerList[i][4], peerList[i][5]);
            }
            // TODO
            parsed = true;
        }
    }

    if (pieceCount == 2){
        if (!strcmp(pieces[0], "status")){
            // Request status from peer. Result will be handled by rx callback
            int peerNo = atoi(pieces[1]);
            sendCommand(peerNo, MSG_GET_STATUS, 0);
            parsed = true;
        }
        if (!strcmp(pieces[0], "config")){
            // Request config from peer. Result will be handled by rx callback
            int peerNo = atoi(pieces[1]);
            sendCommand(peerNo, MSG_GET_CONFIG, 0);
            parsed = true;
        }
        if (!strcmp(pieces[0], "ident")){
            // Request identify blink from peer
            int peerNo = atoi(pieces[1]);
            printf("Request identity\n");
            sendCommand(peerNo, MSG_IDENTIFY, 0);
            parsed = true;
        }
    }
    if (pieceCount == 3){
        if (!strcmp(pieces[0], "dmx")){
            int ch = atoi(pieces[1]);
            int val = atoi(pieces[2]);
            dmx_universe[ch] = val;
            printf("Set dmx ch %i to %i\n", ch, val);
            parsed = true;
        }
        if (!strcmp(pieces[0], "adr")){
            uint8_t peerNo = atoi(pieces[1]);
            uint8_t adr = atoi(pieces[2]);
            printf("Set dmx adr on peer no %i to %i\n", peerNo, adr);
            sendCommand(peerNo, MSG_SET_DMX_ADDRESS, adr);
            parsed = true;
        }
        if (!strcmp(pieces[0], "count")){
            int peerNo = atoi(pieces[1]);
            int count = atoi(pieces[2]);
            printf("Set channel count on peer no %i to %i\n", peerNo, count);
            sendCommand(peerNo, MSG_SET_FIXTURE_CHANNELS, count);
            parsed = true;
        }
        if (!strcmp(pieces[0], "pers")){
            int peerNo = atoi(pieces[1]);
            int pers = atoi(pieces[2]);
            printf("Set personality on peer no %i to %i\n", peerNo, pers);
            sendCommand(peerNo, MSG_SET_FIXTURE_PERSONALITY, pers);
            parsed = true;
        }

    }
    if (!parsed)printHelp();
}

