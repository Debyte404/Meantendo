#pragma once
#include <Arduino.h>

struct __attribute__((packed)) MeantendoTelemetry {
    uint16_t magic;      // 0xDEB0 signature
    uint16_t seq;        // Rolling sequence number
    int16_t  joyX;       // Filtered X coordinate (0-4095)
    int16_t  joyY;       // Filtered Y coordinate (0-4095)
    uint8_t  buttons;    // Bitfield: [0:A, 1:B, 2:X, 3:Y, 4:SELECT, 5:BACK]
    uint8_t  battery;    // Battery / power indicator (0-100 or status)
    uint16_t checksum;   // Simple checksum
};

namespace MeantendoWireless {
    bool init();
    void sendTelemetry(int rawFilteredX, int rawFilteredY, uint8_t buttonBitmask);
    bool isEnabled();
    void setEnabled(bool enable);
    uint32_t getPacketsSent();
    uint32_t getPacketsRecv();
    int16_t getRemoteJoyY();
    uint8_t getRemoteButtons();
    bool isPeerConnected();
}
