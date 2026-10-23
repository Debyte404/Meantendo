#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ═══════════════════════════════════════════════════════════
//  Meantendo Companion — True Peer-to-Peer Node (Player 2)
//  - Direct ESP-NOW on Channel 1 (Sub-2ms Deterministic Telemetry)
//  - Two Input Modes:
//      A) Physical Controls: Joystick on GPIO 34/35, Buttons on 32/33/26/27
//      B) Serial Keyboard: WASD for Joystick, J/K/U/I for A/B/X/Y
//      C) Auto-Bot Mode: Type 'b' to toggle smooth AI Player 2
//  - Bidirectional Telemetry: Receives Player 1 inputs & sends Player 2 inputs
// ═══════════════════════════════════════════════════════════

struct __attribute__((packed)) MeantendoTelemetry {
    uint16_t magic;      // 0xDEB0 signature
    uint16_t seq;        // Rolling sequence number
    int16_t  joyX;       // Filtered X coordinate (0-4095)
    int16_t  joyY;       // Filtered Y coordinate (0-4095)
    uint8_t  buttons;    // Bitfield: [0:A, 1:B, 2:X, 3:Y, 4:SELECT, 5:BACK]
    uint8_t  battery;    // Battery / power indicator (0-100 or status)
    uint16_t checksum;   // Simple checksum
};

// GPIO Pins matching standard Meantendo wiring
#define PIN_JOY_X   34
#define PIN_JOY_Y   35
#define PIN_BTN_A   32
#define PIN_BTN_B   33
#define PIN_BTN_X   26
#define PIN_BTN_Y   27
#define PIN_BTN_SEL 19
#define PIN_BTN_BCK 25

static uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Local Player 2 State
static int16_t  p2JoyX = 2048;
static int16_t  p2JoyY = 2048;
static uint8_t  p2Buttons = 0;
static uint16_t p2Seq = 0;
static uint32_t p2PacketsSent = 0;

// Remote Player 1 (Meantendo Console) State
static volatile int16_t  p1JoyX = 2048;
static volatile int16_t  p1JoyY = 2048;
static volatile uint8_t  p1Buttons = 0;
static volatile uint16_t p1Seq = 0;
static volatile uint32_t p1PacketsRecv = 0;
static volatile unsigned long lastP1PacketTime = 0;

// Operational Modes
static bool autoBotMode = false;
static bool hasPhysicalControls = false;

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
    p1Seq = pkt->seq;
    p1PacketsRecv++;
    lastP1PacketTime = millis();
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n╔═══════════════════════════════════════════════════════╗");
    Serial.println("║   MEANTENDO COMPANION — PEER CONSOLE / CONTROLLER 2   ║");
    Serial.println("╚═══════════════════════════════════════════════════════╝");

    // Initialize physical GPIOs with pullups
    pinMode(PIN_BTN_A, INPUT_PULLUP);
    pinMode(PIN_BTN_B, INPUT_PULLUP);
    pinMode(PIN_BTN_X, INPUT_PULLUP);
    pinMode(PIN_BTN_Y, INPUT_PULLUP);
    pinMode(PIN_BTN_SEL, INPUT_PULLUP);
    pinMode(PIN_BTN_BCK, INPUT_PULLUP);

    // Check if analog joystick appears to be wired (resting near ~2048 +/- 1200)
    int testX = analogRead(PIN_JOY_X);
    int testY = analogRead(PIN_JOY_Y);
    if (testX > 500 && testX < 3500 && testY > 500 && testY < 3500) {
        hasPhysicalControls = true;
        Serial.printf("[HW] Physical Joystick Detected! (X=%d, Y=%d)\n", testX, testY);
    } else {
        hasPhysicalControls = false;
        Serial.println("[HW] No physical joystick detected; Serial Keyboard active!");
    }

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
            Serial.println("[OK] ESP-NOW initialized on Channel 1 (Peer-to-Peer)");
        }
    }

    Serial.println("\n── CONTROLS GUIDE ──────────────────────────────────────");
    Serial.println(" • W / S : Joystick UP / DOWN  (P2 Paddle in Pong)");
    Serial.println(" • A / D : Joystick LEFT / RIGHT");
    Serial.println(" • J / K : Button A / Button B");
    Serial.println(" • U / I : Button X / Button Y");
    Serial.println(" • SPACE : Center Joystick & Release Buttons");
    Serial.println(" • B     : Toggle Auto-Bot (AI Player 2 Mode)");
    Serial.println("────────────────────────────────────────────────────────\n");
}

