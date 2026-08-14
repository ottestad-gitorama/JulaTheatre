#include "radio.h"
#include "constants.h"
#include "terminal.h"
#include "common.h"
#include "storage.h"

void app_main() {
    initNVS();
    radioSetup();
    terminalSetup();

    while (false){
        printf("Discover request\n");
        sendLightFrame();
        delay(10);
        sendDiscoverRequest();
        delay(1000);
    }

    while (true){
       terminalUpdate();
       delay(10); 
       
       sendLightFrame();
    } 


}