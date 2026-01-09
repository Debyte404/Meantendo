/**
 * @file OTAManager.hpp
 * @brief Hyper-Advanced OTA Manager for Meantendo Gaming Console
 * @author Debyte
 * @version 2.0.0
 * 
 * Core OTA Manager with support for:
 * - GitHub Releases integration
 * - Semantic versioning with NVS persistence
 * - Non-blocking update checks
 * - A/B partition rollback
 * - Progress callbacks for UI updates
 */

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include "OTAConfig.hpp"

// Forward declaration for web UI
class OTAWebUI;

/**
 * @class OTAManager
 * @brief Singleton class managing all OTA update operations
 */
class OTAManager {
public:
    // ========================================================================
    //  Singleton Pattern
    // ========================================================================
    static OTAManager& getInstance() {
        static OTAManager instance;
        return instance;
    }
    
    // Delete copy constructor and assignment
    OTAManager(const OTAManager&) = delete;
    OTAManager& operator=(const OTAManager&) = delete;
    
    // ========================================================================
    //  Initialization
    // ========================================================================
    
    /**
     * @brief Initialize the OTA Manager
     * @param config Configuration parameters
     * @return true on success
     */
    bool begin(const OTAConfig& config = OTAConfig());
    
    /**
     * @brief Deinitialize and cleanup
     */
    void end();
    
    // ========================================================================
    //  WiFi Management
    // ========================================================================
    
    /**
     * @brief Connect to WiFi using stored or provided credentials
     * @param ssid WiFi SSID (nullptr = use NVS)
     * @param password WiFi password (nullptr = use NVS)
     * @param timeoutMs Connection timeout
     * @return true on successful connection
     */
    bool connectWiFi(const char* ssid = nullptr, const char* password = nullptr, 
                     uint32_t timeoutMs = OTA_WIFI_TIMEOUT_MS);
    
    /**
     * @brief Disconnect from WiFi
     */
    void disconnectWiFi();
    
    /**
     * @brief Check if WiFi is connected
     */
    bool isWiFiConnected() const;
    
    /**
     * @brief Start WiFi Access Point for configuration
     * @return true on success
     */
    bool startAPMode();
    
    /**
     * @brief Save WiFi credentials to NVS
     */
    bool saveWiFiCredentials(const char* ssid, const char* password);
    
    // ========================================================================
    //  Version Management
    // ========================================================================
    
    /**
     * @brief Get current firmware version
     */
    SemanticVersion getCurrentVersion() const;
    
    /**
     * @brief Get version string
     */
    String getVersionString() const;
    
    /**
     * @brief Save current version to NVS
     */
    bool saveCurrentVersion();
    
    /**
     * @brief Get stored version from NVS
     */
    SemanticVersion getStoredVersion() const;
    
    // ========================================================================
    //  Update Checking (Non-blocking)
    // ========================================================================
    
    /**
     * @brief Start asynchronous update check
     * @return true if check started
     */
    bool checkForUpdateAsync();
    
    /**
     * @brief Poll async update check status
     * @return Current status
     */
    OTAStatus pollUpdateCheckStatus();
    
    /**
     * @brief Synchronous update check (blocking)
     * @param manifest Output manifest if available
     * @return true if update is available
     */
    bool checkForUpdate(OTAManifest& manifest);
    
    /**
     * @brief Check if an update is available
     */
    bool isUpdateAvailable() const;
    
    /**
     * @brief Get the available update manifest
     */
    const OTAManifest& getAvailableUpdate() const;
    
    // ========================================================================
    //  Update Execution
    // ========================================================================
    
    /**
     * @brief Start the OTA update process
     * @return true if update started
     */
    bool startUpdate();
    
    /**
     * @brief Start update from a specific URL
     * @param url Firmware download URL
     * @return true if update started
     */
    bool startUpdateFromURL(const char* url);
    
