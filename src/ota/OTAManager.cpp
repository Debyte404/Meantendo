/**
 * @file OTAManager.cpp
 * @brief Implementation of the Hyper-Advanced OTA Manager
 * @author Debyte
 * @version 2.0.0
 */

#include "OTAManager.hpp"
#include <mbedtls/sha256.h>
#include <esp_app_format.h>

// ============================================================================
//  Constructor / Destructor
// ============================================================================

OTAManager::OTAManager() 
    : _initialized(false)
    , _status(OTAStatus::IDLE)
    , _updateAvailable(false)
    , _progress(0)
    , _wifiConnected(false)
    , _apModeActive(false)
    , _lastWiFiAttempt(0)
    , _asyncCheckPending(false)
    , _asyncCheckStartTime(0)
    , _asyncTaskHandle(nullptr) {
    
    // Initialize current version from compile-time constants
    _currentVersion.major = MEANTENDO_VERSION_MAJOR;
    _currentVersion.minor = MEANTENDO_VERSION_MINOR;
    _currentVersion.patch = MEANTENDO_VERSION_PATCH;
    
    memset(&_availableUpdate, 0, sizeof(_availableUpdate));
    memset(&_stats, 0, sizeof(_stats));
    memset(_statusMessage, 0, sizeof(_statusMessage));
    strcpy(_statusMessage, "Ready");
}

OTAManager::~OTAManager() {
    end();
}

// ============================================================================
//  Initialization
// ============================================================================

bool OTAManager::begin(const OTAConfig& config) {
    if (_initialized) return true;
    
    _config = config;
    
    // Initialize NVS preferences
    if (!_prefs.begin(OTA_NVS_NAMESPACE, false)) {
        Serial.println(F("[OTA] Failed to initialize NVS"));
        return false;
    }
    
    // Load stored stats
    _loadStats();
    
    // Save current version to NVS if first boot
    SemanticVersion storedVer = getStoredVersion();
    if (storedVer.major == 0 && storedVer.minor == 0 && storedVer.patch == 0) {
        saveCurrentVersion();
    }
    
    // Check if this is first boot after OTA
    const esp_partition_t* running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            Serial.println(F("[OTA] First boot after update - verifying..."));
            // Mark as valid after successful boot
            // User should call markFirmwareValid() after confirming stability
        }
    }
    
    _updateStatus(OTAStatus::IDLE, "OTA Manager initialized");
    _initialized = true;
    
    Serial.printf("[OTA] Initialized - Version: %s\n", getVersionString().c_str());
    Serial.printf("[OTA] Running partition: %s\n", running ? running->label : "unknown");
    
    return true;
}

void OTAManager::end() {
    if (!_initialized) return;
    
    // Abort any ongoing update
    if (_status == OTAStatus::DOWNLOADING || _status == OTAStatus::INSTALLING) {
        abortUpdate();
    }
    
    // Disconnect WiFi if we connected it
    if (_wifiConnected) {
        disconnectWiFi();
    }
    
    // Save stats
    _saveStats();
    
    // Close preferences
    _prefs.end();
    
    _initialized = false;
}

// ============================================================================
//  WiFi Management
// ============================================================================

bool OTAManager::connectWiFi(const char* ssid, const char* password, uint32_t timeoutMs) {
    // If already connected, return success
    if (WiFi.status() == WL_CONNECTED) {
        _wifiConnected = true;
        return true;
    }
    
    // Rate limit connection attempts
    if (millis() - _lastWiFiAttempt < 5000) {
        return false;
    }
    _lastWiFiAttempt = millis();
    
    // Get credentials from NVS if not provided
    String wifiSSID, wifiPass;
    if (ssid == nullptr || strlen(ssid) == 0) {
        wifiSSID = _prefs.getString(OTA_NVS_KEY_WIFI_SSID, OTA_DEFAULT_WIFI_SSID);
        if (wifiSSID.isEmpty()) {
            Serial.println(F("[OTA] No WiFi SSID configured"));
            _updateStatus(OTAStatus::ERROR_WIFI, "No WiFi credentials");
            return false;
        }
    } else {
        wifiSSID = ssid;
    }
    
    if (password == nullptr || strlen(password) == 0) {
        wifiPass = _prefs.getString(OTA_NVS_KEY_WIFI_PASS, OTA_DEFAULT_WIFI_PASS);
    } else {
        wifiPass = password;
    }
    
    Serial.printf("[OTA] Connecting to WiFi: %s\n", wifiSSID.c_str());
    _updateStatus(OTAStatus::CHECKING, "Connecting to WiFi...");
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeoutMs) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        _wifiConnected = true;
        Serial.printf("[OTA] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        _updateStatus(OTAStatus::IDLE, "WiFi connected");
        return true;
    }
    
    Serial.println(F("[OTA] WiFi connection failed"));
    _updateStatus(OTAStatus::ERROR_WIFI, "WiFi connection failed");
    return false;
}

