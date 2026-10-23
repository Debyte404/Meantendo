#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <cmath>

// ═══════════════════════════════════════════════════════════
//  Meantendo Companion Bridge & Multiplayer Tester Bot
//  - Receives ESP-NOW packets from Meantendo and relays to:
//      1. USB Serial Monitor (115200 baud)
//      2. Local Web Arcade Platform via WebSockets (Port 81)
//  - Automated Player 2 Spammer Bot (type 'b' in serial to toggle)
// ═══════════════════════════════════════════════════════════

struct __attribute__((packed)) MeantendoTelemetry {
    uint16_t magic;      // 0xDEB0
    uint16_t seq;        // Rolling sequence
    int16_t  joyX;       // 0-4095
    int16_t  joyY;       // 0-4095
    uint8_t  buttons;    // Bitmask
    uint8_t  battery;
    uint16_t checksum;
};

// Network Services
WebServer server(80);
WebSocketsServer webSocket(81);

// Telemetry State
volatile uint16_t lastSeq = 0;
volatile int16_t currentX = 2048;
volatile int16_t currentY = 2048;
volatile uint8_t currentButtons = 0;
volatile uint32_t totalPacketsRecv = 0;
volatile bool newDataAvailable = false;

static uint8_t broadcastMac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#ifdef TESTER_BOT_AUTOSTART
static bool botSpammerActive = true;
#else
static bool botSpammerActive = false;
#endif

// Embedded HTML Dashboard
extern const char INDEX_HTML[] PROGMEM;

// ── ESP-NOW Callback ──────────────────────────────────────
void onDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (len != sizeof(MeantendoTelemetry)) return;

    const MeantendoTelemetry *pkt = reinterpret_cast<const MeantendoTelemetry*>(data);
    if (pkt->magic != 0xDEB0) return;

    lastSeq = pkt->seq;
    currentX = pkt->joyX;
    currentY = pkt->joyY;
    currentButtons = pkt->buttons;
    totalPacketsRecv++;
    newDataAvailable = true;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n╔══════════════════════════════════════════╗");
    Serial.println("║    MEANTENDO COMPANION & TEST BOT        ║");
    Serial.println("╚══════════════════════════════════════════╝");

    // 1. Wi-Fi SoftAP Mode on Channel 1
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Meantendo-Arcade", "12345678", 1);
    IPAddress apIP = WiFi.softAPIP();

    Serial.printf("[OK] SoftAP 'Meantendo-Arcade' started\n");
    Serial.printf("[OK] Dashboard URL: http://%s/\n", apIP.toString().c_str());

    // 2. Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ERR] ESP-NOW Init Failed!");
    } else {
        esp_now_register_recv_cb(onDataRecv);

        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, broadcastMac, 6);
        peerInfo.channel = 1;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);

        Serial.println("[OK] ESP-NOW active on Channel 1 (TX/RX)");
    }

    // 3. HTTP Web Server
    server.on("/", []() {
        server.send_P(200, "text/html", INDEX_HTML);
    });
    server.begin();
    Serial.println("[OK] HTTP Server listening on port 80");

    // 4. WebSocket Server
    webSocket.begin();
    Serial.println("[OK] WebSocket Server listening on port 81");

    if (botSpammerActive) {
        Serial.println("\n>>> [AUTOSTART] Automated Player 2 Spammer Bot is ACTIVE! <<<");
    } else {
        Serial.println("\nTip: Type 'b' in Serial Monitor at any time to toggle Tester Bot mode!");
    }
}

