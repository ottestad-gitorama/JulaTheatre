#include "radio.h"
#include "constants.h"
#include "terminal.h"
#include "common.h"
#include "storage.h"

void app_main() {
    initNVS();
    radioSetup();
    terminalSetup();


    long testTime = millis();
    int testCount = 0;
    uint16_t lastTestLoss = 0;
    while (true){
       terminalUpdate();
       delay(10); 
    
       sendLightFrame();
       testCount++;
       if (millis()-testTime>1000){
        testTime = millis();
                sendMessage(peerList[0], MSG_STATUS_REQUEST, 0, 0);
                if (waitForReply()){
                    uint16_t l = status_reply.packet_loss-lastTestLoss;
                    lastTestLoss = status_reply.packet_loss;
                    printf("Sent: %i\tLoss: %i\tTotal: %i\n",testCount, l ,status_reply.packet_loss);                    

                }

        testCount = 0;
       }
    } 


}