void OTAManager::disconnectWiFi() {
    WiFi.disconnect(true);
    _wifiConnected = false;
}

bool OTAManager::isWiFiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool OTAManager::startAPMode() {
    Serial.println(F("[OTA] Starting Access Point mode..."));
    
    WiFi.mode(WIFI_AP);
    bool success = WiFi.softAP(OTA_AP_MODE_SSID, OTA_AP_MODE_PASS);
    
    if (success) {
        _apModeActive = true;
        Serial.printf("[OTA] AP Started: %s\n", OTA_AP_MODE_SSID);
        Serial.printf("[OTA] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    }
    
    return success;
}

bool OTAManager::saveWiFiCredentials(const char* ssid, const char* password) {
    if (!_initialized) return false;
    
    _prefs.putString(OTA_NVS_KEY_WIFI_SSID, ssid);
    _prefs.putString(OTA_NVS_KEY_WIFI_PASS, password);
    
    Serial.printf("[OTA] WiFi credentials saved for: %s\n", ssid);
    return true;
}

// ============================================================================
//  Version Management
// ============================================================================

SemanticVersion OTAManager::getCurrentVersion() const {
    return _currentVersion;
}

String OTAManager::getVersionString() const {
    return _currentVersion.toString();
}

bool OTAManager::saveCurrentVersion() {
    if (!_initialized) return false;
    
    _prefs.putUChar("ver_major", _currentVersion.major);
    _prefs.putUChar("ver_minor", _currentVersion.minor);
    _prefs.putUChar("ver_patch", _currentVersion.patch);
    _prefs.putString(OTA_NVS_KEY_VERSION, _currentVersion.toString());
    _prefs.putString(OTA_NVS_KEY_BUILD_TIME, MEANTENDO_BUILD_TIMESTAMP);
    
    return true;
}

SemanticVersion OTAManager::getStoredVersion() const {
    SemanticVersion ver;
    
    ver.major = _prefs.getUChar("ver_major", 0);
    ver.minor = _prefs.getUChar("ver_minor", 0);
    ver.patch = _prefs.getUChar("ver_patch", 0);
    
    return ver;
}

// ============================================================================
//  Update Checking
// ============================================================================

bool OTAManager::checkForUpdateAsync() {
    if (_asyncCheckPending) return false;
    if (_status == OTAStatus::DOWNLOADING || _status == OTAStatus::INSTALLING) return false;
    
    _asyncCheckPending = true;
    _asyncCheckStartTime = millis();
    _updateStatus(OTAStatus::CHECKING, "Checking for updates...");
    
    // For simplicity, we'll use blocking check in async context
    // In production, use FreeRTOS task
    return true;
}

OTAStatus OTAManager::pollUpdateCheckStatus() {
    if (!_asyncCheckPending) return _status;
    
    // Timeout check
    if (millis() - _asyncCheckStartTime > _config.timeoutMs) {
        _asyncCheckPending = false;
        _updateStatus(OTAStatus::ERROR_TIMEOUT, "Update check timed out");
        return _status;
    }
    
    // Perform actual check
    if (checkForUpdate(_availableUpdate)) {
        _updateAvailable = true;
        _asyncCheckPending = false;
    } else {
        _asyncCheckPending = false;
    }
    
    return _status;
}

bool OTAManager::checkForUpdate(OTAManifest& manifest) {
    if (!isWiFiConnected()) {
        if (!connectWiFi()) {
            return false;
        }
    }
    
    _updateStatus(OTAStatus::CHECKING, "Fetching release info...");
    
    // Use secure client for HTTPS
    WiFiClientSecure client;
    client.setInsecure();  // TODO: Add proper CA validation
    
    HTTPClient http;
    
    // Determine URL based on source
    String url;
    if (_config.source == OTASource::GITHUB_RELEASE) {
        url = OTA_GITHUB_API_URL;
    } else {
        url = String(_config.manifestUrl);
    }
    
    Serial.printf("[OTA] Checking: %s\n", url.c_str());
    
    http.begin(client, url);
    http.addHeader("Accept", "application/vnd.github.v3+json");
    http.addHeader("User-Agent", "Meantendo-OTA/2.0");
    http.setTimeout(_config.timeoutMs);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] HTTP Error: %d\n", httpCode);
        _updateStatus(OTAStatus::ERROR_SERVER, "Server error");
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // Parse response based on source type
    bool parsed = false;
    if (_config.source == OTASource::GITHUB_RELEASE) {
        parsed = _parseGitHubRelease(payload, manifest);
    } else {
        parsed = _parseManifestJson(payload, manifest);
    }
    
    if (!parsed) {
        _updateStatus(OTAStatus::ERROR_SERVER, "Failed to parse response");
        return false;
    }
    
    // Compare versions
    if (manifest.version > _currentVersion) {
        _updateAvailable = true;
        _availableUpdate = manifest;
        _updateStatus(OTAStatus::UPDATE_AVAILABLE, "Update available!");
        
        Serial.printf("[OTA] Update available: %s -> %s\n", 
                     _currentVersion.toString().c_str(),
                     manifest.version.toString().c_str());
        return true;
    }
    
    _updateStatus(OTAStatus::IDLE, "Firmware is up to date");
    Serial.println(F("[OTA] Already on latest version"));
    return false;
}

bool OTAManager::isUpdateAvailable() const {
    return _updateAvailable;
}

const OTAManifest& OTAManager::getAvailableUpdate() const {
    return _availableUpdate;
}

// ============================================================================
//  Update Execution
// ============================================================================

bool OTAManager::startUpdate() {
    if (!_updateAvailable) {
        _updateStatus(OTAStatus::ERROR_DOWNLOAD, "No update available");
        return false;
    }
    
    return startUpdateFromURL(_availableUpdate.downloadUrl);
}

bool OTAManager::startUpdateFromURL(const char* url) {
    if (!isWiFiConnected()) {
        _updateStatus(OTAStatus::ERROR_WIFI, "Not connected to WiFi");
        return false;
    }
    
    Serial.printf("[OTA] Starting update from: %s\n", url);
    _updateStatus(OTAStatus::DOWNLOADING, "Downloading firmware...");
    _progress = 0;
    
    WiFiClientSecure client;
    client.setInsecure();  // TODO: Add proper CA validation
    
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("User-Agent", "Meantendo-OTA/2.0");
    http.setTimeout(_config.timeoutMs);
    
    // Follow redirects (GitHub uses them)
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OTA] Download failed: %d\n", httpCode);
        _updateStatus(OTAStatus::ERROR_DOWNLOAD, "Download failed");
        http.end();
        return false;
    }
    
    int contentLength = http.getSize();
    Serial.printf("[OTA] Content length: %d bytes\n", contentLength);
    
    if (contentLength <= 0) {
        _updateStatus(OTAStatus::ERROR_DOWNLOAD, "Invalid content length");
        http.end();
        return false;
    }
    
    // Check if we have enough space
    if (!Update.begin(contentLength)) {
        Serial.printf("[OTA] Not enough space: %d required\n", contentLength);
        _updateStatus(OTAStatus::ERROR_NO_SPACE, "Not enough space");
        http.end();
        return false;
    }
    
    _updateStatus(OTAStatus::INSTALLING, "Installing firmware...");
    
    // Stream firmware to Update
    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    size_t written = 0;
    
    while (http.connected() && written < contentLength) {
        size_t available = stream->available();
        if (available > 0) {
            size_t toRead = min(available, sizeof(buffer));
            size_t bytesRead = stream->readBytes(buffer, toRead);
            
            if (bytesRead > 0) {
                if (Update.write(buffer, bytesRead) != bytesRead) {
                    Serial.println(F("[OTA] Write error"));
                    _updateStatus(OTAStatus::ERROR_FLASH, "Flash write error");
                    Update.abort();
                    http.end();
                    return false;
                }
                
                written += bytesRead;
                _progress = (written * 100) / contentLength;
                _notifyProgress(_progress, "Installing...");
            }
        }
        yield();
    }
    
    http.end();
    
    if (written != contentLength) {
        Serial.printf("[OTA] Incomplete: %d/%d\n", written, contentLength);
        _updateStatus(OTAStatus::ERROR_DOWNLOAD, "Download incomplete");
        Update.abort();
        return false;
    }
    
    // Finalize update
    if (!Update.end(true)) {
        Serial.printf("[OTA] Update failed: %s\n", Update.errorString());
        _updateStatus(OTAStatus::ERROR_FLASH, Update.errorString());
        return false;
    }
    
    // Update stats
    _stats.totalUpdates++;
    _stats.successfulUpdates++;
    _stats.lastUpdateTimestamp = millis() / 1000;
    _saveStats();
    
    _progress = 100;
    _updateStatus(OTAStatus::SUCCESS, "Update successful! Rebooting...");
    Serial.println(F("[OTA] Update successful! Rebooting..."));
    
    delay(1000);
    ESP.restart();
    
    return true;  // Won't reach here
}

