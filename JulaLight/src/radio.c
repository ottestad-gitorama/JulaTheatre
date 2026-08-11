








#include "radio.h"
 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t peerAddress[] = {0,0,0,0,0,0};
int16_t rssi;
uint16_t packet_loss;
uint16_t sequence = 0;
bool peerSet = false;

uint16_t getCRC(const uint8_t *data, uint16_t len)
{

  // #define hvis if
  // #define returner return 

  // hvis(1==2) returner 0;


    uint16_t crc = 0x0000;

    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool checkCRC(const uint8_t *data, uint16_t len){
  // Returns true if checksum stored in the last two bytes matches the crc of the rest of the buffer
  uint16_t crc = getCRC(data, len-2);
//  uint16_t expected_crc = ((uint16_t)data[len - 2] << 8) | data[len - 1];
  uint16_t expected_crc = data[len - 2] | ((uint16_t)data[len - 1] << 8);
  if (crc == expected_crc) return true; 
  printf("CRC Failed!\n");
  return false; 
}

void print_mac_address() {
    uint8_t mac[6];
    // Get the Station interface MAC address.
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        printf("Local MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        printf("Failed to get MAC address\n");
    }
}



// Callback when data is sent
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_FAIL) {
    // I dont care.
  }
}


void setPeer(uint8_t * _peerAddress){
  // The first command or light frame received sets the master.
  peerSet = true;
  memcpy(peerAddress, _peerAddress, 6);
  // Add peer
  printf("Add peer\n");
  esp_now_peer_info_t peerInfo = {
      .channel = 0,
      .encrypt = false,
  };
  memcpy(peerInfo.peer_addr, _peerAddress, 6);
  ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
}


// Callback when data is received
void on_data_recv(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len) {
  if (!checkCRC(incomingData, len)) return;
  if (len == sizeof(light_frame_t)){
    light_frame_t* light_frame = (light_frame_t*) incomingData;
    if (!peerSet) setPeer(esp_now_info->src_addr);
    rssi = esp_now_info->rx_ctrl->rssi;
    if (light_frame->protocol_version != PROTOCOL_VERSION){printf("Wrong protocol\n"); return;} // Wrong protocol

    if (light_frame->message_type != MSG_LIGHT_FRAME) {printf("Wrong command\n"); return;} // Wrong command
    memcpy(dmx_universe, light_frame->data, DMX_UNIVERSE_SIZE);
  } 

  if (len == sizeof(command_t)){
    printf("Received command size\n");
    command_t* command = (command_t*) incomingData;
    if (command->protocol_version != PROTOCOL_VERSION) return; // Wrong protocol
    if (!peerSet) setPeer(esp_now_info->src_addr);
    rssi = esp_now_info->rx_ctrl->rssi;
    doCommand(command->message_type, command->parameter);
  } 
  
  if (len == sizeof(discover_t)){
    printf("Received discover size\n");
    discover_t* discover = (discover_t*) incomingData;
    if (discover->protocol_version != PROTOCOL_VERSION) return; // Wrong protocol
    if (discover->message_type != MSG_DISCOVER) return; // Wrong message
    printf("rx: Discovery\n");
    sendDiscoveryMessage();
  } 
}

void radioSetup() {
    // Initialize WiFi in Station mode
    printf("Initialize WiFi in Station mode\n");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM ); // TESTCODE
    ESP_ERROR_CHECK(esp_wifi_start());

    // Initialize ESP-NOW
    printf("Initi ESP-now\n");
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));
}


void sendReply(uint16_t battery_mv){
  status_reply_t reply;
  reply.protocol_version = PROTOCOL_VERSION;
  reply.message_type = MSG_STATUS_REPLY;
  reply.battery_mv = battery_mv;
  reply.rssi = rssi;
  reply.packet_loss = packet_loss;
  reply.values[0] = channelValues[0];
  reply.values[1] = channelValues[1];
  reply.values[2] = channelValues[2];
  reply.values[3] = channelValues[3];
  reply.crc = getCRC((const uint8_t *) &reply, sizeof(status_reply_t)-2);
  esp_err_t err = esp_now_send(peerAddress, (uint8_t *) &reply, sizeof(status_reply_t)); 
  if (!err == ESP_OK) {
    // I dont care
  }
}

void sendConfig(){
  get_config_reply_t reply;
  reply.protocol_version = PROTOCOL_VERSION;
  reply.message_type = MSG_GET_CONFIG_REPLY;
  reply.config.channel_count = fixture_config.channel_count;
  reply.config.dmx_address = fixture_config.dmx_address;
  reply.config.personality = fixture_config.personality;
  reply.crc = getCRC((const uint8_t *) &reply, sizeof(get_config_reply_t)-2);
  esp_err_t err = esp_now_send(peerAddress, (uint8_t *) &reply, sizeof(get_config_reply_t)); 
  if (!err == ESP_OK) {
    // I dont care
  }
}


void sendDiscoveryMessage(){
  delay(esp_random()%500); // Random delay to avoid crash.
  discover_t discover;
  discover.protocol_version = PROTOCOL_VERSION;
  discover.message_type = MSG_DISCOVER_REPLY;
  discover.crc = getCRC((const uint8_t *) &discover, sizeof(discover_t)-2);
  for (int i=0; i<3; i++){ // several sends to compensate for cross talk
    delay(esp_random()%200+10); // Random delay to avoid cross talk
    esp_err_t err = esp_now_send(peerAddress, (uint8_t *) &discover, sizeof(discover_t)); 
    if (!err == ESP_OK) {
      // I dont care
    }
  }
}
