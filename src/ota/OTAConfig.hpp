/**
 * @file OTAConfig.hpp
 * @brief Hyper-Advanced OTA Configuration for Meantendo Gaming Console
 * @author Debyte
 * @version 2.0.0
 * 
 * Configuration constants and types for the OTA update system.
 * Supports HTTPS, GitHub Releases, delta updates, and secure boot.
 */

#pragma once
#include <Arduino.h>

// ============================================================================
//  VERSION MANAGEMENT
// ============================================================================
#define MEANTENDO_VERSION_MAJOR     1
#define MEANTENDO_VERSION_MINOR     0
#define MEANTENDO_VERSION_PATCH     0
#define MEANTENDO_VERSION_STRING    "1.0.0"
#define MEANTENDO_BUILD_TIMESTAMP   __DATE__ " " __TIME__

// ============================================================================
//  OTA SERVER CONFIGURATION
// ============================================================================

// GitHub Release URL (Primary source)
#define OTA_GITHUB_OWNER            "Debyte404"
#define OTA_GITHUB_REPO             "Meantendo"
#define OTA_GITHUB_API_URL          "https://api.github.com/repos/" OTA_GITHUB_OWNER "/" OTA_GITHUB_REPO "/releases/latest"

// Custom OTA Server (Fallback)
#define OTA_CUSTOM_SERVER_URL       "https://ota.meantendo.local"
#define OTA_MANIFEST_ENDPOINT       "/api/manifest.json"
#define OTA_FIRMWARE_ENDPOINT       "/api/firmware"

// HTTPS Settings
#define OTA_USE_HTTPS               true
#define OTA_TIMEOUT_MS              30000
#define OTA_RETRY_COUNT             3
#define OTA_RETRY_DELAY_MS          5000

// ============================================================================
//  WIFI CONFIGURATION (User-configurable via NVS)
// ============================================================================
#define OTA_DEFAULT_WIFI_SSID       ""      // Empty = use NVS
#define OTA_DEFAULT_WIFI_PASS       ""      // Empty = use NVS
#define OTA_AP_MODE_SSID            "Meantendo-Setup"
#define OTA_AP_MODE_PASS            "meantendo123"
#define OTA_WIFI_TIMEOUT_MS         15000
#define OTA_AP_PORTAL_TIMEOUT_MS    180000  // 3 minutes

// ============================================================================
//  NVS NAMESPACE AND KEYS
// ============================================================================
#define OTA_NVS_NAMESPACE           "meantendo_ota"
#define OTA_NVS_KEY_VERSION         "fw_version"
#define OTA_NVS_KEY_BUILD_TIME      "build_time"
#define OTA_NVS_KEY_UPDATE_COUNT    "update_cnt"
#define OTA_NVS_KEY_LAST_CHECK      "last_check"
#define OTA_NVS_KEY_ROLLBACK_VER    "rollback_ver"
#define OTA_NVS_KEY_WIFI_SSID       "wifi_ssid"
#define OTA_NVS_KEY_WIFI_PASS       "wifi_pass"
#define OTA_NVS_KEY_AUTO_UPDATE     "auto_update"
#define OTA_NVS_KEY_CHECK_INTERVAL  "check_intv"

// ============================================================================
//  WEB UI CONFIGURATION
// ============================================================================
#define OTA_WEB_UI_PORT             80
#define OTA_WEBSOCKET_PORT          81
#define OTA_WEB_UI_ENABLED          true
#define OTA_MAX_UPLOAD_SIZE         (3 * 1024 * 1024)  // 3MB max firmware

// ============================================================================
//  SECURITY CONFIGURATION
// ============================================================================
#define OTA_VERIFY_SIGNATURE        true
#define OTA_ENABLE_ROLLBACK         true
#define OTA_ROLLBACK_TIMEOUT_MS     30000
#define OTA_HASH_ALGORITHM          "SHA256"

