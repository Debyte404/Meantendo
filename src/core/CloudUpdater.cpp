#include "CloudUpdater.hpp"
#include "Config.hpp"
#include "Wireless.hpp"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>

namespace CloudUpdater {

static Preferences prefs;
static bool initialized = false;
static std::vector<ScannedAP> scanResults;
static bool scanInProgress = false;

void init() {
    if (initialized) return;
    prefs.begin("meantendo_net", false);
    initialized = true;
}

String getSavedSSID() {
    init();
    return prefs.getString("ssid", "");
}

String getSavedPassword() {
    init();
    return prefs.getString("pass", "");
}

void saveCredentials(const String& ssid, const String& pass) {
    init();
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
}

bool isAutoUpdateEnabled() {
    init();
    return prefs.getBool("autoupdate", true);
}

void setAutoUpdateEnabled(bool enabled) {
    init();
    prefs.putBool("autoupdate", enabled);
}

void powerOffWifi() {
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    scanInProgress = false;
    Serial.println("[WIFI] Radio powered OFF (Battery Conservation)");
}

void startScan() {
    scanResults.clear();

    // Release ESP-NOW raw MAC hook so STA scan works cleanly
    esp_now_deinit();
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(50);

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    scanInProgress = true;
    WiFi.scanNetworks(true); // Asynchronous scan
    Serial.println("[WIFI] Asynchronous RF scan started...");
}

bool isScanComplete() {
    int16_t status = WiFi.scanComplete();
    if (status >= 0) {
        scanResults.clear();
        for (int i = 0; i < status; i++) {
            ScannedAP ap;
            ap.ssid = WiFi.SSID(i);
            ap.rssi = WiFi.RSSI(i);
            ap.encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            if (ap.ssid.length() > 0) {
                scanResults.push_back(ap);
            }
        }
        WiFi.scanDelete();
        scanInProgress = false;

        // Battery optimization: put radio into modem sleep while user selects from list
        WiFi.setSleep(true);
        Serial.printf("[WIFI] Scan finished: %d APs found. Radio put to sleep.\n", (int)scanResults.size());
        return true;
    }
    return false;
}

int getScanCount() {
    return (int)scanResults.size();
}

ScannedAP getScannedNetwork(int index) {
    if (index >= 0 && index < (int)scanResults.size()) {
        return scanResults[index];
    }
    return ScannedAP{"", 0, false};
}

ConnectStatus connect(const String& ssid, const String& pass, int timeoutSec) {
    Serial.printf("[WIFI] Initiating connection to: '%s' (Pass Length: %d)\n", ssid.c_str(), pass.length());

    // 1. Release ESP-NOW and cancel any ongoing scans
    esp_now_deinit();
    if (WiFi.scanComplete() < 0) {
        WiFi.scanDelete();
    }

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    delay(80);

    // 2. Turn on STA and set moderate TX power to prevent battery voltage brownout
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_17dBm); // ~17 dBm saves 50mA peak draw on 18650 cell
    delay(40);

    if (pass.length() > 0) {
        WiFi.begin(ssid.c_str(), pass.c_str());
    } else {
        WiFi.begin(ssid.c_str());
    }

    unsigned long start = millis();
    wl_status_t status = WiFi.status();

    while (status != WL_CONNECTED && (millis() - start) < (unsigned long)(timeoutSec * 1000)) {
        delay(250);
        status = WiFi.status();
        Serial.printf("[WIFI] Status: %d\n", status);

        // Early exit on explicit authentication failure
        if (status == WL_CONNECT_FAILED) {
            Serial.println("[WIFI] Authentication failed: Router rejected password!");
            powerOffWifi();
            return CONN_ERR_WRONG_PASSWORD;
        } else if (status == WL_NO_SSID_AVAIL) {
            Serial.println("[WIFI] SSID not found in range!");
            powerOffWifi();
            return CONN_ERR_NOT_FOUND;
        }
    }

