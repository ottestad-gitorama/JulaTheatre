// VenstopLight
#include "radio.h"
#include "fixture.h"
#include "utils.h"
#include "terminal.h"
#include "storage.h"

void doCommand(uint16_t command, uint16_t parameter){
    switch(command){
      case MSG_IDENTIFY:
        printf("rx: Identify\n");
        fixtureIdentify();
      break;
      case MSG_GET_CONFIG:
        printf("rx: Get config\n");
        sendConfig();
      break;
      case MSG_GET_STATUS:
        printf("rx: Get status\n");
        sendReply((uint16_t)(1000.0*batteryVoltage));
      break;
      break;
      case MSG_SET_DMX_ADDRESS:
        printf("rx: Set dmx address\n");
        fixtureSetDmxAddress(parameter);
      break;
      case MSG_SET_FIXTURE_CHANNELS:
        printf("rx: set fixture channels\n");
        fixtureSetChannelCount(parameter);
      break;
      case MSG_SET_FIXTURE_PERSONALITY:
        printf("tx: set fixture personality\n");
        fixtureSetPersonality(parameter);
      break;
    }
}

void app_main() {
  terminalSetup();
  initNVS();
  fixtureInit();
  radioSetup();
  print_mac_address();

  while (false){
      // setLedChannel(PWM_LED_RED, 63);
      // delay(500);
      // setLedChannel(PWM_LED_RED, 127);
      // delay(500);
      // setLedChannel(PWM_LED_RED, 128+63);
      // delay(500);
      // setLedChannel(PWM_LED_RED, 255);
      // delay(500);
      for (int i=0; i<10; i++){
        setLedChannel(PWM_LED_RED, i*20);
        readBatteryVoltage();
        delay(1000);
        printf("Batt: %.4fV\n", batteryVoltage);
      }
      setLedChannel(PWM_LED_RED, 0);
      delay(1000);
      
    }
    printf("Channels: %i\t%i\t%i\t%i\n", channelValues[0],channelValues[1],channelValues[2],channelValues[3]);
    printf("Battery: %0.2fV\n", batteryVoltage);
    printf("Rssi: %i\ndB", rssi);
    printf("Packet loss%i\n", packet_loss);
    printf("Dmx address: %i\n", fixture_config.dmx_address);
    printf("Channel count: %i\n", fixture_config.channel_count);
    printf("Personality: %i\n", fixture_config.personality);
    printf("Starting main loop!\n");
    while(true){
      terminalUpdate();
      fixtureUpdate(); // TODO: Move this to radio rx?
      delay(10);
      // printf("Battery voltage = %imV\n", (int)(batteryVoltage*1000));
    }


}