# ESP-NOW Wireless Lamp Protocol Proposal

## Overview

The system consists of:

- **1 Master** (ESP32)
  - Receives DMX input.
  - Converts DMX to ESP-NOW broadcast packets.
  - Handles configuration and diagnostics.

- **N Lamp Nodes** (ESP32)
  - Receive broadcast lighting data.
  - Listen for individually addressed management messages.
  - Report status when requested.

---

# Communication Types

## 1. Broadcast Lighting Frames

Purpose:
- Real-time lighting updates.
- Sent continuously (25–40 Hz recommended).
- Every frame contains the complete virtual DMX universe.

Advantages:

- Packet loss automatically corrected by next frame.
- No synchronization state.
- Very simple firmware.
- Predictable timing.
- Similar behavior to standard DMX.

Each lamp simply extracts its configured channels.

---

## 2. Unicast Management Messages

Purpose:

- Configuration
- Diagnostics
- Firmware information
- Battery status
- Identify lamp
- Factory reset
- Future firmware update support

These messages use the lamp's MAC address.

---

# Lamp Configuration

Each lamp stores its own configuration in NVS.

```c
typedef struct
{
    uint8_t  lamp_id;         // Human-readable ID
    uint16_t start_channel;   // First DMX channel
    uint8_t  channel_count;   // 1, 3 or 4
    uint8_t  personality;     // Future use
} lamp_config_t;
```

---

# Broadcast Lighting Packet

```c
typedef struct __attribute__((packed))
{
    uint8_t  protocol_version;    // Protocol compatibility
    uint8_t  message_type;        // MSG_LIGHT_FRAME
    uint16_t sequence;            // Incrementing frame counter
    uint16_t first_channel;       // First transmitted channel
    uint16_t channel_count;       // Number of transmitted channels

    uint8_t  data[channel_count]; // DMX values

    uint16_t crc;
} light_frame_t;
```

# Suggested Message Types

```c
typedef enum
{
    MSG_LIGHT_FRAME = 0,

    MSG_DISCOVER,
    MSG_IDENTIFY,

    MSG_GET_STATUS,
    MSG_STATUS_REPLY,

    MSG_SET_CONFIG,
    MSG_SAVE_CONFIG,

    MSG_FACTORY_RESET

} message_type_t;
```

Future additions:

- Read configuration
- Set brightness limit

---

# Status Reply

```c
typedef struct __attribute__((packed))
{
    uint8_t  protocol_version;
    uint8_t  message_type;
    uint8_t  lamp_id;
    uint16_t battery_mv;

    int8_t   rssi;

    uint32_t uptime_seconds;

    uint16_t packets_received;
    uint16_t packets_missed;

    uint8_t  firmware;

    uint16_t crc;

} status_reply_t;
```

---

# Discovery Procedure

1. Master broadcasts `DISCOVER`.
2. Lamps wait a random delay.
3. Lamps reply with:
   - MAC address
   - Lamp ID
   - Firmware version
   - Battery voltage
4. Master builds a list of available lamps.
5. Repeat to make up for cross talks

---

# Configuration Procedure

1. User selects a lamp, by MAC.
2. Master sends `IDENTIFY`.
3. Lamp flashes LEDs.
4. User confirms.
5. Master sends new configuration.
6. Lamp stores configuration in NVS.
7. Lamp acknowledges.

---

# Runtime Behavior

Lighting:

```
DMX
 ↓
Master
 ↓
Broadcast lighting frame
 ↓
All lamps receive packet
 ↓
Each lamp extracts its own channels
 ↓
PWM output updated
```

---

# Packet Loss

Each lighting frame contains a sequence number.

Lamp behavior:

- Missing one frame:
  - Ignore.
  - Keep previous output.

- Missing several frames:
  - Continue holding output.

- Timeout (e.g. 500 ms):
  - Continue holding output. Better to avoid blackouts during show.


---

# Battery Monitoring

Battery does **not** need to be broadcast continuously.

Master polls lamps individually.

Example:

```
GET_STATUS Lamp 1
← STATUS_REPLY

GET_STATUS Lamp 2
← STATUS_REPLY

GET_STATUS Lamp 3
← STATUS_REPLY
```

Polling every minute is sufficient.

---

# Identity

Each lamp has two identities.

## MAC Address

Permanent hardware identity.

Used for:

- Configuration
- Diagnostics
- Firmware updates

---

## Lamp ID

Human-readable number.

Example:

```
Lamp 1
Lamp 2
Lamp 3
...
```

Printed on the lamp for easy identification.

---

# Design Principles

- Continuous broadcast for lighting.
- Unicast for configuration and diagnostics.
- Simple lamp firmware.
- No synchronization state.
- DMX-like behavior.
- Store configuration in NVS.
- Sequence numbers for diagnostics.
- Protocol versioning from day one.
- CRC on every packet.