    if (status == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected! IP: %s | RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        saveCredentials(ssid, pass);
        return CONN_SUCCESS;
    } else {
        Serial.printf("[WIFI] Connection failed (Final Status Code: %d). Powering radio off.\n", status);
        powerOffWifi();
        if (status == WL_CONNECT_FAILED) return CONN_ERR_WRONG_PASSWORD;
        if (status == WL_NO_SSID_AVAIL) return CONN_ERR_NOT_FOUND;
        return CONN_ERR_TIMEOUT;
    }
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String getLocalIP() {
    return WiFi.localIP().toString();
}

void disconnectAndRestoreEspNow() {
    powerOffWifi();
    delay(50);
    MeantendoWireless::init();
}

bool isVersionNewer(const String& remoteTag, const String& currentVersion) {
    auto cleanVersion = [](const String& v) -> String {
        String s = v;
        s.trim();
        if (s.startsWith("v") || s.startsWith("V")) s = s.substring(1);
        return s;
    };

    String r = cleanVersion(remoteTag);
    String c = cleanVersion(currentVersion);

    int rMajor = 0, rMinor = 0, rPatch = 0;
    int cMajor = 0, cMinor = 0, cPatch = 0;

    sscanf(r.c_str(), "%d.%d.%d", &rMajor, &rMinor, &rPatch);
    sscanf(c.c_str(), "%d.%d.%d", &cMajor, &cMinor, &cPatch);

    if (rMajor != cMajor) return rMajor > cMajor;
    if (rMinor != cMinor) return rMinor > cMinor;
    return rPatch > cPatch;
}

bool checkLatestRelease(ReleaseInfo& outInfo) {
    outInfo.found = false;
    outInfo.isNewer = false;
    outInfo.assetSize = 0;

    if (!isConnected()) {
        Serial.println("[OTA] Error: WiFi not connected");
        return false;
    }

    WiFiClientSecure client;
    client.setInsecure(); // Skip TLS verification for GitHub

    HTTPClient http;
    http.setTimeout(10000);

    String apiUrl = String("https://api.github.com/repos/") + MEANTENDO_GITHUB_OWNER + "/" + MEANTENDO_GITHUB_REPO + "/releases/latest";
    Serial.printf("[OTA] Checking GitHub: %s\n", apiUrl.c_str());

    http.begin(client, apiUrl);
    http.setUserAgent("Meantendo-Console-OTA");
    http.addHeader("Accept", "application/vnd.github.v3+json");

    int httpCode = http.GET();
    Serial.printf("[OTA] API HTTP Response: %d\n", httpCode);

    if (httpCode == 200) {
        WiFiClient* stream = http.getStreamPtr();
        String jsonPayload = "";
        
        unsigned long start = millis();
        while (http.connected() && (millis() - start < 8000)) {
            while (stream->available()) {
                char c = stream->read();
                jsonPayload += c;
                if (jsonPayload.length() > 16384) break;
            }
            if (jsonPayload.length() > 16384) break;
            delay(10);
        }
        http.end();

        int tagIdx = jsonPayload.indexOf("\"tag_name\":");
        if (tagIdx != -1) {
            int q1 = jsonPayload.indexOf("\"", tagIdx + 11);
            int q2 = jsonPayload.indexOf("\"", q1 + 1);
            if (q1 != -1 && q2 != -1) {
                outInfo.tagName = jsonPayload.substring(q1 + 1, q2);
                outInfo.found = true;
            }
        }

        int binIdx = jsonPayload.indexOf(MEANTENDO_ASSET_NAME);
        if (binIdx != -1) {
            int urlIdx = jsonPayload.lastIndexOf("\"browser_download_url\":", binIdx);
            if (urlIdx != -1) {
                int q1 = jsonPayload.indexOf("\"", urlIdx + 23);
                int q2 = jsonPayload.indexOf("\"", q1 + 1);
                if (q1 != -1 && q2 != -1) {
                    outInfo.downloadUrl = jsonPayload.substring(q1 + 1, q2);
                }
            }
        }

        if (outInfo.downloadUrl.length() == 0 && outInfo.found) {
            outInfo.downloadUrl = String("https://github.com/") + MEANTENDO_GITHUB_OWNER + "/" + MEANTENDO_GITHUB_REPO + "/releases/download/" + outInfo.tagName + "/" + MEANTENDO_ASSET_NAME;
        }

        outInfo.assetName = MEANTENDO_ASSET_NAME;
        outInfo.isNewer = isVersionNewer(outInfo.tagName, MEANTENDO_VERSION);
        return true;
    } else if (httpCode == 404) {
        http.end();
        Serial.println("[OTA] No published releases found on repo yet.");
        return false;
    }

    http.end();

    // Fallback: Check direct redirect URL on github.com
    String directUrl = String("https://github.com/") + MEANTENDO_GITHUB_OWNER + "/" + MEANTENDO_GITHUB_REPO + "/releases/latest";
    http.begin(client, directUrl);
    http.setUserAgent("Meantendo-Console-OTA");
    const char* headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);