// Public key for firmware signature verification (ECDSA P-256)
// Replace with your actual public key in production
static const char OTA_PUBLIC_KEY[] PROGMEM = R"(
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAExBLJ1PWjDBLt4P4xF0ypz/MQ3aeV
aIWb5EYO8lJ8jF1nN4rw0N0MndzjqF5qxN3QBCqMvqxVB3uD3jhMOyFlQQ==
-----END PUBLIC KEY-----
)";

// Root CA Certificate for HTTPS (GitHub API)
static const char OTA_ROOT_CA[] PROGMEM = R"(
-----BEGIN CERTIFICATE-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQC3...
-----END CERTIFICATE-----
)";

// ============================================================================
//  UPDATE POLICY CONFIGURATION
// ============================================================================

/**
 * @brief OTA Update Source Types
 */
enum class OTASource : uint8_t {
    GITHUB_RELEASE = 0,    // Primary: GitHub Releases API
    HTTPS_SERVER   = 1,    // Secondary: Custom HTTPS server
    HTTP_SERVER    = 2,    // Fallback: HTTP (insecure, dev only)
    SD_CARD        = 3,    // Manual: SD card update
    WEB_UPLOAD     = 4,    // Manual: Web interface upload
    SERIAL_UPLOAD  = 5     // Manual: Serial (ArduinoOTA style)
};

/**
 * @brief OTA Update Policies
 */
enum class OTAPolicy : uint8_t {
    AUTO           = 0,    // Automatic updates when available
    MANUAL         = 1,    // User must explicitly trigger
    SCHEDULED      = 2,    // Check at specific intervals
    WIFI_ONLY      = 3,    // Only update when on WiFi
    BATTERY_SAFE   = 4,    // Only update when battery > threshold
    DEVELOPMENT    = 5     // Accept all updates including pre-release
};

/**
 * @brief OTA Update Priority Levels
 */
enum class OTAUpdatePriority : uint8_t {
    CRITICAL       = 0,    // Security fix - force update
    HIGH           = 1,    // Important - prompt immediately
    NORMAL         = 2,    // Regular update - notify user
    LOW            = 3,    // Minor update - optional
    PRERELEASE     = 4     // Beta/Alpha - dev only
};

/**
 * @brief OTA Update Status Codes
 */
enum class OTAStatus : uint8_t {
    IDLE                = 0,
    CHECKING            = 1,
    UPDATE_AVAILABLE    = 2,
    DOWNLOADING         = 3,
    VERIFYING           = 4,
    INSTALLING          = 5,
    REBOOTING           = 6,
    SUCCESS             = 7,
    ERROR_WIFI          = 100,
    ERROR_NETWORK       = 101,
    ERROR_SERVER        = 102,
    ERROR_DOWNLOAD      = 103,
    ERROR_VERIFY        = 104,
    ERROR_FLASH         = 105,
    ERROR_SIGNATURE     = 106,
    ERROR_NO_SPACE      = 107,
    ERROR_ROLLBACK      = 108,
    ERROR_TIMEOUT       = 109,
    ERROR_ABORTED       = 110
};

/**
 * @brief Structure to hold version information
 */
struct SemanticVersion {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    char prerelease[16];   // e.g., "beta.1", "rc.2"
    char buildMeta[16];    // e.g., "20260109"
    
    // Constructor
    SemanticVersion() : major(0), minor(0), patch(0) {
        prerelease[0] = '\0';
        buildMeta[0] = '\0';
    }
    
    SemanticVersion(uint8_t maj, uint8_t min, uint8_t pat) 
        : major(maj), minor(min), patch(pat) {
        prerelease[0] = '\0';
        buildMeta[0] = '\0';
    }
    
    // Compare versions (returns: -1 if less, 0 if equal, 1 if greater)
    int compare(const SemanticVersion& other) const {
        if (major != other.major) return major > other.major ? 1 : -1;
        if (minor != other.minor) return minor > other.minor ? 1 : -1;
        if (patch != other.patch) return patch > other.patch ? 1 : -1;
        return 0;  // Equal (ignore prerelease for now)
    }
    
    bool operator>(const SemanticVersion& other) const { return compare(other) > 0; }
    bool operator<(const SemanticVersion& other) const { return compare(other) < 0; }
    bool operator==(const SemanticVersion& other) const { return compare(other) == 0; }
    bool operator>=(const SemanticVersion& other) const { return compare(other) >= 0; }
    bool operator<=(const SemanticVersion& other) const { return compare(other) <= 0; }
    
