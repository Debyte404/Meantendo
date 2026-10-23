#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

namespace CloudUpdater {

struct ScannedAP {
    String ssid;
    int32_t rssi;
    bool encrypted;
};

enum ConnectStatus {
    CONN_SUCCESS = 0,
    CONN_ERR_WRONG_PASSWORD = 1,
    CONN_ERR_NOT_FOUND = 2,
    CONN_ERR_TIMEOUT = 3,
    CONN_ERR_UNKNOWN = 4
};

// Initialization & NVS
void init();
String getSavedSSID();
String getSavedPassword();
void saveCredentials(const String& ssid, const String& pass);
bool isAutoUpdateEnabled();
void setAutoUpdateEnabled(bool enabled);

// Wi-Fi Management
void powerOffWifi();
void startScan();
bool isScanComplete();
int getScanCount();
ScannedAP getScannedNetwork(int index);

ConnectStatus connect(const String& ssid, const String& pass, int timeoutSec = 15);
bool isConnected();
String getLocalIP();
void disconnectAndRestoreEspNow();

// GitHub Release & OTA Update
struct ReleaseInfo {
    bool found;
    String tagName;
    String assetName;
    String downloadUrl;
    size_t assetSize;
    String releaseNotes;
    bool isNewer;
};

bool checkLatestRelease(ReleaseInfo& outInfo);
bool isVersionNewer(const String& remoteTag, const String& currentVersion);

typedef void (*OtaProgressCallback)(int percent, size_t bytesDownloaded, size_t totalBytes);
bool performOtaUpdate(const String& downloadUrl, size_t expectedSize, OtaProgressCallback progressCb);
String getLastOtaError();

} // namespace CloudUpdater