    int directCode = http.GET();
    if (directCode == 302 || directCode == 301) {
        String loc = http.header("Location");
        int tagPos = loc.lastIndexOf("/tag/");
        if (tagPos != -1) {
            outInfo.tagName = loc.substring(tagPos + 5);
            outInfo.found = true;
            outInfo.downloadUrl = String("https://github.com/") + MEANTENDO_GITHUB_OWNER + "/" + MEANTENDO_GITHUB_REPO + "/releases/latest/download/" + MEANTENDO_ASSET_NAME;
            outInfo.assetName = MEANTENDO_ASSET_NAME;
            outInfo.isNewer = isVersionNewer(outInfo.tagName, MEANTENDO_VERSION);
            http.end();
            return true;
        }
    }
    http.end();

    return false;
}

static String lastOtaErrorMsg = "";

String getLastOtaError() {
    return lastOtaErrorMsg;
}

bool performOtaUpdate(const String& downloadUrl, size_t expectedSize, OtaProgressCallback progressCb) {
    lastOtaErrorMsg = "";
    if (!isConnected()) {
        lastOtaErrorMsg = "WiFi disconnected";
        Serial.println("[OTA] Error: WiFi disconnected during update");
        return false;
    }

    Serial.printf("[OTA] Initializing OTA stream from: %s\n", downloadUrl.c_str());

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(30000);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.setReuse(true);

    if (!http.begin(client, downloadUrl)) {
        lastOtaErrorMsg = "HTTP.begin failed";
        Serial.println("[OTA] HTTP.begin failed");
        return false;
    }

    http.setUserAgent("Meantendo-Console-OTA");

    int httpCode = http.GET();
    Serial.printf("[OTA] GET response: %d\n", httpCode);

    if (httpCode != 200) {
        lastOtaErrorMsg = "HTTP " + String(httpCode);
        Serial.printf("[OTA] Download failed: HTTP %d\n", httpCode);
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    Serial.printf("[OTA] Content-Length: %d bytes\n", contentLength);

    if (contentLength <= 0) {
        lastOtaErrorMsg = "Zero Content-Length";
        http.end();
        return false;
    }

    if (contentLength > 1310720) {
        lastOtaErrorMsg = "Size " + String(contentLength) + " > 1.25MB";
        Serial.printf("[OTA] Firmware binary too large: %d bytes\n", contentLength);
        http.end();
        return false;
    }

    if (!Update.begin(contentLength)) {
        lastOtaErrorMsg = "Update.begin err " + String(Update.getError());
        Serial.printf("[OTA] Update.begin failed: %s\n", lastOtaErrorMsg.c_str());
        http.end();
        return false;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buff[1024];
    size_t writtenTotal = 0;
    int lastPercent = -1;
    unsigned long lastDataTime = millis();

    while (http.connected() && (writtenTotal < (size_t)contentLength)) {
        size_t availableBytes = stream->available();
        if (availableBytes > 0) {
            lastDataTime = millis();
            size_t bytesToRead = availableBytes < sizeof(buff) ? availableBytes : sizeof(buff);
            int readBytes = stream->read(buff, bytesToRead);
            if (readBytes > 0) {
                size_t written = Update.write(buff, readBytes);
                if (written != (size_t)readBytes) {
                    lastOtaErrorMsg = "Flash write err " + String(Update.getError());
                    Update.abort();
                    http.end();
                    return false;
                }
                writtenTotal += readBytes;

                int percent = (int)((writtenTotal * 100) / contentLength);
                if (percent != lastPercent) {
                    lastPercent = percent;
                    if (progressCb) {
                        progressCb(percent, writtenTotal, contentLength);
                    }
                }
            }
        } else {
            if (millis() - lastDataTime > 20000) {
                lastOtaErrorMsg = "Stream timeout";
                Update.abort();
                http.end();
                return false;
            }
            delay(2);
        }
        yield();
    }

    http.end();

    if (writtenTotal != (size_t)contentLength) {
        lastOtaErrorMsg = "Incomplete (" + String(writtenTotal) + "/" + String(contentLength) + ")";
        Update.abort();
        return false;
    }

    if (!Update.end(true)) {
        lastOtaErrorMsg = "Update.end err " + String(Update.getError());
        return false;
    }

    Serial.println("[OTA] Update successfully verified and installed!");
    return true;
}

} // namespace CloudUpdater
