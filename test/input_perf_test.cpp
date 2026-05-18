#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ── WiFi ─────────────────────────────────────
const char* SSID     = "Debyte";
const char* PASSWORD = "123456789";

// ── Pins ─────────────────────────────────────
#define LED_PIN      2
#define JOY_X_PIN    34
#define JOY_Y_PIN    35
#define BTN_SELECT   19
#define BTN_A        32
#define BTN_B        33
#define BTN_X        26
#define BTN_Y        27
#define BTN_BACK     25

#define JOY_DEADZONE 400

// sampled at boot, joystick must be at rest
int JOY_X_CENTER = 2048;
int JOY_Y_CENTER = 2048;

// ── Web server ────────────────────────────────
WebServer server(80);

// ── Input state ──────────────────────────────
struct Button {
  const char* name;
  uint8_t     pin;
  bool        pressed;
};

Button buttons[] = {
  { "A",      BTN_A,      false },
  { "B",      BTN_B,      false },
  { "X",      BTN_X,      false },
  { "Y",      BTN_Y,      false },
  { "SELECT", BTN_SELECT, false },
  { "BACK",   BTN_BACK,   false },
};
const uint8_t BTN_COUNT = sizeof(buttons) / sizeof(buttons[0]);

int joyX = 0, joyY = 0;

// ── HTML ──────────────────────────────────────
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Handheld Tester</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@400;700&display=swap');
  :root {
    --bg:      #0d0f12;
    --surface: #161920;
    --border:  #2a2f3a;
    --accent:  #00e5a0;
    --accent2: #e5006e;
    --dim:     #2a3040;
    --text:    #c8d0e0;
    --muted:   #4a5568;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'Share Tech Mono', monospace;
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 24px;
    gap: 24px;
  }
  h1 {
    font-family: 'Orbitron', sans-serif;
    font-size: 13px;
    font-weight: 700;
    letter-spacing: 0.3em;
    color: var(--accent);
    text-transform: uppercase;
  }
  .card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 10px;
    padding: 20px 24px;
    width: 100%;
    max-width: 420px;
  }
  .card-title {
    font-family: 'Orbitron', sans-serif;
    font-size: 10px;
    letter-spacing: 0.25em;
    color: var(--muted);
    text-transform: uppercase;
    margin-bottom: 16px;
  }
  .btn-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
  }
  .btn {
    height: 52px;
    border-radius: 8px;
    border: 1px solid var(--dim);
    background: var(--dim);
    display: flex;
    align-items: center;
    justify-content: center;
    font-family: 'Orbitron', sans-serif;
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 0.1em;
    color: var(--muted);
    transition: background 0.06s, border-color 0.06s, color 0.06s, box-shadow 0.06s;
  }
  .btn.active {
    background: var(--accent);
    border-color: var(--accent);
    color: #000;
    box-shadow: 0 0 18px rgba(0,229,160,0.35);
  }
  .btn.active-alt {
    background: var(--accent2);
    border-color: var(--accent2);
    color: #fff;
    box-shadow: 0 0 18px rgba(229,0,110,0.35);
  }
  .joy-wrap { display: flex; flex-direction: column; gap: 14px; }
  .joy-field { display: flex; flex-direction: column; gap: 6px; }
  .joy-label {
    font-size: 10px;
    letter-spacing: 0.2em;
    color: var(--muted);
    display: flex;
    justify-content: space-between;
  }
  .joy-label span { color: var(--accent); }
  .bar-track {
    height: 6px;
    background: var(--dim);
    border-radius: 3px;
    overflow: hidden;
    position: relative;
  }
  .bar-fill {
    position: absolute;
    height: 100%;
    background: var(--accent);
    border-radius: 3px;
    transition: left 0.05s, width 0.05s;
  }
  .dpad {
    display: grid;
    grid-template-columns: repeat(3, 32px);
    grid-template-rows: repeat(3, 32px);
    gap: 3px;
    margin: 0 auto;
    width: fit-content;
  }
  .dpad-cell {
    border-radius: 4px;
    background: var(--dim);
    border: 1px solid var(--border);
    display: flex; align-items: center; justify-content: center;
    font-size: 14px;
    transition: background 0.06s, border-color 0.06s;
  }
  .dpad-cell.center { background: var(--surface); border-color: var(--border); }
  .dpad-cell.active { background: var(--accent); border-color: var(--accent); }
  .status {
    font-size: 10px;
    color: var(--muted);
    letter-spacing: 0.15em;
    text-align: center;
  }
  .status .dot {
    display: inline-block;
    width: 6px; height: 6px;
    border-radius: 50%;
    background: var(--accent);
    margin-right: 6px;
    animation: pulse 1.4s ease-in-out infinite;
  }
  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50%       { opacity: 0.3; }
  }