bool OTAManager::startUpdateFromData(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        _updateStatus(OTAStatus::ERROR_VERIFY, "Invalid firmware data");
        return false;
    }
    
    Serial.printf("[OTA] Installing from data: %d bytes\n", length);
    _updateStatus(OTAStatus::INSTALLING, "Installing firmware...");
    _progress = 0;
    
    if (!Update.begin(length)) {
        _updateStatus(OTAStatus::ERROR_NO_SPACE, "Not enough space");
        return false;
    }
    
    // Write data in chunks
    size_t written = 0;
    const size_t chunkSize = 4096;
    
    while (written < length) {
        size_t toWrite = min(chunkSize, length - written);
        
        if (Update.write(data + written, toWrite) != toWrite) {
            _updateStatus(OTAStatus::ERROR_FLASH, "Flash write error");
            Update.abort();
            return false;
        }
        
        written += toWrite;
        _progress = (written * 100) / length;
        _notifyProgress(_progress, "Installing...");
        yield();
    }
    
    if (!Update.end(true)) {
        _updateStatus(OTAStatus::ERROR_FLASH, Update.errorString());
        return false;
    }
    
    // Update stats
    _stats.totalUpdates++;
    _stats.successfulUpdates++;
    _stats.lastUpdateTimestamp = millis() / 1000;
    _saveStats();
    
    _progress = 100;
    _updateStatus(OTAStatus::SUCCESS, "Update successful! Rebooting...");
    
    delay(1000);
    ESP.restart();
    
    return true;
}

