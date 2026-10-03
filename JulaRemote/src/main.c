#include "radio.h"
#include "constants.h"
#include "terminal.h"
#include "common.h"
#include "storage.h"
#include "io.h"
#include "dmx.h"


void app_main() {
    initNVS();
    ioSetup();
    ledRed();
    radioSetup();
    terminalSetup();
    dmxSetup();

    long testTime = millis();
    int testCount = 0;
    uint16_t lastTestLoss = 0;
    ledGreen();
    while (true){
        ioUpdate();
        terminalUpdate();
        if(dmxUpdate()){
            printf("%i\t%i\t%i\t%i\t\n", dmx_universe[0], dmx_universe[1], dmx_universe[2], dmx_universe[3]);
            // dmx_status_t s;
            // dmxGetStatus(&s);
            // printf("frames=%" PRIu32 " breaks=%" PRIu32 " slots=%u age=%" PRIu32
            //        "ms boundary_drops=%" PRIu32 " overflow=%" PRIu32
            //        " CH1=%u CH2=%u CH512=%u\n",
            //        s.frames_received, s.breaks_received, s.slots_received,
            //        s.age_ms, s.boundary_drops, s.fifo_overflows,
            //        dmx_universe[0], dmx_universe[1], dmx_universe[511]);
        }
        delay(20); 
    //    if (sw1a){
       if (gpio_get_level(DMX_IN)){
        ledRed();
        dmx_universe[1] = (uint8_t)(255*pots[0]);
        dmx_universe[2] = (uint8_t)(255*pots[0]);
        dmx_universe[3] = (uint8_t)(255*pots[0]);
        dmx_universe[4] = (uint8_t)(255*pots[0]);
        dmx_universe[5] = (uint8_t)(255*pots[1]);
        dmx_universe[6] = (uint8_t)(255*pots[1]);
        dmx_universe[7] = (uint8_t)(255*pots[1]);
        dmx_universe[8] = (uint8_t)(255*pots[1]);
        dmx_universe[9] = (uint8_t)(255*pots[2]);
        dmx_universe[10] = (uint8_t)(255*pots[2]);
        dmx_universe[11] = (uint8_t)(255*pots[2]);
        dmx_universe[12] = (uint8_t)(255*pots[2]);
        dmx_universe[13] = (uint8_t)(255*pots[3]);
        dmx_universe[14] = (uint8_t)(255*pots[3]);
        dmx_universe[15] = (uint8_t)(255*pots[3]);
        dmx_universe[16] = (uint8_t)(255*pots[3]);
        dmx_universe[17] = (uint8_t)(255*pots[3]);
       } else {
          ledGreen();
       }
    
       sendLightFrame();
       testCount++;
       #ifdef TEST_RADIO_RANGE
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
       #endif
    } 


}