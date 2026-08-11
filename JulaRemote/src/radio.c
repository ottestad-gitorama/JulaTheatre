#include "radio.h"
 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t peerList[MAX_PEERS][6];
uint16_t peerCount = 0;
int16_t rssi;
uint16_t packet_loss;
uint16_t sequence = 0;
bool peerSet = false;

uint16_t getCRC(const uint8_t *data, uint16_t len)
{
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

void addPeerList(uint8_t * _peerAddress){
  if (peerCount >= MAX_PEERS) return;
  if (esp_now_is_peer_exist(_peerAddress)){
    printf("Peer already exist\n");
    return;
  }
  memcpy(peerList[peerCount], _peerAddress, 6);
  peerCount++;
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
  printf("RX: %i\n", len);
  if (!checkCRC(incomingData, len)) return;
  if (len == sizeof(status_reply_t)){
    printf("Received status reply size\n");
    status_reply_t* status_reply = (status_reply_t*) incomingData;
    if (status_reply->protocol_version != PROTOCOL_VERSION) return; // Wrong protocol
    if (status_reply->message_type != MSG_STATUS_REPLY) return; // wrong message
    printf("Status reply:\n");
    printf("Battery: %imV\n", status_reply->battery_mv);
    printf("rssi: %idB\n", status_reply->rssi);
    printf("Packet loss: %i\n", status_reply->packet_loss);
    printf("Values:\t%i\t%i\t%i\t%i\n", status_reply->values[0], status_reply->values[1], status_reply->values[2], status_reply->values[3]);
  }
  if (len == sizeof(get_config_reply_t)){
    printf("Received config reply size\n");
    get_config_reply_t* config_reply = (get_config_reply_t*) incomingData;
    if (config_reply->protocol_version != PROTOCOL_VERSION) return; // Wrong protocol
    if (config_reply->message_type != MSG_GET_CONFIG_REPLY) return; // Wrong message
    fixture_config_t config = config_reply->config;
    printf("Config received:\n");
    printf("Dmx address: %i\n", config.dmx_address);    
    printf("Channel count: %i\n", config.channel_count);    
    printf("Personality:%i\n", config.personality);    
  } 

  if (len == sizeof(discover_t)){
    discover_t* discover = (discover_t*) incomingData;
    if (discover->protocol_version != PROTOCOL_VERSION) return; // Wrong protocol
    if (discover->message_type == MSG_DISCOVER_REPLY) {

    }    
    addPeerList(esp_now_info->src_addr);
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
    esp_now_peer_info_t broadcastPeer = {};
    memcpy(broadcastPeer.peer_addr, broadcastAddress, 6);
    broadcastPeer.channel = 0;
    broadcastPeer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&broadcastPeer));
}

void sendCommand(uint8_t peerNo, uint8_t message, uint16_t par){
  command_t command;
  command.protocol_version = PROTOCOL_VERSION;
  command.message_type = message;
  command.parameter = par;
  command.crc = getCRC((const uint8_t *) &command, sizeof(command_t)-2);
  esp_err_t err = esp_now_send(peerList[peerNo], (uint8_t *) &command, sizeof(command_t)); 
  if (err != ESP_OK) {
    printf("Esp error %i while sending\n", err);
  }
}

void sendDiscoverRequest(){
  discover_t discover;
  discover.protocol_version = PROTOCOL_VERSION;
  discover.message_type = MSG_DISCOVER;
  discover.crc = getCRC((const uint8_t *) &discover, sizeof(discover_t)-2);

  esp_err_t err = esp_now_send(broadcastAddress, (uint8_t *) &discover, sizeof(discover_t)); 
  if (err != ESP_OK) {
    printf("Esp error %i while sending\n", err);
  }
}

void sendLightFrame(){
  light_frame_t lightFrame;
  lightFrame.protocol_version = PROTOCOL_VERSION;
  lightFrame.message_type = MSG_LIGHT_FRAME;
  lightFrame.sequence = sequence++;
  memcpy(lightFrame.data, dmx_universe, DMX_UNIVERSE_SIZE);
  lightFrame.crc = getCRC((const uint8_t *) &lightFrame, sizeof(light_frame_t)-2);

  esp_err_t err = esp_now_send(broadcastAddress, (uint8_t *) &lightFrame, sizeof(light_frame_t)); 
  if (err != ESP_OK) {
    printf("Esp error %i while sending\n", err);
  }
}

