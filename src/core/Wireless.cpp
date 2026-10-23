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

// Peer Telemetry State
static volatile int16_t remoteJoyY = 2048;
static volatile uint8_t remoteButtons = 0;
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

    remoteJoyY = pkt->joyY;
    remoteButtons = pkt->buttons;
    packetsRecv++;
    lastPeerPacketTime = millis();
}

bool init() {
    if (initialized) return true;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_max_tx_power(78); // ~19.5 dBm

    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERR] ESP-NOW initialization failed");
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastMac, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("[ERR] Failed to register broadcast peer");
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
    pkt.checksum = calcChecksum(pkt);

    esp_err_t result = esp_now_send(broadcastMac, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
    if (result == ESP_OK) {
        packetsSent++;
    }
}

bool isEnabled() { return enabled; }
void setEnabled(bool enable) { enabled = enable; }
uint32_t getPacketsSent() { return packetsSent; }
uint32_t getPacketsRecv() { return packetsRecv; }
int16_t getRemoteJoyY() { return remoteJoyY; }
uint8_t getRemoteButtons() { return remoteButtons; }

bool isPeerConnected() {
    return (millis() - lastPeerPacketTime < 1000);
}

} // namespace MeantendoWireless