void OTAManager::abortUpdate() {
    if (_status == OTAStatus::DOWNLOADING || _status == OTAStatus::INSTALLING) {
        Update.abort();
        _stats.failedUpdates++;
        _saveStats();
    }
    
    _updateStatus(OTAStatus::ERROR_ABORTED, "Update aborted");
    _progress = 0;
}

uint8_t OTAManager::getProgress() const {
    return _progress;
}

OTAStatus OTAManager::getStatus() const {
    return _status;
}

const char* OTAManager::getStatusMessage() const {
    return _statusMessage;
}

// ============================================================================
//  Rollback Management
// ============================================================================

bool OTAManager::canRollback() const {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(running);
    
    if (!next) return false;
    
    // Check if other partition has valid app
    esp_app_desc_t app_desc;
    if (esp_ota_get_partition_description(next, &app_desc) != ESP_OK) {
        return false;
    }
    
    return true;
}

bool OTAManager::markFirmwareValid() {
    esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
    if (err == ESP_OK) {
        Serial.println(F("[OTA] Firmware marked as valid"));
        return true;
    }
    Serial.printf("[OTA] Failed to mark valid: %d\n", err);
    return false;
}

bool OTAManager::rollback() {
    if (!canRollback()) {
        _updateStatus(OTAStatus::ERROR_ROLLBACK, "Rollback not available");
        return false;
    }
    
    Serial.println(F("[OTA] Initiating rollback..."));
    _updateStatus(OTAStatus::REBOOTING, "Rolling back...");
    
    _stats.rollbackCount++;
    _saveStats();
    
    esp_err_t err = esp_ota_mark_app_invalid_rollback_and_reboot();
    
    // If we get here, rollback failed
    _updateStatus(OTAStatus::ERROR_ROLLBACK, "Rollback failed");
    return false;
}

