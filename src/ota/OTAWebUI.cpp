/**
 * @file OTAWebUI.cpp
 * @brief Implementation of OTA Web Management Interface
 * @author Debyte
 * @version 2.0.0
 */

#include "OTAWebUI.hpp"
#include <ArduinoJson.h>

// Static instance pointer for callbacks
static OTAWebUI* _instance = nullptr;

// ============================================================================
//  Singleton Implementation
// ============================================================================

OTAWebUI& OTAWebUI::getInstance() {
    static OTAWebUI instance;
    return instance;
}

OTAWebUI::OTAWebUI() : _running(false), _server(nullptr), _webSocket(nullptr) {
    _instance = this;
}

OTAWebUI::~OTAWebUI() {
    end();
}

// ============================================================================
//  Initialization
// ============================================================================

bool OTAWebUI::begin(uint16_t port) {
    if (_running) return true;
    
    _server = new WebServer(port);
    _webSocket = new WebSocketsServer(OTA_WEBSOCKET_PORT);
    
    // Setup HTTP routes
    _server->on("/", HTTP_GET, _staticHandleRoot);
    _server->on("/api/status", HTTP_GET, _staticHandleStatus);
    _server->on("/api/check", HTTP_POST, _staticHandleCheckUpdate);
    _server->on("/api/update", HTTP_POST, _staticHandleStartUpdate);
    _server->on("/api/rollback", HTTP_POST, _staticHandleRollback);
    _server->on("/api/wifi", HTTP_GET, _staticHandleWiFiConfig);
    _server->on("/api/wifi", HTTP_POST, _staticHandleSaveWiFi);
    _server->on("/api/reboot", HTTP_POST, _staticHandleReboot);
    
    // Firmware upload handler
    _server->on("/api/upload", HTTP_POST, _staticHandleUploadComplete,
        [this]() { _handleUpload(); });
    
    _server->onNotFound(_staticHandleNotFound);
    
    // Setup WebSocket
    _webSocket->begin();
    _webSocket->onEvent([](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        if (_instance) {
            _instance->_onWebSocketEvent(num, type, payload, length);
        }
    });
    
    _server->begin();
    _running = true;
    
    Serial.printf("[WebUI] Started on port %d (WS: %d)\n", port, OTA_WEBSOCKET_PORT);
    return true;
}

void OTAWebUI::end() {
    if (!_running) return;
    
    if (_webSocket) {
        _webSocket->close();
        delete _webSocket;
        _webSocket = nullptr;
    }
    
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    
    _running = false;
}

void OTAWebUI::loop() {
    if (!_running) return;
    _server->handleClient();
    _webSocket->loop();
}

bool OTAWebUI::isRunning() const {
    return _running;
}

// ============================================================================
//  WebSocket Broadcasting
// ============================================================================

void OTAWebUI::broadcastProgress(uint8_t progress, const char* status) {
    if (!_running || !_webSocket) return;
    
    JsonDocument doc;
    doc["type"] = "progress";
    doc["progress"] = progress;
    doc["status"] = status;
    
    String json;
    serializeJson(doc, json);
    _webSocket->broadcastTXT(json);
}

void OTAWebUI::broadcastStatus(const char* jsonStatus) {
    if (!_running || !_webSocket) return;
    _webSocket->broadcastTXT(jsonStatus);
}

// ============================================================================
//  HTTP Handlers
// ============================================================================