    /**
     * @brief Start update from uploaded file data
     * @param data Firmware binary data
     * @param length Data length
     * @return true if update started
     */
    bool startUpdateFromData(const uint8_t* data, size_t length);
    
    /**
     * @brief Abort an ongoing update
     */
    void abortUpdate();
    
    /**
     * @brief Get current update progress (0-100)
     */
    uint8_t getProgress() const;
    
    /**
     * @brief Get current OTA status
     */
    OTAStatus getStatus() const;
    
    /**
     * @brief Get status message
     */
    const char* getStatusMessage() const;
    
    // ========================================================================
    //  Rollback Management
    // ========================================================================
    
    /**
     * @brief Check if rollback is available
     */
    bool canRollback() const;
    
    /**
     * @brief Mark current firmware as valid (prevents auto-rollback)
     */
    bool markFirmwareValid();
    
    /**
     * @brief Trigger rollback to previous firmware
     * @return true if rollback initiated
     */
    bool rollback();
    
    /**
     * @brief Get rollback partition info
     */
    String getRollbackInfo() const;
    
    // ========================================================================
    //  Configuration
    // ========================================================================
    
    /**
     * @brief Update OTA configuration
     */
    void setConfig(const OTAConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const OTAConfig& getConfig() const;
    
    /**
     * @brief Set progress callback
     */
    void setProgressCallback(OTAProgressCallback callback);
    
    /**
     * @brief Set update policy
     */
    void setPolicy(OTAPolicy policy);
    
    // ========================================================================
    //  Statistics & Diagnostics
    // ========================================================================
    
    /**
     * @brief Get update statistics
     */
    struct UpdateStats {
        uint32_t totalUpdates;
        uint32_t successfulUpdates;
        uint32_t failedUpdates;
        uint32_t rollbackCount;
        uint32_t lastUpdateTimestamp;
        uint32_t lastCheckTimestamp;
        char lastError[64];
    };
    
    UpdateStats getStats() const;
    
    /**
     * @brief Get partition information
     */
    String getPartitionInfo() const;
    
    /**
     * @brief Get free space for OTA
     */
    size_t getFreeOTASpace() const;
    
    // ========================================================================
    //  Task Handler (call from main loop)
    // ========================================================================
    
    /**
     * @brief Process OTA tasks (call periodically)
     */
    void loop();
    
private:
    // Private constructor for singleton
    OTAManager();
    ~OTAManager();
    
    // Internal state
    bool _initialized;
    OTAConfig _config;
    OTAStatus _status;
    OTAManifest _availableUpdate;
    bool _updateAvailable;
    uint8_t _progress;
    char _statusMessage[128];
    Preferences _prefs;
    UpdateStats _stats;
    
    // WiFi state
    bool _wifiConnected;
    bool _apModeActive;
    unsigned long _lastWiFiAttempt;
    
    // Async check state
    bool _asyncCheckPending;
    unsigned long _asyncCheckStartTime;
    TaskHandle_t _asyncTaskHandle;
    
    // Current version (compiled)
    SemanticVersion _currentVersion;
    
    // Internal methods
    bool _parseGitHubRelease(const String& json, OTAManifest& manifest);
    bool _parseManifestJson(const String& json, OTAManifest& manifest);
    bool _downloadFirmware(const char* url, size_t expectedSize);
    bool _verifyFirmware(const uint8_t* data, size_t length, const char* expectedHash);
    bool _verifySignature(const uint8_t* data, size_t length, const char* signature);
    bool _flashFirmware(const uint8_t* data, size_t length);
    void _updateStatus(OTAStatus status, const char* message = nullptr);
    void _loadStats();
    void _saveStats();
    void _notifyProgress(uint8_t progress, const char* message);
    
    // Friend class for Web UI access
    friend class OTAWebUI;
};

// Global accessor macro
#define OTA OTAManager::getInstance()

#endif // MEANTENDO_OTA_MANAGER_HPP