void loop() {
    server.handleClient();
    webSocket.loop();

    // Check serial input to toggle bot spammer
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'b' || c == 'B') {
            botSpammerActive = !botSpammerActive;
            Serial.printf("\n>>> Tester Bot Mode: %s <<<\n", botSpammerActive ? "ENABLED" : "DISABLED");
        }
    }

    // Automated Tester Bot: Spams smooth simulated joystick moves and buttons
    if (botSpammerActive) {
        static unsigned long lastBotTx = 0;
        static uint16_t botSeq = 0;
        if (millis() - lastBotTx >= 30) {
            lastBotTx = millis();
            float angle = (millis() % 3000) * (2.0f * 3.14159f / 3000.0f);
            int simY = 2048 + (int)(1400.0f * sin(angle));
            int simX = 2048 + (int)(1000.0f * cos(angle));

            uint8_t simBtns = 0;
            int phase = (millis() / 400) % 4;
            if (phase == 0) simBtns |= (1 << 0); // A
            if (phase == 1) simBtns |= (1 << 1); // B
            if (phase == 2) simBtns |= (1 << 2); // X
            if (phase == 3) simBtns |= (1 << 3); // Y

            MeantendoTelemetry botPkt;
            botPkt.magic = 0xDEB0;
            botPkt.seq = botSeq++;
            botPkt.joyX = simX;
            botPkt.joyY = simY;
            botPkt.buttons = simBtns;
            botPkt.battery = 99;
            botPkt.checksum = 0;

            esp_now_send(broadcastMac, reinterpret_cast<uint8_t*>(&botPkt), sizeof(botPkt));

            static unsigned long lastLog = 0;
            if (millis() - lastLog >= 1000) {
                lastLog = millis();
                const char* bNames[] = { "A", "B", "X", "Y" };
                Serial.printf("[BOT-SPAMMER] TX #%5u | Simulating P2: Y=%4d | Btn: [%s]\n",
                              botSeq, simY, bNames[phase]);
            }
        }
    }

    // Stream received telemetry to Serial & WebSockets at ~50 Hz
    static unsigned long lastBroadcast = 0;
    unsigned long now = millis();

    if (newDataAvailable && (now - lastBroadcast >= 20)) {
        lastBroadcast = now;
        newDataAvailable = false;

        // Print to Serial
        char btnStr[32] = "";
        if (currentButtons & (1 << 0)) strcat(btnStr, "A ");
        if (currentButtons & (1 << 1)) strcat(btnStr, "B ");
        if (currentButtons & (1 << 2)) strcat(btnStr, "X ");
        if (currentButtons & (1 << 3)) strcat(btnStr, "Y ");
        if (currentButtons & (1 << 4)) strcat(btnStr, "SEL ");
        if (currentButtons & (1 << 5)) strcat(btnStr, "BACK ");

        Serial.printf("[ESP-NOW #%5u] X:%4d Y:%4d | Buttons: [%s]\n",
                      lastSeq, currentX, currentY, btnStr);

        // Broadcast JSON to all connected Web browsers
        char jsonBuf[128];
        snprintf(jsonBuf, sizeof(jsonBuf),
                 "{\"seq\":%u,\"x\":%d,\"y\":%d,\"b\":%u,\"total\":%u}",
                 lastSeq, currentX, currentY, currentButtons, totalPacketsRecv);
        webSocket.broadcastTXT(jsonBuf);
    }
}

