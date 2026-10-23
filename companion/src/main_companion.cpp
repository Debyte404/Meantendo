#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ═══════════════════════════════════════════════════════════
//  Meantendo Companion — True Peer-to-Peer Multiplayer Node
//  - Direct ESP-NOW on Channel 1 (Sub-2ms Deterministic Sync)
//  - Two-Way Peer Sync: Receives ball coordinates, scores & P1 paddle;
//    sends P2 paddle back in real-time.
//  - Auto-Bot AI by default (smoothly tracks ball to play as P2)
//  - Interactive Laptop Keyboard (WASD) in Serial Monitor
//  - Live ASCII Terminal Display on Laptop
// ═══════════════════════════════════════════════════════════

struct __attribute__((packed)) MeantendoTelemetry {
    uint16_t magic;      // 0xDEB0 signature
    uint16_t seq;        // Rolling sequence number
    int16_t  joyX;       // Filtered X coordinate (0-4095)
    int16_t  joyY;       // Filtered Y coordinate (0-4095)
    uint8_t  buttons;    // Bitfield: [0:A, 1:B, 2:X, 3:Y, 4:SELECT, 5:BACK]
    uint8_t  battery;    // Battery / power indicator (0-100 or status)
    uint8_t  gameId;     // 0=Menu, 1=Pong, 2=Snake
    uint8_t  ballX;      // Ball X (0-160)
    uint8_t  ballY;      // Ball Y (0-128)
    uint8_t  scoreP1;    // Player 1 score
    uint8_t  scoreP2;    // Player 2 score
    uint8_t  extra;      // Reserved
    uint16_t checksum;   // Simple checksum
};

#define PIN_JOY_X   34
#define PIN_JOY_Y   35
#define PIN_BTN_A   32
#define PIN_BTN_B   33
#define PIN_BTN_X   26
#define PIN_BTN_Y   27

static uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Player 2 Local State (Companion)
static int16_t  p2JoyX = 2048;
static int16_t  p2JoyY = 2048;
static uint8_t  p2Buttons = 0;
static uint16_t p2Seq = 0;
static uint32_t p2PacketsSent = 0;

// Player 1 Remote State (Meantendo Console)
static volatile int16_t  p1JoyX = 2048;
static volatile int16_t  p1JoyY = 2048;
static volatile uint8_t  p1Buttons = 0;
static volatile uint8_t  p1GameId = 0;
static volatile uint8_t  p1BallX = 80;
static volatile uint8_t  p1BallY = 64;
static volatile uint8_t  p1ScoreP1 = 0;
static volatile uint8_t  p1ScoreP2 = 0;
static volatile uint16_t p1Seq = 0;
static volatile uint32_t p1PacketsRecv = 0;
static volatile unsigned long lastP1PacketTime = 0;

// Active Modes
static bool autoBotMode = true; // Auto-Bot is ON by default so P2 plays immediately
static int initialAdcX = 0;
static int initialAdcY = 0;

static uint16_t calcChecksum(const MeantendoTelemetry &pkt) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t*>(&pkt);
    size_t len = sizeof(MeantendoTelemetry) - sizeof(uint16_t);
    uint16_t sum = 0xAA55;
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 1) ^ bytes[i];
    }
    return sum;
}

// ── ESP-NOW Receive Callback ──────────────────────────────
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len != sizeof(MeantendoTelemetry)) return;
    const MeantendoTelemetry *pkt = reinterpret_cast<const MeantendoTelemetry*>(incomingData);
    if (pkt->magic != 0xDEB0) return;

    p1JoyX = pkt->joyX;
    p1JoyY = pkt->joyY;
    p1Buttons = pkt->buttons;
    p1GameId = pkt->gameId;
    p1BallX = pkt->ballX;
    p1BallY = pkt->ballY;
    p1ScoreP1 = pkt->scoreP1;
    p1ScoreP2 = pkt->scoreP2;
    p1Seq = pkt->seq;
    p1PacketsRecv++;
    lastP1PacketTime = millis();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔══════════════════════════════════════════════════════════════════╗");
    Serial.println("║   MEANTENDO COMPANION — TRUE P2P MULTIPLAYER PEER NODE          ║");
    Serial.println("╚══════════════════════════════════════════════════════════════════╝");

    // Sample initial ADC to ignore floating pins if no real joystick is attached
    initialAdcX = analogRead(PIN_JOY_X);
    initialAdcY = analogRead(PIN_JOY_Y);

    // 1. Direct Wi-Fi Station Mode on Channel 1
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_max_tx_power(78); // ~19.5 dBm

    // 2. Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERR] ESP-NOW initialization failed!");
    } else {
        esp_now_register_recv_cb(onDataRecv);

        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, broadcastMac, 6);
        peerInfo.channel = 1;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("[ERR] Failed to add broadcast peer!");
        } else {
            Serial.println("[OK] ESP-NOW Peer-to-Peer active on Channel 1 (TX/RX)");
        }
    }

    Serial.println("\n── CONTROLS & DISPLAY GUIDE ────────────────────────────────────");
    Serial.println(" • AUTO-BOT AI: Active by default (tracks ball and returns it)");
    Serial.println(" • W / S      : Keyboard Manual Control (P2 Paddle UP / DOWN)");
    Serial.println(" • A / D      : P2 Left / Right");
    Serial.println(" • B          : Toggle Auto-Bot AI vs Manual Keyboard");
    Serial.println(" • SPACE      : Reset Paddle to Center");
    Serial.println("────────────────────────────────────────────────────────────────\n");
}