String OTAManager::getRollbackInfo() const {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(running);
    
    String info = "Running: ";
    info += running ? running->label : "unknown";
    info += " | Backup: ";
    
    if (next) {
        esp_app_desc_t app_desc;
        if (esp_ota_get_partition_description(next, &app_desc) == ESP_OK) {
            info += next->label;
            info += " (v";
            info += app_desc.version;
            info += ")";
        } else {
            info += "empty";
        }
    } else {
        info += "none";
    }
    
    return info;
}

// ============================================================================
//  Configuration
// ============================================================================

void OTAManager::setConfig(const OTAConfig& config) {
    _config = config;
}

const OTAConfig& OTAManager::getConfig() const {
    return _config;
}

void OTAManager::setProgressCallback(OTAProgressCallback callback) {
    _config.progressCallback = callback;
}

void OTAManager::setPolicy(OTAPolicy policy) {
    _config.policy = policy;
    _prefs.putUChar(OTA_NVS_KEY_AUTO_UPDATE, static_cast<uint8_t>(policy));
}

// ============================================================================
//  Statistics & Diagnostics
// ============================================================================

OTAManager::UpdateStats OTAManager::getStats() const {
    return _stats;
}

String OTAManager::getPartitionInfo() const {
    const esp_partition_t* running = esp_ota_get_running_partition();
    
    String info = "Partition: ";
    info += running ? running->label : "unknown";
    info += " | Address: 0x";
    info += String(running ? running->address : 0, HEX);
    info += " | Size: ";
    info += String(running ? running->size / 1024 : 0);
    info += "KB";
    
    return info;
}

size_t OTAManager::getFreeOTASpace() const {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(running);
    
    return next ? next->size : 0;
}

// ============================================================================
//  Main Loop
// ============================================================================

void OTAManager::loop() {
    if (!_initialized) return;
    
    // Handle async check
    if (_asyncCheckPending) {
        pollUpdateCheckStatus();
    }
    
    // Auto-check based on policy
    if (_config.policy == OTAPolicy::SCHEDULED || _config.policy == OTAPolicy::AUTO) {
        static unsigned long lastAutoCheck = 0;
        if (millis() - lastAutoCheck > _config.checkIntervalMs) {
            lastAutoCheck = millis();
            if (isWiFiConnected()) {
                checkForUpdateAsync();
            }
        }
    }
}

// ============================================================================
//  Private Methods
// ============================================================================

