#include "radio.h"
 
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t peerAddress[] = {0,0,0,0,0,0};
int16_t rssi;
uint16_t packet_loss;
bool first_packet_loss_scan = true;
uint16_t sequence = 0;
bool peerSet = false;
bool doDiscoverBeaconing = false;

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
  if (len < 2) return false;
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

  // message_t and light_frame_t shares same header, so this works for initial checks
  const message_t* message =  (message_t*) incomingData;
  if (message->protocol_version != PROTOCOL_VERSION) {printf("Wrong protocol\n"); return;}
  message_enum msg_type = message->message_type;

  if (msg_type==MSG_LIGHT_FRAME){
    if (len != sizeof(light_frame_t)) {printf("Wrong light frame size\n"); return;}
  } else {
    if (len != sizeof(message_t)) {printf("Wrong message size\n"); return;}
  }

  if (!peerSet) setPeer(esp_now_info->src_addr); // First legal rx selects as master
  rssi = esp_now_info->rx_ctrl->rssi;


  switch (msg_type)
  {
    case MSG_LIGHT_FRAME:
      {
        const light_frame_t* light_frame = (const light_frame_t*) incomingData;
        memcpy(dmx_universe, light_frame->data, DMX_UNIVERSE_SIZE);

        if (first_packet_loss_scan) {
            first_packet_loss_scan = false;
            sequence = light_frame->sequence + 1;
        }
        else {
            if (light_frame->sequence != sequence) {
                packet_loss += (uint16_t)(light_frame->sequence - sequence);
            }
            sequence = light_frame->sequence + 1;
        }        
        
        break;

      }
    case MSG_CONFIG_SET:
      setFixtureConfig((config_enum) message->parameter, message->value);
      break;
    case MSG_CONFIG_REQUEST:
      sendMessage(MSG_CONFIG_REPLY, message->parameter, getFixtureConfig((config_enum) message->parameter));
      break;
    case MSG_DISCOVER:
      doDiscoverBeaconing = true;
      break;
    case MSG_IDENTIFY:
      fixtureIdentify();
      break;
    case MSG_STATUS_REQUEST:
      sendStatus();
      break;
    default:
      printf("Wrong message type\n");
      break;
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

void sendMessage(message_enum msg_type, uint16_t parameter, uint16_t value){
  message_t message;
  message.protocol_version = PROTOCOL_VERSION;
  message.message_type = msg_type;
  message.parameter = parameter;
  message.value = value;
  message.crc = getCRC((const uint8_t *) &message, sizeof(message_t)-2);
  esp_err_t err = esp_now_send(peerAddress, (uint8_t *) &message, sizeof(message_t)); 
  if (err != ESP_OK) {
    // I dont care
  }
}

void sendStatus(){
  status_reply_t status;
  status.protocol_version = PROTOCOL_VERSION;
  status.message_type = MSG_STATUS_REPLY;
  status.firmware_version = FIRMWARE_VERSION;
  status.battery_mv = (uint16_t)(batteryVoltage*1000.0);
  status.rssi = rssi;
  status.packet_loss = packet_loss;
  status.values[0] = channelValues[0];
  status.values[1] = channelValues[1];
  status.values[2] = channelValues[2];
  status.values[3] = channelValues[3];
  status.crc = getCRC((const uint8_t *) &status, sizeof(status_reply_t)-2);
  esp_err_t err = esp_now_send(peerAddress, (uint8_t *) &status, sizeof(status_reply_t)); 
  if (err != ESP_OK) {
    // I dont care
  }
}