void OTAWebUI::_handleRoot() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Meantendo OTA Update</title>
    <style>
        :root {
            --primary: #6366f1;
            --primary-dark: #4f46e5;
            --success: #10b981;
            --warning: #f59e0b;
            --danger: #ef4444;
            --bg: #0f172a;
            --card: #1e293b;
            --text: #f1f5f9;
            --muted: #94a3b8;
        }
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Segoe UI', system-ui, sans-serif;
            background: linear-gradient(135deg, var(--bg) 0%, #1a1a2e 100%);
            color: var(--text);
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 600px; margin: 0 auto; }
        .header {
            text-align: center;
            margin-bottom: 30px;
            padding: 20px;
        }
        .logo {
            font-size: 2.5rem;
            font-weight: 800;
            background: linear-gradient(135deg, #6366f1, #a855f7);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 5px;
        }
        .version { color: var(--muted); font-size: 0.9rem; }
        .card {
            background: var(--card);
            border-radius: 16px;
            padding: 24px;
            margin-bottom: 20px;
            border: 1px solid rgba(255,255,255,0.05);
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
        }
        .card-title {
            font-size: 1.1rem;
            font-weight: 600;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .status-badge {
            display: inline-block;
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 0.8rem;
            font-weight: 500;
        }
        .badge-idle { background: rgba(99,102,241,0.2); color: #a5b4fc; }
        .badge-checking { background: rgba(245,158,11,0.2); color: #fcd34d; }
        .badge-available { background: rgba(16,185,129,0.2); color: #6ee7b7; }
        .badge-error { background: rgba(239,68,68,0.2); color: #fca5a5; }
        .progress-container {
            background: rgba(0,0,0,0.3);
            border-radius: 10px;
            height: 24px;
            overflow: hidden;
            margin: 16px 0;
        }
        .progress-bar {
            height: 100%;
            background: linear-gradient(90deg, var(--primary), #a855f7);
            border-radius: 10px;
            transition: width 0.3s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 0.75rem;
            font-weight: 600;
        }
        .btn {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
            padding: 12px 24px;
            border: none;
            border-radius: 10px;
            font-size: 0.95rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            width: 100%;
            margin-bottom: 10px;
        }
        .btn:hover { transform: translateY(-2px); }
        .btn:active { transform: translateY(0); }
        .btn-primary { background: var(--primary); color: white; }
        .btn-primary:hover { background: var(--primary-dark); }
        .btn-success { background: var(--success); color: white; }
        .btn-warning { background: var(--warning); color: black; }
        .btn-danger { background: var(--danger); color: white; }
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
        }
        .info-item {
            background: rgba(0,0,0,0.2);
            padding: 12px;
            border-radius: 8px;
        }
        .info-label { color: var(--muted); font-size: 0.8rem; }
        .info-value { font-weight: 600; margin-top: 4px; }
        .update-info {
            background: rgba(16,185,129,0.1);
            border: 1px solid rgba(16,185,129,0.3);
            border-radius: 12px;
            padding: 16px;
            margin: 16px 0;
            display: none;
        }
        .upload-zone {
            border: 2px dashed rgba(255,255,255,0.2);
            border-radius: 12px;
            padding: 30px;
            text-align: center;
            transition: all 0.2s;
            cursor: pointer;
        }
        .upload-zone:hover { border-color: var(--primary); background: rgba(99,102,241,0.1); }
        .upload-zone.dragover { border-color: var(--success); background: rgba(16,185,129,0.1); }
        input[type="file"] { display: none; }
        .log {
            background: rgba(0,0,0,0.3);
            border-radius: 8px;
            padding: 12px;
            font-family: monospace;
            font-size: 0.8rem;
            max-height: 150px;
            overflow-y: auto;
            margin-top: 12px;
        }
        .log-entry { margin-bottom: 4px; color: var(--muted); }
        .log-entry.success { color: var(--success); }
        .log-entry.error { color: var(--danger); }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <div class="logo">Meantendo</div>
            <div class="version" id="version">v0.0.0</div>
        </div>
        
        <div class="card">
            <div class="card-title">
                ⚡ Firmware Status
                <span class="status-badge badge-idle" id="statusBadge">Idle</span>
            </div>
            <div class="info-grid">
                <div class="info-item">
                    <div class="info-label">Current Version</div>
                    <div class="info-value" id="currentVersion">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Partition</div>
                    <div class="info-value" id="partition">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">Free Space</div>
                    <div class="info-value" id="freeSpace">-</div>
                </div>
                <div class="info-item">
                    <div class="info-label">WiFi Signal</div>
                    <div class="info-value" id="rssi">-</div>
                </div>
            </div>
            <div class="update-info" id="updateInfo">
                <strong>🎉 Update Available!</strong>
                <div id="newVersion" style="margin-top:8px;"></div>
            </div>
            <div class="progress-container" id="progressContainer" style="display:none;">
                <div class="progress-bar" id="progressBar" style="width:0%;">0%</div>
            </div>
            <button class="btn btn-primary" onclick="checkUpdate()">🔍 Check for Updates</button>
            <button class="btn btn-success" id="updateBtn" onclick="startUpdate()" style="display:none;">
                ⬆️ Install Update
            </button>
        </div>
        
        <div class="card">
            <div class="card-title">📁 Manual Upload</div>
            <div class="upload-zone" id="uploadZone" onclick="document.getElementById('fileInput').click()">
                <div style="font-size:2rem;margin-bottom:10px;">📦</div>
                <div>Drop firmware.bin here or click to browse</div>
                <div style="color:var(--muted);font-size:0.8rem;margin-top:8px;">Max size: 3MB</div>
            </div>
            <input type="file" id="fileInput" accept=".bin" onchange="uploadFirmware(this.files[0])">
        </div>
        
        <div class="card">
            <div class="card-title">🔧 System Actions</div>
            <button class="btn btn-warning" onclick="rollback()">⏪ Rollback to Previous</button>
            <button class="btn btn-danger" onclick="reboot()">🔄 Reboot Device</button>
        </div>
        
        <div class="card">
            <div class="card-title">📋 Activity Log</div>
            <div class="log" id="log"></div>
        </div>
    </div>
    
    <script>
        let ws;
        const log = document.getElementById('log');
        
        function addLog(msg, type = '') {
            const entry = document.createElement('div');
            entry.className = 'log-entry ' + type;
            entry.textContent = new Date().toLocaleTimeString() + ' - ' + msg;
            log.insertBefore(entry, log.firstChild);
        }
        
        function connectWS() {
            ws = new WebSocket('ws://' + location.hostname + ':81');
            ws.onmessage = function(e) {
                const data = JSON.parse(e.data);
                if (data.type === 'progress') {
                    updateProgress(data.progress, data.status);
                }
            };
            ws.onclose = function() { setTimeout(connectWS, 2000); };
        }
        
        function updateProgress(pct, status) {
            const container = document.getElementById('progressContainer');
            const bar = document.getElementById('progressBar');
            container.style.display = 'block';
            bar.style.width = pct + '%';
            bar.textContent = pct + '%';
            document.getElementById('statusBadge').textContent = status;
        }
        
        async function fetchStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                document.getElementById('version').textContent = 'v' + data.version;
                document.getElementById('currentVersion').textContent = data.version;
                document.getElementById('partition').textContent = data.partition;
                document.getElementById('freeSpace').textContent = (data.freeSpace/1024).toFixed(0) + ' KB';
                document.getElementById('rssi').textContent = data.rssi + ' dBm';
            } catch(e) { addLog('Failed to fetch status', 'error'); }
        }
        
        async function checkUpdate() {
            addLog('Checking for updates...');
            document.getElementById('statusBadge').textContent = 'Checking...';
            document.getElementById('statusBadge').className = 'status-badge badge-checking';
            try {
                const res = await fetch('/api/check', { method: 'POST' });
                const data = await res.json();
                if (data.available) {
                    document.getElementById('updateInfo').style.display = 'block';
                    document.getElementById('newVersion').textContent = 'Version ' + data.version + ' is available!';
                    document.getElementById('updateBtn').style.display = 'block';
                    document.getElementById('statusBadge').textContent = 'Update Available';
                    document.getElementById('statusBadge').className = 'status-badge badge-available';
                    addLog('Update found: v' + data.version, 'success');
                } else {
                    document.getElementById('statusBadge').textContent = 'Up to Date';
                    document.getElementById('statusBadge').className = 'status-badge badge-idle';
                    addLog('Already on latest version');
                }
            } catch(e) {
                document.getElementById('statusBadge').textContent = 'Error';
                document.getElementById('statusBadge').className = 'status-badge badge-error';
                addLog('Check failed: ' + e.message, 'error');
            }
        }
        
        async function startUpdate() {
            if (!confirm('Install update now? Device will reboot.')) return;
            addLog('Starting update...');
            try {
                await fetch('/api/update', { method: 'POST' });
            } catch(e) { addLog('Update request failed', 'error'); }
        }
        
        async function rollback() {
            if (!confirm('Rollback to previous firmware?')) return;
            addLog('Initiating rollback...');
            try {
                await fetch('/api/rollback', { method: 'POST' });
            } catch(e) { addLog('Rollback failed', 'error'); }
        }
        
        async function reboot() {
            if (!confirm('Reboot device now?')) return;
            await fetch('/api/reboot', { method: 'POST' });
        }
        
        function uploadFirmware(file) {
            if (!file) return;
            if (!file.name.endsWith('.bin')) { addLog('Invalid file type', 'error'); return; }
            const formData = new FormData();
            formData.append('firmware', file);
            addLog('Uploading: ' + file.name);
            
            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/api/upload');
            xhr.upload.onprogress = function(e) {
                if (e.lengthComputable) updateProgress(Math.round(e.loaded/e.total*100), 'Uploading...');
            };
            xhr.onload = function() {
                if (xhr.status === 200) addLog('Upload complete!', 'success');
                else addLog('Upload failed: ' + xhr.statusText, 'error');
            };
            xhr.send(formData);
        }
        
        // Drag and drop
        const zone = document.getElementById('uploadZone');
        zone.ondragover = e => { e.preventDefault(); zone.classList.add('dragover'); };
        zone.ondragleave = () => zone.classList.remove('dragover');
        zone.ondrop = e => { e.preventDefault(); zone.classList.remove('dragover'); uploadFirmware(e.dataTransfer.files[0]); };
        
        // Init
        connectWS();
        fetchStatus();
        setInterval(fetchStatus, 10000);
        addLog('Web UI connected');
    </script>
</body>
</html>
)rawliteral";
    
    _server->send(200, "text/html", html);
}

void OTAWebUI::_handleStatus() {
    JsonDocument doc;
    doc["version"] = OTA.getVersionString();
    doc["partition"] = OTA.getPartitionInfo();
    doc["freeSpace"] = OTA.getFreeOTASpace();
    doc["rssi"] = WiFi.RSSI();
    doc["status"] = static_cast<int>(OTA.getStatus());
    doc["message"] = OTA.getStatusMessage();
    doc["canRollback"] = OTA.canRollback();
    
    String json;
    serializeJson(doc, json);
    _server->send(200, "application/json", json);
}

void OTAWebUI::_handleCheckUpdate() {
    OTAManifest manifest;
    bool available = OTA.checkForUpdate(manifest);
    
    JsonDocument doc;
    doc["available"] = available;
    if (available) {
        doc["version"] = manifest.version.toString();
        doc["size"] = manifest.firmwareSize;
        doc["notes"] = manifest.releaseNotes;
    }
    
    String json;
    serializeJson(doc, json);
    _server->send(200, "application/json", json);
}

void OTAWebUI::_handleStartUpdate() {
    if (OTA.startUpdate()) {
        _server->send(200, "application/json", "{\"success\":true}");
    } else {
        _server->send(400, "application/json", "{\"success\":false,\"error\":\"Update failed\"}");
    }
}

void OTAWebUI::_handleRollback() {
    if (OTA.rollback()) {
        _server->send(200, "application/json", "{\"success\":true}");
    } else {
        _server->send(400, "application/json", "{\"success\":false,\"error\":\"Rollback not available\"}");
    }
}

void OTAWebUI::_handleUpload() {
    HTTPUpload& upload = _server->upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[WebUI] Upload: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
        uint8_t progress = (Update.progress() * 100) / Update.size();
        broadcastProgress(progress, "Installing...");
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[WebUI] Upload success: %u bytes\n", upload.totalSize);
            broadcastProgress(100, "Complete!");
        } else {
            Update.printError(Serial);
        }
    }
}

