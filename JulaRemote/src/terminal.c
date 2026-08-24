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

void enttecPacketReceived(uint8_t label, uint8_t *data, uint16_t length)
{
    // ENTTEC label 6 = Output Only Send DMX Packet
    if (label != 6) {
        return;
    }

    // Payload contains:
    // byte 0   = DMX start code
    // byte 1.. = DMX channels
    if (length < 1) {
        return;
    }

    // We only handle standard DMX data
    if (data[0] != 0x00) {
        return;
    }

    uint16_t channels = length - 1;

    if (channels > DMX_UNIVERSE_SIZE) {
        channels = DMX_UNIVERSE_SIZE;
    }

    memcpy(dmx_universe, &data[1], channels);

    sendLightFrame();
}

#define BUF_SIZE 255

#define ENTTEC_START 0x7E
#define ENTTEC_END   0xE7

char terminalData[BUF_SIZE];
int terminalLength = 0;

typedef enum {
    RX_IDLE,
    RX_TERMINAL,
    RX_ENTTEC_LABEL,
    RX_ENTTEC_LEN_LSB,
    RX_ENTTEC_LEN_MSB,
    RX_ENTTEC_PAYLOAD,
    RX_ENTTEC_END
} rx_state_t;

static rx_state_t rxState = RX_IDLE;

static uint8_t enttecLabel;
static uint16_t enttecLength;
static uint16_t enttecPos;

#define ENTTEC_MAX_PAYLOAD 600
static uint8_t enttecData[ENTTEC_MAX_PAYLOAD];


static void processByte(uint8_t b)
{
    switch (rxState) {

        case RX_IDLE:
            if (b == ENTTEC_START) {
                rxState = RX_ENTTEC_LABEL;
            } else {
                terminalLength = 0;
                rxState = RX_TERMINAL;
                processByte(b);
            }
            break;


        case RX_TERMINAL:
            if (b == '\n') {
                terminalData[terminalLength] = '\0';
                terminalParse(terminalData);

                terminalLength = 0;
                rxState = RX_IDLE;
            }
            else if (terminalLength < BUF_SIZE - 1) {
                terminalData[terminalLength++] = b;
            }
            else {
                // Terminal command too long - discard it
                terminalLength = 0;
                rxState = RX_IDLE;
            }
            break;


        case RX_ENTTEC_LABEL:
            enttecLabel = b;
            rxState = RX_ENTTEC_LEN_LSB;
            break;


        case RX_ENTTEC_LEN_LSB:
            enttecLength = b;
            rxState = RX_ENTTEC_LEN_MSB;
            break;


        case RX_ENTTEC_LEN_MSB:
            enttecLength |= ((uint16_t)b << 8);

            if (enttecLength > ENTTEC_MAX_PAYLOAD) {
                rxState = RX_IDLE;       // invalid packet
            } else {
                enttecPos = 0;
                rxState = enttecLength ?
                          RX_ENTTEC_PAYLOAD :
                          RX_ENTTEC_END;
            }
            break;


        case RX_ENTTEC_PAYLOAD:
            enttecData[enttecPos++] = b;

            if (enttecPos >= enttecLength) {
                rxState = RX_ENTTEC_END;
            }
            break;


        case RX_ENTTEC_END:
            if (b == ENTTEC_END) {
                enttecPacketReceived(
                    enttecLabel,
                    enttecData,
                    enttecLength
                );
            }

            rxState = RX_IDLE;
            break;
    }
}


