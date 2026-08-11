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
    printf("dmx [ch] [val]                   Set dmx value\n");
    printf("discover                      Send discover message\n");
    printf("channel [unit] [channel]      Set unit dmx channel\n");
    printf("count [unit] [count]          Set unit channel count\n");
    printf("pers [unit] [pers]            Set unit personality\n");
    printf("status [unit]                 Get status of unit\n");
    printf("config [unit]                Get config of unit\n");
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
        if (!strcmp(pieces[0], "discover")){
            // TODO Send a single discover reply
            parsed = true;
        }
        if (!strcmp(pieces[0], "status")){
            // TODO
            printf("Channels: %i\t%i\t%i\t%i\n", channelValues[0],channelValues[1],channelValues[2],channelValues[3]);
            printf("Battery: %0.2fV\n", batteryVoltage);
            printf("Rssi: %i\ndB", rssi);
            printf("Packet loss%i\n", packet_loss);
            parsed = true;
        }
        if (!strcmp(pieces[0], "config")){
            // TODO
            printf("Dmx address: %i\n", fixture_config.dmx_address);
            printf("Channel count: %i\n", fixture_config.channel_count);
            printf("Personality: %i\n", fixture_config.personality);
            parsed = true;
        }
        if (!strcmp(pieces[0], "ident")){
            // TODO
            parsed = true;
        }
    }

    if (pieceCount == 2){
        if (!strcmp(pieces[0], "channel")){
            int ch = atoi(pieces[2]);
            // TODO
            parsed = true;
        }
        if (!strcmp(pieces[0], "count")){
            int count = atoi(pieces[2]);
            // TODO
            parsed = true;
        }
        if (!strcmp(pieces[0], "pers")){
            int pers = atoi(pieces[2]);
            // TODO
            parsed = true;
        }
    }
    if (pieceCount == 3){
        if (!strcmp(pieces[0], "dmx")){
            int ch = atoi(pieces[1]);
            int val = atoi(pieces[2]);
            // TODO
            parsed = true;
        }

        if (!parsed)printHelp();
    }
}

