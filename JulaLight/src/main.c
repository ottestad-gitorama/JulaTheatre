// VenstopLight
#include "radio.h"
#include "fixture.h"
#include "utils.h"
#include "terminal.h"
#include "storage.h"


void app_main() {
  terminalSetup();
  initNVS();
  fixtureInit();
  radioSetup();
  print_mac_address();

    printf("Channels: %i\t%i\t%i\t%i\n", channelValues[0],channelValues[1],channelValues[2],channelValues[3]);
    printf("Battery: %0.2fV\n", batteryVoltage);
    printf("Rssi: %i\ndB", rssi);
    printf("Packet loss%i\n", packet_loss);
    printf("Dmx address: %i\n", fixture_config.dmx_address);
    printf("Channel count: %i\n", fixture_config.channel_count);
    printf("Personality: %i\n", fixture_config.personality);
    printf("Starting main loop!\n");
    while(true){
      if (doDiscoverBeaconing){
        for (int i=0; i<3; i++){ // several sends to compensate for cross talk
          delay(esp_random()%200+10); // Random delay to avoid cross talk
          sendMessage(MSG_DISCOVER_REPLY, 0, 0);
        }
        doDiscoverBeaconing = false;
      }
      terminalUpdate();
      fixtureUpdate(); // TODO: Move this to radio rx?
      delay(10);
      // printf("Battery voltage = %imV\n", (int)(batteryVoltage*1000));
    }


}