void terminalUpdate()
{
    uint8_t buf[128];

    int len = uart_read_bytes(
        UART_NUM_0,
        buf,
        sizeof(buf),
        pdMS_TO_TICKS(0)
    );

    for (int i = 0; i < len; i++) {
        processByte(buf[i]);
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

void showConfigFromPeerList(int peerNo){
    printf("%02x:%02x:%02x:%02x:%02x:%02x\t", 
    peerList[peerNo][0], peerList[peerNo][1], peerList[peerNo][2], peerList[peerNo][3], peerList[peerNo][4], peerList[peerNo][5]);
    sendMessage(peerList[peerNo], MSG_CONFIG_REQUEST, CFG_ADDRESS, 0);
    if (waitForReply()){
        if (config_reply_parameter_no == CFG_ADDRESS){printf("adr:%i\t", config_reply_value);}
    } else {printf("adr: -\t");}
    sendMessage(peerList[peerNo], MSG_CONFIG_REQUEST, CFG_CHANNEL_COUNT, 0);
    if (waitForReply()){
        if (config_reply_parameter_no == CFG_CHANNEL_COUNT){printf("ch count:%i\t", config_reply_value);}
    } else {printf("ch count: -\t");}
    sendMessage(peerList[peerNo], MSG_CONFIG_REQUEST, CFG_PERSONALITY, 0);
    if (waitForReply()){
        if (config_reply_parameter_no == CFG_PERSONALITY){printf("pers:%i\t", config_reply_value);}
    } else {printf("pers: -\t");}

    sendMessage(peerList[peerNo], MSG_CONFIG_REQUEST, CFG_GAMMA_0, 0);
    if (waitForReply()){
        if (config_reply_parameter_no == CFG_GAMMA_0){printf("Gamma: %i, ", config_reply_value);}
    } else {printf("Gamma: -, ");}

    sendMessage(peerList[peerNo], MSG_CONFIG_REQUEST, CFG_GAMMA_1, 0);
    if (waitForReply()){
        if (config_reply_parameter_no == CFG_GAMMA_1){printf("%i, ", config_reply_value);}
    } else {printf("-, ");}
    sendMessage(peerList[peerNo], MSG_CONFIG_REQUEST, CFG_GAMMA_2, 0);
    if (waitForReply()){
        if (config_reply_parameter_no == CFG_GAMMA_2){printf("%i, ", config_reply_value);}
    } else {printf("-, ");}
    sendMessage(peerList[peerNo], MSG_CONFIG_REQUEST, CFG_GAMMA_3, 0);
    if (waitForReply()){
        if (config_reply_parameter_no == CFG_GAMMA_3){printf("%i\t", config_reply_value);}
    } else {printf("-\t");}
   

    printf("\n");
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
            sendMessage(broadcastAddress, MSG_DISCOVER, 0, 0);
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

    if (!strcmp(pieces[0], "status")){
        if (pieceCount == 1){
            // List status of all discovered devices
            printf("No\tFW\tBatt\trssi\tLoss\n");
            for (int i = 0; i<peerCount; i++){
                sendMessage(peerList[i], MSG_STATUS_REQUEST, 0, 0);
                if (waitForReply()){
                    printf("%i\t", i);
                    printf("%i\t", status_reply.firmware_version);
                    printf("%imV\t", status_reply.battery_mv);
                    printf("%idB\t", status_reply.rssi);
                    printf("%i\n", status_reply.packet_loss);                    
                }
            }
            parsed = true;
        }
        if (pieceCount == 2){
            // Request status from peer. Result will be handled by rx callback
            printf("Requesting status\n");
            int peerNo = atoi(pieces[1]);
            sendMessage(peerList[peerNo], MSG_STATUS_REQUEST, 0, 0);
            if (waitForReply()){
                printf("Status reply:\n");
                printf("Firmware: %i\n", status_reply.firmware_version);
                printf("Battery: %imV\n", status_reply.battery_mv);
                printf("rssi: %idB\n", status_reply.rssi);
                printf("Packet loss: %i\n", status_reply.packet_loss);
                printf("Values:\t%i\t%i\t%i\t%i\n", status_reply.values[0], status_reply.values[1], status_reply.values[2], status_reply.values[3]);
            }
            parsed = true;
        }
    }
    if (!strcmp(pieces[0], "config")){
        // Request config from peer. Result will be handled by rx callback
        if (pieceCount == 1){
            // List status of all discovered devices
            for (int i = 0; i<peerCount; i++){
                showConfigFromPeerList(i);
            }
            parsed = true;
        }
        if (pieceCount == 2){
            int peerNo = atoi(pieces[1]);
            showConfigFromPeerList(peerNo);
            parsed = true;
        }
    }
    if (pieceCount == 2){
        if (!strcmp(pieces[0], "ident")){
            // Request identify blink from peer
            printf("Request identity\n");
            int peerNo = atoi(pieces[1]);
            sendMessage(peerList[peerNo], MSG_IDENTIFY, 0, 0);
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
            sendMessage(peerList[peerNo], MSG_CONFIG_SET, CFG_ADDRESS, adr);
            parsed = true;
        }
        if (!strcmp(pieces[0], "count")){
            int peerNo = atoi(pieces[1]);
            int count = atoi(pieces[2]);
            printf("Set channel count on peer no %i to %i\n", peerNo, count);
            sendMessage(peerList[peerNo], MSG_CONFIG_SET, CFG_CHANNEL_COUNT, count);
            parsed = true;
        }
        if (!strcmp(pieces[0], "pers")){
            int peerNo = atoi(pieces[1]);
            int pers = atoi(pieces[2]);
            printf("Set personality on peer no %i to %i\n", peerNo, pers);
            sendMessage(peerList[peerNo], MSG_CONFIG_SET, CFG_PERSONALITY, pers);
            parsed = true;
        }
        if (!strcmp(pieces[0], "gamma0")){
            int peerNo = atoi(pieces[1]);
            int gamma = atoi(pieces[2]);
            printf("Set gamma ch 0 on peer no %i to %i\n", peerNo, gamma);
            sendMessage(peerList[peerNo], MSG_CONFIG_SET, CFG_GAMMA_0, gamma);
            parsed = true;
        }
        if (!strcmp(pieces[0], "gamma1")){
            int peerNo = atoi(pieces[1]);
            int gamma = atoi(pieces[2]);
            printf("Set gamma ch 1 on peer no %i to %i\n", peerNo, gamma);
            sendMessage(peerList[peerNo], MSG_CONFIG_SET, CFG_GAMMA_1, gamma);
            parsed = true;
        }
        if (!strcmp(pieces[0], "gamma2")){
            int peerNo = atoi(pieces[1]);
            int gamma = atoi(pieces[2]);
            printf("Set gamma ch 2 on peer no %i to %i\n", peerNo, gamma);
            sendMessage(peerList[peerNo], MSG_CONFIG_SET, CFG_GAMMA_2, gamma);
            parsed = true;
        }
        if (!strcmp(pieces[0], "gamma3")){
            int peerNo = atoi(pieces[1]);
            int gamma = atoi(pieces[2]);
            printf("Set gamma ch 3 on peer no %i to %i\n", peerNo, gamma);
            sendMessage(peerList[peerNo], MSG_CONFIG_SET, CFG_GAMMA_3, gamma);
            parsed = true;
        }

    }
    if (!parsed)printHelp();
}

