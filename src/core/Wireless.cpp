#include "Wireless.hpp"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

namespace MeantendoWireless {

static bool initialized = false;
static bool enabled = true;
static uint32_t packetsSent = 0;
static uint32_t packetsRecv = 0;
static uint16_t sequenceNum = 0;
static uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Peer Telemetry & Game State
static volatile int16_t  remoteJoyX = 2048;
static volatile int16_t  remoteJoyY = 2048;
static volatile uint8_t  remoteButtons = 0;
static volatile uint8_t  remoteGameId = 0;
static volatile uint8_t  remoteBallX = 0;
static volatile uint8_t  remoteBallY = 0;
static volatile uint8_t  remoteScoreP1 = 0;
static volatile uint8_t  remoteScoreP2 = 0;
static volatile unsigned long lastPeerPacketTime = 0;

static uint16_t calcChecksum(const MeantendoTelemetry &pkt) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t*>(&pkt);
    size_t len = sizeof(MeantendoTelemetry) - sizeof(uint16_t);
    uint16_t sum = 0xAA55;
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 1) ^ bytes[i];
    }
    return sum;
}

static void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len != sizeof(MeantendoTelemetry)) return;
    const MeantendoTelemetry *pkt = reinterpret_cast<const MeantendoTelemetry*>(incomingData);
    if (pkt->magic != 0xDEB0) return;

    remoteJoyX = pkt->joyX;
    remoteJoyY = pkt->joyY;
    remoteButtons = pkt->buttons;
    remoteGameId = pkt->gameId;
    remoteBallX = pkt->ballX;
    remoteBallY = pkt->ballY;
    remoteScoreP1 = pkt->scoreP1;
    remoteScoreP2 = pkt->scoreP2;
    packetsRecv++;
    lastPeerPacketTime = millis();
}

bool init() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_max_tx_power(78); // ~19.5 dBm

    esp_now_deinit();
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERR] ESP-NOW initialization failed");
        initialized = false;
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[ERR] Failed to register broadcast peer");
        initialized = false;
        return false;
    }

    initialized = true;
    enabled = true;
    Serial.println("[OK] Meantendo Wireless ESP-NOW active on Channel 1 (TX/RX)");
    return true;
}

void sendTelemetry(int rawFilteredX, int rawFilteredY, uint8_t buttonBitmask) {
    if (!initialized || !enabled) return;

    MeantendoTelemetry pkt;
    pkt.magic = 0xDEB0;
    pkt.seq = sequenceNum++;
    pkt.joyX = (int16_t)rawFilteredX;
    pkt.joyY = (int16_t)rawFilteredY;
    pkt.buttons = buttonBitmask;
    pkt.battery = 100;
    pkt.gameId = 0;
    pkt.ballX = 0;
    pkt.ballY = 0;
    pkt.scoreP1 = 0;
    pkt.scoreP2 = 0;
    pkt.extra = 0;
    pkt.checksum = calcChecksum(pkt);

    if (esp_now_send(broadcastMac, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt)) == ESP_OK) {
        packetsSent++;
    }
}

void sendGameState(uint8_t gameId, uint8_t ballX, uint8_t ballY, uint8_t scoreP1, uint8_t scoreP2, int joyX, int joyY, uint8_t btns) {
    if (!initialized || !enabled) return;

    MeantendoTelemetry pkt;
    pkt.magic = 0xDEB0;
    pkt.seq = sequenceNum++;
    pkt.joyX = (int16_t)joyX;
    pkt.joyY = (int16_t)joyY;
    pkt.buttons = btns;
    pkt.battery = 100;
    pkt.gameId = gameId;
    pkt.ballX = ballX;
    pkt.ballY = ballY;
    pkt.scoreP1 = scoreP1;
    pkt.scoreP2 = scoreP2;
    pkt.extra = 0;
    pkt.checksum = calcChecksum(pkt);

    if (esp_now_send(broadcastMac, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt)) == ESP_OK) {
        packetsSent++;
    }
}

bool isEnabled() { return enabled; }
void setEnabled(bool enable) { enabled = enable; }
uint32_t getPacketsSent() { return packetsSent; }
uint32_t getPacketsRecv() { return packetsRecv; }
int16_t getRemoteJoyX() { return remoteJoyX; }
int16_t getRemoteJoyY() { return remoteJoyY; }
uint8_t getRemoteButtons() { return remoteButtons; }

uint8_t getRemoteGameId() { return remoteGameId; }
uint8_t getRemoteBallX() { return remoteBallX; }
uint8_t getRemoteBallY() { return remoteBallY; }
uint8_t getRemoteScoreP1() { return remoteScoreP1; }
uint8_t getRemoteScoreP2() { return remoteScoreP2; }

bool isPeerConnected() {
    return (millis() - lastPeerPacketTime < 1000);
}

} // namespace MeantendoWireless