bool OTAManager::_parseGitHubRelease(const String& json, OTAManifest& manifest) {
    // Parse GitHub Releases API response
    JsonDocument doc;
    
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.printf("[OTA] JSON parse error: %s\n", error.c_str());
        return false;
    }
    
    // Extract tag name (version)
    const char* tagName = doc["tag_name"] | "";
    String versionStr = tagName;
    
    // Remove 'v' prefix if present
    if (versionStr.startsWith("v") || versionStr.startsWith("V")) {
        versionStr = versionStr.substring(1);
    }
    
    if (!manifest.version.parse(versionStr)) {
        Serial.printf("[OTA] Failed to parse version: %s\n", tagName);
        return false;
    }
    
    // Get release notes
    const char* body = doc["body"] | "";
    strncpy(manifest.releaseNotes, body, sizeof(manifest.releaseNotes) - 1);
    
    // Get release date
    const char* publishedAt = doc["published_at"] | "";
    strncpy(manifest.releaseDate, publishedAt, sizeof(manifest.releaseDate) - 1);
    
    // Find firmware asset
    JsonArray assets = doc["assets"].as<JsonArray>();
    bool foundAsset = false;
    
    for (JsonObject asset : assets) {
        const char* name = asset["name"] | "";
        String assetName = name;
        
        // Look for .bin file
        if (assetName.endsWith(".bin")) {
            const char* downloadUrl = asset["browser_download_url"] | "";
            strncpy(manifest.downloadUrl, downloadUrl, sizeof(manifest.downloadUrl) - 1);
            strncpy(manifest.assetName, name, sizeof(manifest.assetName) - 1);
            manifest.firmwareSize = asset["size"] | 0;
            foundAsset = true;
            break;
        }
    }
    
    if (!foundAsset) {
        Serial.println(F("[OTA] No firmware asset found in release"));
        return false;
    }
    
    manifest.priority = OTAUpdatePriority::NORMAL;
    manifest.isDelta = false;
    
    Serial.printf("[OTA] Found release: v%s, Asset: %s (%d bytes)\n",
                 manifest.version.toString().c_str(),
                 manifest.assetName,
                 manifest.firmwareSize);
    
    return true;
}

bool OTAManager::_parseManifestJson(const String& json, OTAManifest& manifest) {
    JsonDocument doc;
    
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return false;
    }
    
    // Parse custom manifest format
    const char* version = doc["version"] | "";
    manifest.version.parse(version);
    
    const char* url = doc["url"] | "";
    strncpy(manifest.downloadUrl, url, sizeof(manifest.downloadUrl) - 1);
    
    const char* hash = doc["sha256"] | "";
    strncpy(manifest.sha256Hash, hash, sizeof(manifest.sha256Hash) - 1);
    
    manifest.firmwareSize = doc["size"] | 0;
    manifest.priority = static_cast<OTAUpdatePriority>(doc["priority"] | 2);
    manifest.isDelta = doc["delta"] | false;
    
    return true;
}

void OTAManager::_updateStatus(OTAStatus status, const char* message) {
    _status = status;
    if (message) {
        strncpy(_statusMessage, message, sizeof(_statusMessage) - 1);
    }
    
    // Also notify via callback
    if (_config.progressCallback) {
        _config.progressCallback(status, _progress, _statusMessage);
    }
}

void OTAManager::_loadStats() {
    _stats.totalUpdates = _prefs.getUInt("stat_total", 0);
    _stats.successfulUpdates = _prefs.getUInt("stat_success", 0);
    _stats.failedUpdates = _prefs.getUInt("stat_failed", 0);
    _stats.rollbackCount = _prefs.getUInt("stat_rollback", 0);
    _stats.lastUpdateTimestamp = _prefs.getUInt("stat_last_update", 0);
    _stats.lastCheckTimestamp = _prefs.getUInt("stat_last_check", 0);
}

void OTAManager::_saveStats() {
    _prefs.putUInt("stat_total", _stats.totalUpdates);
    _prefs.putUInt("stat_success", _stats.successfulUpdates);
    _prefs.putUInt("stat_failed", _stats.failedUpdates);
    _prefs.putUInt("stat_rollback", _stats.rollbackCount);
    _prefs.putUInt("stat_last_update", _stats.lastUpdateTimestamp);
    _prefs.putUInt("stat_last_check", millis() / 1000);
}

void OTAManager::_notifyProgress(uint8_t progress, const char* message) {
    if (_config.progressCallback) {
        _config.progressCallback(_status, progress, message);
    }
    
    // Also print to serial
    static uint8_t lastReported = 0;
    if (progress - lastReported >= 10 || progress == 100) {
        Serial.printf("[OTA] Progress: %d%%\n", progress);
        lastReported = progress;
    }
}