void loop() {
    // 1. Handle Serial Keyboard Input
    if (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case 'w': case 'W': p2JoyY = max(0, p2JoyY - 600); break;
            case 's': case 'S': p2JoyY = min(4095, p2JoyY + 600); break;
            case 'a': case 'A': p2JoyX = max(0, p2JoyX - 600); break;
            case 'd': case 'D': p2JoyX = min(4095, p2JoyX + 600); break;
            case ' ':           p2JoyX = 2048; p2JoyY = 2048; p2Buttons = 0; break;
            case 'j': case 'J': p2Buttons ^= (1 << 0); break; // Toggle A
            case 'k': case 'K': p2Buttons ^= (1 << 1); break; // Toggle B
            case 'u': case 'U': p2Buttons ^= (1 << 2); break; // Toggle X
            case 'i': case 'I': p2Buttons ^= (1 << 3); break; // Toggle Y
            case 'b': case 'B':
                autoBotMode = !autoBotMode;
                Serial.printf("\n>>> AUTO-BOT AI MODE: %s <<<\n\n", autoBotMode ? "ENABLED (Auto-playing P2)" : "DISABLED (Manual controls)");
                break;
            default: break;
        }
    }

    // 2. Read Physical Pins if available
    if (hasPhysicalControls) {
        p2JoyX = 4095 - analogRead(PIN_JOY_Y);
        p2JoyY = analogRead(PIN_JOY_X);
        p2Buttons = 0;
        if (digitalRead(PIN_BTN_A) == LOW)   p2Buttons |= (1 << 0);
        if (digitalRead(PIN_BTN_B) == LOW)   p2Buttons |= (1 << 1);
        if (digitalRead(PIN_BTN_X) == LOW)   p2Buttons |= (1 << 2);
        if (digitalRead(PIN_BTN_Y) == LOW)   p2Buttons |= (1 << 3);
        if (digitalRead(PIN_BTN_SEL) == LOW) p2Buttons |= (1 << 4);
        if (digitalRead(PIN_BTN_BCK) == LOW) p2Buttons |= (1 << 5);
    }

    // 3. Auto-Bot AI (Simulates human Player 2 tracking when enabled)
    if (autoBotMode) {
        float angle = (millis() % 2400) * (2.0f * 3.14159f / 2400.0f);
        p2JoyY = 2048 + (int)(1500.0f * sin(angle));
        p2JoyX = 2048 + (int)(800.0f * cos(angle));
        int phase = (millis() / 500) % 4;
        p2Buttons = (1 << phase);
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
        txPkt.checksum = calcChecksum(txPkt);

        if (esp_now_send(broadcastMac, reinterpret_cast<uint8_t*>(&txPkt), sizeof(txPkt)) == ESP_OK) {
            p2PacketsSent++;
        }
    }

    // 5. Periodic Diagnostic Console (Every 1000ms)
    static unsigned long lastLog = 0;
    if (millis() - lastLog >= 1000) {
        lastLog = millis();

        bool p1Connected = (millis() - lastP1PacketTime < 1000);

        char p1BtnStr[32] = "";
        if (p1Buttons & (1 << 0)) strcat(p1BtnStr, "A ");
        if (p1Buttons & (1 << 1)) strcat(p1BtnStr, "B ");
        if (p1Buttons & (1 << 2)) strcat(p1BtnStr, "X ");
        if (p1Buttons & (1 << 3)) strcat(p1BtnStr, "Y ");

        char p2BtnStr[32] = "";
        if (p2Buttons & (1 << 0)) strcat(p2BtnStr, "A ");
        if (p2Buttons & (1 << 1)) strcat(p2BtnStr, "B ");
        if (p2Buttons & (1 << 2)) strcat(p2BtnStr, "X ");
        if (p2Buttons & (1 << 3)) strcat(p2BtnStr, "Y ");

        if (p1Connected) {
            Serial.printf("[PEER-LINK: OK] P1 (Meantendo): Y=%4d Btns:[%s] RX#%u | P2 (Companion): Y=%4d Btns:[%s] TX#%u\n",
                          p1JoyY, p1BtnStr, p1PacketsRecv, p2JoyY, p2BtnStr, p2PacketsSent);
        } else {
            Serial.printf("[PEER-LINK: WAITING] Transmitting P2 Telemetry (TX#%u) | Waiting for Meantendo Console packets on Channel 1...\n",
                          p2PacketsSent);
        }
    }
}
