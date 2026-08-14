#pragma once
#include <stdio.h>

#define DMX_UNIVERSE_SIZE 100
extern uint8_t dmx_universe[DMX_UNIVERSE_SIZE];

typedef struct
{
    uint16_t  dmx_address;         // Human-readable ID
    uint8_t  channel_count;   // 1, 3 or 4
    uint8_t  personality;     // Future use
} fixture_config_t;

// Master to light
typedef struct __attribute__((packed))
{
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // MSG_LIGHT_FRAME
    uint16_t sequence;            // Incrementing frame counter for uncritical loss detection
    uint8_t  data[DMX_UNIVERSE_SIZE]; // DMX values
    uint16_t crc;
} light_frame_t;

typedef struct __attribute__((packed))
{
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // ANY
    uint16_t parameter;
    uint16_t crc;
} command_t;

// Light to Master
typedef struct __attribute__((packed)){
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // MSG_STATUS_REPLY
    uint16_t firmware_version;  
    uint8_t  values[4];
    uint16_t battery_mv;
    int16_t rssi;
    uint16_t packet_loss;
    uint16_t crc;
} status_reply_t;

typedef struct  __attribute__((packed))
{
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // MSG_GET_CONFIG_REPLY
    fixture_config_t config;
    uint16_t crc;
    
} get_config_reply_t;

typedef struct __attribute__ ((packed)){
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // MSG_DISCOVER
    uint16_t crc;
} discover_t;

typedef enum
{
    MSG_LIGHT_FRAME = 0,
    MSG_DISCOVER,
    MSG_DISCOVER_REPLY,               
    MSG_IDENTIFY,
    MSG_GET_STATUS,
    MSG_STATUS_REPLY,
    MSG_GET_CONFIG,
    MSG_GET_CONFIG_REPLY,
    MSG_SET_DMX_ADDRESS,
    MSG_SET_FIXTURE_CHANNELS,
    MSG_SET_FIXTURE_PERSONALITY
} message_type_t;
