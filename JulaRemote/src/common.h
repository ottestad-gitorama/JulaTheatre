#pragma once
#include <stdio.h>

#define PROTOCOL_VERSION 101 // 1.01


// FIXTURE ///////////////////////////////////////////////////////////////////////////////////////
#define DMX_UNIVERSE_SIZE 100
extern uint8_t dmx_universe[DMX_UNIVERSE_SIZE];

typedef struct
{
    uint16_t  dmx_address;         // Human-readable ID
    uint8_t  channel_count;   // 1, 3 or 4
    uint8_t  personality;     // Future use
    float gamma[4];
} fixture_config_t;

// Master to light //////////////////////////////////////////////////////////////////////////////
typedef struct __attribute__((packed))
{
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // MSG_LIGHT_FRAME
    uint16_t sequence;            // Incrementing frame counter for uncritical loss detection
    uint8_t  data[DMX_UNIVERSE_SIZE]; // DMX values
    uint16_t crc;
} light_frame_t;

// Both ways ////////////////////////////////////////////////////////////////////////////////////////////

typedef struct __attribute__((packed))
{
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // ANY
    uint16_t parameter;
    uint16_t value;
    uint16_t crc;
} message_t;


typedef enum
{
    MSG_LIGHT_FRAME = 0,    // m -> s
    MSG_DISCOVER,           // m -> s
    MSG_DISCOVER_REPLY,     // s -> m 
    MSG_IDENTIFY,           // m -> s
    MSG_STATUS_REQUEST,     // m -> s
    MSG_STATUS_REPLY,       // s -> m
    MSG_CONFIG_SET,         // m -> s
    MSG_CONFIG_REQUEST,     // m -> s
    MSG_CONFIG_REPLY,       // s -> m
} message_enum;

typedef enum{
    CFG_ADDRESS = 0,
    CFG_CHANNEL_COUNT,
    CFG_PERSONALITY,
    CFG_GAMMA_0,
    CFG_GAMMA_1,
    CFG_GAMMA_2,
    CFG_GAMMA_3
} config_enum;

// Light to Master ////////////////////////////////////////////////////////////////////////////////////////
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