    // Convert to string
    String toString() const {
        String result = String(major) + "." + String(minor) + "." + String(patch);
        if (prerelease[0] != '\0') {
            result += "-" + String(prerelease);
        }
        if (buildMeta[0] != '\0') {
            result += "+" + String(buildMeta);
        }
        return result;
    }
    
    // Parse from string (e.g., "1.2.3-beta.1+20260109")
    bool parse(const String& versionStr) {
        int majorEnd = versionStr.indexOf('.');
        if (majorEnd < 0) return false;
        
        int minorEnd = versionStr.indexOf('.', majorEnd + 1);
        if (minorEnd < 0) return false;
        
        // Find prerelease and build metadata markers
        int prereleaseStart = versionStr.indexOf('-', minorEnd);
        int buildMetaStart = versionStr.indexOf('+', minorEnd);
        
        int patchEnd = versionStr.length();
        if (prereleaseStart > 0) patchEnd = prereleaseStart;
        else if (buildMetaStart > 0) patchEnd = buildMetaStart;
        
        major = versionStr.substring(0, majorEnd).toInt();
        minor = versionStr.substring(majorEnd + 1, minorEnd).toInt();
        patch = versionStr.substring(minorEnd + 1, patchEnd).toInt();
        
        // Parse prerelease
        if (prereleaseStart > 0) {
            int preEnd = buildMetaStart > 0 ? buildMetaStart : versionStr.length();
            strncpy(prerelease, versionStr.substring(prereleaseStart + 1, preEnd).c_str(), sizeof(prerelease) - 1);
            prerelease[sizeof(prerelease) - 1] = '\0';
        }
        
        // Parse build metadata
        if (buildMetaStart > 0) {
            strncpy(buildMeta, versionStr.substring(buildMetaStart + 1).c_str(), sizeof(buildMeta) - 1);
            buildMeta[sizeof(buildMeta) - 1] = '\0';
        }
        
        return true;
    }
};

/**
 * @brief OTA Update Manifest (parsed from JSON)
 */
struct OTAManifest {
    SemanticVersion version;
    char downloadUrl[256];
    char sha256Hash[65];
    char signature[128];
    uint32_t firmwareSize;
    OTAUpdatePriority priority;
    char releaseNotes[512];
    char releaseDate[24];
    char assetName[64];
    bool isDelta;
    char baseVersion[16];      // For delta updates
};

/**
 * @brief OTA Progress Callback Function Type
 */
typedef void (*OTAProgressCallback)(OTAStatus status, uint8_t progress, const char* message);

/**
 * @brief OTA Configuration Structure
 */
struct OTAConfig {
    OTASource source;
    OTAPolicy policy;
    char serverUrl[128];
    char manifestUrl[256];
    uint32_t timeoutMs;
    uint8_t retryCount;
    bool validateSignature;
    bool enableRollback;
    bool enableWebUI;
    uint16_t webUIPort;
    uint32_t checkIntervalMs;  // Auto-check interval
    OTAProgressCallback progressCallback;
    
    // Default configuration
    OTAConfig() {
        source = OTASource::GITHUB_RELEASE;
        policy = OTAPolicy::MANUAL;
        strncpy(serverUrl, OTA_GITHUB_API_URL, sizeof(serverUrl));
        strncpy(manifestUrl, OTA_GITHUB_API_URL, sizeof(manifestUrl));
        timeoutMs = OTA_TIMEOUT_MS;
        retryCount = OTA_RETRY_COUNT;
        validateSignature = OTA_VERIFY_SIGNATURE;
        enableRollback = OTA_ENABLE_ROLLBACK;
        enableWebUI = OTA_WEB_UI_ENABLED;
        webUIPort = OTA_WEB_UI_PORT;
        checkIntervalMs = 3600000;  // 1 hour
        progressCallback = nullptr;
    }
};

#endif // MEANTENDO_OTA_CONFIG_HPP