void loop() {
    // 1. Handle Serial Keyboard Input from Laptop
    if (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case 'w': case 'W':
                autoBotMode = false;
                p2JoyY = max(200, p2JoyY - 600);
                Serial.println("[INPUT] Manual UP (W) -> P2 Paddle moved UP");
                break;
            case 's': case 'S':
                autoBotMode = false;
                p2JoyY = min(3895, p2JoyY + 600);
                Serial.println("[INPUT] Manual DOWN (S) -> P2 Paddle moved DOWN");
                break;
            case 'a': case 'A': p2JoyX = max(200, p2JoyX - 600); break;
            case 'd': case 'D': p2JoyX = min(3895, p2JoyX + 600); break;
            case ' ':
                p2JoyX = 2048; p2JoyY = 2048; p2Buttons = 0;
                break;
            case 'b': case 'B':
                autoBotMode = !autoBotMode;
                Serial.printf("\n>>> [MODE SWITCH] Auto-Bot AI: %s <<<\n\n",
                              autoBotMode ? "ENABLED (AI playing P2)" : "DISABLED (Manual keyboard)");
                break;
            default: break;
        }
    }

    // 2. Physical joystick check (only if values deviate significantly from initial float)
    int curAdcX = analogRead(PIN_JOY_X);
    int curAdcY = analogRead(PIN_JOY_Y);
    if (abs(curAdcX - initialAdcX) > 400 || abs(curAdcY - initialAdcY) > 400) {
        autoBotMode = false;
        p2JoyX = 4095 - curAdcY;
        p2JoyY = curAdcX;
    }

    // 3. Auto-Bot AI — Tracks Pong Ball dynamically!
    if (autoBotMode) {
        if (p1GameId == 1) { // In Pong match
            // Map 0-128 ball Y to 0-4095 joystick range
            int targetJoyY = map(p1BallY, 10, 118, 400, 3695);
            // Move smoothly towards target
            int diff = targetJoyY - p2JoyY;
            p2JoyY += constrain(diff, -180, 180);
        } else {
            // General idle wandering when not in Pong
            float angle = (millis() % 2000) * (2.0f * 3.14159f / 2000.0f);
            p2JoyY = 2048 + (int)(1400.0f * sin(angle));
        }
    }

    // 4. Transmit Player 2 Telemetry at 60 Hz (~16ms)
    static unsigned long lastTx = 0;
    if (millis() - lastTx >= 16) {
        lastTx = millis();

        MeantendoTelemetry txPkt;
        txPkt.magic = 0xDEB0;
        txPkt.seq = p2Seq++;
        txPkt.joyX = p2JoyX;
        txPkt.joyY = p2JoyY;
        txPkt.buttons = p2Buttons;
        txPkt.battery = 100;
        txPkt.gameId = p1GameId;
        txPkt.ballX = p1BallX;
        txPkt.ballY = p1BallY;
        txPkt.scoreP1 = p1ScoreP1;
        txPkt.scoreP2 = p1ScoreP2;
        txPkt.extra = autoBotMode ? 1 : 0;
        txPkt.checksum = calcChecksum(txPkt);

        if (esp_now_send(broadcastMac, reinterpret_cast<uint8_t*>(&txPkt), sizeof(txPkt)) == ESP_OK) {
            p2PacketsSent++;
        }
    }

    // 5. Live ASCII Terminal Display (Updates every 300ms)
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay >= 300) {
        lastDisplay = millis();
        bool p1Connected = (millis() - lastP1PacketTime < 1000);

        if (p1Connected && p1GameId == 1) {
            // Live 2-Player Pong ASCII Match Visualizer!
            int courtWidth = 32;
            int bPos = map(p1BallX, 0, 160, 1, courtWidth - 2);
            bPos = constrain(bPos, 1, courtWidth - 2);

            char court[40];
            memset(court, ' ', courtWidth);
            court[0] = '|';
            court[courtWidth - 1] = '|';
            court[bPos] = 'O';
            court[courtWidth] = '\0';

            Serial.printf("[P1: %d | P2: %d] %s  [P1:Y=%3d | P2:Y=%3d] %s\n",
                          p1ScoreP1, p1ScoreP2, court,
                          map(p1JoyY, 0, 4095, 10, 108),
                          map(p2JoyY, 0, 4095, 10, 108),
                          autoBotMode ? "[AI TRACKING]" : "[MANUAL]");
        } else if (p1Connected) {
            Serial.printf("[PEER-LINK: OK] Connected to Meantendo! RX#%u | TX#%u | Mode: %s\n",
                          p1PacketsRecv, p2PacketsSent,
                          autoBotMode ? "Auto-Bot AI" : "Manual");
        } else {
            Serial.printf("[PEER-LINK: SEARCHING] TX#%u | Waiting for Meantendo Console on Channel 1...\n",
                          p2PacketsSent);
        }
    }
}