void OTAWebUI::_handleUploadComplete() {
    if (Update.hasError()) {
        _server->send(400, "application/json", "{\"success\":false}");
    } else {
        _server->send(200, "application/json", "{\"success\":true}");
        delay(1000);
        ESP.restart();
    }
}

void OTAWebUI::_handleWiFiConfig() {
    _server->send(200, "application/json", "{\"ssid\":\"" + WiFi.SSID() + "\"}");
}

void OTAWebUI::_handleSaveWiFi() {
    String ssid = _server->arg("ssid");
    String pass = _server->arg("password");
    
    if (OTA.saveWiFiCredentials(ssid.c_str(), pass.c_str())) {
        _server->send(200, "application/json", "{\"success\":true}");
    } else {
        _server->send(400, "application/json", "{\"success\":false}");
    }
}

void OTAWebUI::_handleReboot() {
    _server->send(200, "application/json", "{\"success\":true}");
    delay(500);
    ESP.restart();
}

void OTAWebUI::_handleNotFound() {
    _server->send(404, "text/plain", "Not Found");
}

void OTAWebUI::_onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_CONNECTED) {
        Serial.printf("[WebUI] Client %u connected\n", num);
    } else if (type == WStype_DISCONNECTED) {
        Serial.printf("[WebUI] Client %u disconnected\n", num);
    }
}

// ============================================================================
//  Static Callback Wrappers
// ============================================================================

void OTAWebUI::_staticHandleRoot() { if (_instance) _instance->_handleRoot(); }
void OTAWebUI::_staticHandleStatus() { if (_instance) _instance->_handleStatus(); }
void OTAWebUI::_staticHandleCheckUpdate() { if (_instance) _instance->_handleCheckUpdate(); }
void OTAWebUI::_staticHandleStartUpdate() { if (_instance) _instance->_handleStartUpdate(); }
void OTAWebUI::_staticHandleRollback() { if (_instance) _instance->_handleRollback(); }
void OTAWebUI::_staticHandleUploadComplete() { if (_instance) _instance->_handleUploadComplete(); }
void OTAWebUI::_staticHandleWiFiConfig() { if (_instance) _instance->_handleWiFiConfig(); }
void OTAWebUI::_staticHandleSaveWiFi() { if (_instance) _instance->_handleSaveWiFi(); }
void OTAWebUI::_staticHandleReboot() { if (_instance) _instance->_handleReboot(); }
void OTAWebUI::_staticHandleNotFound() { if (_instance) _instance->_handleNotFound(); }
