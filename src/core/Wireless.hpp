#pragma once
#include <Arduino.h>

struct __attribute__((packed)) MeantendoTelemetry {
    uint16_t magic;      // 0xDEB0 signature
    uint16_t seq;        // Rolling sequence number
    int16_t  joyX;       // Filtered X coordinate (0-4095)
    int16_t  joyY;       // Filtered Y coordinate (0-4095)
    uint8_t  buttons;    // Bitfield: [0:A, 1:B, 2:X, 3:Y, 4:SELECT, 5:BACK]
    uint8_t  battery;    // Battery / power indicator (0-100 or status)
    uint8_t  gameId;     // Active game: 0=Menu, 1=Pong, 2=Snake
    uint8_t  ballX;      // Normalized Ball X coordinate (0-160)
    uint8_t  ballY;      // Normalized Ball Y coordinate (0-128)
    uint8_t  scoreP1;    // Player 1 score
    uint8_t  scoreP2;    // Player 2 score
    uint8_t  extra;      // Reserved
    uint16_t checksum;   // Simple checksum
};

namespace MeantendoWireless {
    bool init();
    void sendTelemetry(int rawFilteredX, int rawFilteredY, uint8_t buttonBitmask);
    void sendGameState(uint8_t gameId, uint8_t ballX, uint8_t ballY, uint8_t scoreP1, uint8_t scoreP2, int joyX, int joyY, uint8_t btns);
    bool isEnabled();
    void setEnabled(bool enable);
    uint32_t getPacketsSent();
    uint32_t getPacketsRecv();
    int16_t getRemoteJoyY();
    int16_t getRemoteJoyX();
    uint8_t getRemoteButtons();
    bool isPeerConnected();

    // Peer game state
    uint8_t getRemoteGameId();
    uint8_t getRemoteBallX();
    uint8_t getRemoteBallY();
    uint8_t getRemoteScoreP1();
    uint8_t getRemoteScoreP2();
}