// ── HTML Web Content ──────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Meantendo Arcade Bridge</title>
  <style>
    body { background: #0c0e17; color: #fff; font-family: system-ui, sans-serif; display: flex; flex-direction: column; align-items: center; padding: 20px; }
    h1 { background: linear-gradient(135deg, #00d2ff, #a855f7); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    .badge { padding: 4px 12px; border-radius: 20px; font-size: 0.8rem; background: rgba(0,210,255,0.2); color: #00d2ff; margin-bottom: 20px; }
    .grid { display: grid; grid-template-columns: 1fr 1.2fr; gap: 20px; max-width: 960px; width: 100%; }
    .card { background: #161a29; border-radius: 12px; padding: 20px; border: 1px solid #282f49; }
    .stick-box { width: 140px; height: 140px; border-radius: 50%; background: #08090f; border: 2px solid #333d61; position: relative; margin: 15px auto; }
    .thumb { width: 40px; height: 40px; border-radius: 50%; background: #00d2ff; position: absolute; top: 50px; left: 50px; box-shadow: 0 0 15px #00d2ff; transition: transform 0.03s linear; }
    .btn-row { display: flex; gap: 10px; justify-content: center; margin-top: 15px; }
    .btn { width: 45px; height: 45px; border-radius: 50%; background: #222940; display: flex; align-items: center; justify-content: center; font-weight: bold; }
    .btn.active { background: #00ff88; color: #000; box-shadow: 0 0 15px #00ff88; }
    canvas { width: 100%; height: 260px; background: #040508; border-radius: 8px; }
  </style>
</head>
<body>
  <h1>MEANTENDO CONNECT</h1>
  <div class="badge" id="status">Connecting to Bridge...</div>
  <div class="grid">
    <div class="card">
      <h3>Live Controller Stream</h3>
      <div class="stick-box"><div class="thumb" id="thumb"></div></div>
      <div class="btn-row">
        <div class="btn" id="bY">Y</div>
        <div class="btn" id="bX">X</div>
        <div class="btn" id="bB">B</div>
        <div class="btn" id="bA">A</div>
      </div>
      <p style="text-align: center; margin-top: 15px; color: #64748b;" id="telemetry">X: 2048 | Y: 2048</p>
    </div>
    <div class="card">
      <h3>Connected Big-Screen Arcade</h3>
      <canvas id="gameCanvas" width="400" height="260"></canvas>
      <p style="margin-top: 10px; font-size: 0.85rem; color: #94a3b8;">Control with Meantendo: <b>Stick</b> to move, <b>Button A</b> to fire!</p>
    </div>
  </div>
  <script>
    const status = document.getElementById('status');
    const thumb = document.getElementById('thumb');
    const tele = document.getElementById('telemetry');
    const bA = document.getElementById('bA');
    const bB = document.getElementById('bB');
    const bX = document.getElementById('bX');
    const bY = document.getElementById('bY');
    let input = { x: 2048, y: 2048, a: false };

    const ws = new WebSocket(`ws://${location.hostname}:81/`);
    ws.onopen = () => { status.textContent = 'ESP-NOW Connected (<3ms)'; status.style.color = '#00ff88'; };
    ws.onmessage = (e) => {
      const d = JSON.parse(e.data);
      input.x = d.x; input.y = d.y;
      input.a = Boolean(d.b & 1);
      const offX = ((d.x - 2048) / 2048) * 45;
      const offY = ((d.y - 2048) / 2048) * 45;
      thumb.style.transform = `translate(${offX}px, ${offY}px)`;
      tele.textContent = `Seq: #${d.seq} | X: ${d.x} | Y: ${d.y}`;
      bA.className = 'btn' + (d.b & 1 ? ' active' : '');
      bB.className = 'btn' + (d.b & 2 ? ' active' : '');
      bX.className = 'btn' + (d.b & 4 ? ' active' : '');
      bY.className = 'btn' + (d.b & 8 ? ' active' : '');
    };

    // Canvas Game
    const cvs = document.getElementById('gameCanvas');
    const ctx = cvs.getContext('2d');
    let p = { x: 200, y: 220 };
    let bullets = [], rocks = [];
    function loop() {
      ctx.fillStyle = '#040508'; ctx.fillRect(0,0,400,260);
      const nx = (input.x - 2048) / 2048;
      const ny = (input.y - 2048) / 2048;
      if (Math.abs(nx) > 0.15) p.x += nx * 4;
      if (Math.abs(ny) > 0.15) p.y += ny * 4;
      p.x = Math.max(10, Math.min(390, p.x));
      p.y = Math.max(10, Math.min(250, p.y));

      if (input.a && Math.random() < 0.2) bullets.push({x: p.x, y: p.y - 10});
      ctx.fillStyle = '#00ff88';
      bullets.forEach((b, i) => { b.y -= 6; ctx.fillRect(b.x-2, b.y, 4, 8); if (b.y < 0) bullets.splice(i, 1); });

      if (Math.random() < 0.03) rocks.push({x: Math.random()*380+10, y: 0});
      ctx.fillStyle = '#ff4466';
      rocks.forEach((r, i) => { r.y += 2; ctx.beginPath(); ctx.arc(r.x, r.y, 8, 0, 7); ctx.fill(); if (r.y > 270) rocks.splice(i, 1); });

      ctx.fillStyle = '#00d2ff';
      ctx.beginPath(); ctx.moveTo(p.x, p.y-10); ctx.lineTo(p.x-8, p.y+8); ctx.lineTo(p.x+8, p.y+8); ctx.fill();
      requestAnimationFrame(loop);
    }
    loop();
  </script>
</body>
</html>
)rawhtml";