</style>
</head>
<body>
<h1>Handheld Input Tester</h1>
<div class="card">
  <div class="card-title">Buttons</div>
  <div class="btn-grid">
    <div class="btn" id="btn-A">A</div>
    <div class="btn" id="btn-B">B</div>
    <div class="btn" id="btn-X">X</div>
    <div class="btn" id="btn-Y">Y</div>
    <div class="btn" id="btn-SELECT">SELECT</div>
    <div class="btn" id="btn-BACK">BACK</div>
  </div>
</div>
<div class="card">
  <div class="card-title">Joystick</div>
  <div class="joy-wrap">
    <div class="joy-field">
      <div class="joy-label">X AXIS <span id="jx-val">—</span></div>
      <div class="bar-track"><div class="bar-fill" id="jx-bar" style="left:50%;width:2px"></div></div>
    </div>
    <div class="joy-field">
      <div class="joy-label">Y AXIS <span id="jy-val">—</span></div>
      <div class="bar-track"><div class="bar-fill" id="jy-bar" style="left:50%;width:2px"></div></div>
    </div>
    <div class="dpad">
      <div class="dpad-cell"></div>
      <div class="dpad-cell" id="d-up">&#9650;</div>
      <div class="dpad-cell"></div>
      <div class="dpad-cell" id="d-left">&#9664;</div>
      <div class="dpad-cell center"></div>
      <div class="dpad-cell" id="d-right">&#9654;</div>
      <div class="dpad-cell"></div>
      <div class="dpad-cell" id="d-down">&#9660;</div>
      <div class="dpad-cell"></div>
    </div>
  </div>
</div>
<div class="status"><span class="dot"></span>POLLING LIVE</div>
<script>
const DEAD = 400, MAX = 4095;
let centerX = 2048, centerY = 2048;
const btns   = ['A','B','X','Y','SELECT','BACK'];
const abBtns = new Set(['A','B']);

async function poll() {
  try {
    const r = await fetch('/state');
    const d = await r.json();

    // use server-reported centers on first poll
    centerX = d.cx;
    centerY = d.cy;

    btns.forEach(name => {
      const el = document.getElementById('btn-' + name);
      el.classList.toggle('active',     d.buttons[name] && !abBtns.has(name));
      el.classList.toggle('active-alt', d.buttons[name] &&  abBtns.has(name));
    });

    const jx = d.jx, jy = d.jy;
    document.getElementById('jx-val').textContent = jx;
    document.getElementById('jy-val').textContent = jy;
    setBar('jx-bar', jx, centerX);
    setBar('jy-bar', jy, centerY);

    document.getElementById('d-left').classList.toggle('active',  jx < centerX - DEAD);
    document.getElementById('d-right').classList.toggle('active', jx > centerX + DEAD);
    document.getElementById('d-up').classList.toggle('active',    jy < centerY - DEAD);
    document.getElementById('d-down').classList.toggle('active',  jy > centerY + DEAD);
  } catch(e) {}
  setTimeout(poll, 50);
}

function setBar(id, val, center) {
  const el = document.getElementById(id);
  if (val < center - DEAD) {
    const w = ((center - val) / center) * 50;
    el.style.left  = (50 - w) + '%';
    el.style.width = w + '%';
  } else if (val > center + DEAD) {
    const w = ((val - center) / (MAX - center)) * 50;
    el.style.left  = '50%';
    el.style.width = w + '%';
  } else {
    el.style.left  = '50%';
    el.style.width = '2px';
  }
}

poll();
</script>
</body>
</html>
)rawhtml";

// ── /state ────────────────────────────────────
void handleState() {
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    buttons[i].pressed = (digitalRead(buttons[i].pin) == LOW);
  }
  joyX = analogRead(JOY_X_PIN);
  joyY = analogRead(JOY_Y_PIN);

  String json = "{\"buttons\":{";
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    json += "\"";
    json += buttons[i].name;
    json += "\":";
    json += buttons[i].pressed ? "true" : "false";
    if (i < BTN_COUNT - 1) json += ",";
  }
  json += "},\"jx\":"; json += joyX;
  json += ",\"jy\":";  json += joyY;
  json += ",\"cx\":";  json += JOY_X_CENTER;
  json += ",\"cy\":";  json += JOY_Y_CENTER;
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

// ── Setup ─────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
  pinMode(JOY_X_PIN, INPUT);
  pinMode(JOY_Y_PIN, INPUT);

  // set ADC to full 0-3.3V range, 12-bit resolution
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // sample joystick center at boot — keep joystick untouched at power-on
  delay(100);
  JOY_X_CENTER = analogRead(JOY_X_PIN);
  JOY_Y_CENTER = analogRead(JOY_Y_PIN);
  Serial.printf("Joy center: X=%d  Y=%d\n", JOY_X_CENTER, JOY_Y_CENTER);

  Serial.printf("Connecting to %s", SSID);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  digitalWrite(LED_PIN, HIGH);
  Serial.printf("\nConnected! Open http://%s\n", WiFi.localIP().toString().c_str());

  server.on("/",      handleRoot);
  server.on("/state", handleState);
  server.begin();
}

// ── Loop ──────────────────────────────────────
void loop() {
  server.handleClient();
}