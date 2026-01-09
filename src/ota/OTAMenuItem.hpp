/**
 * @file OTAMenuItem.hpp
 * @brief OTA Update Menu Integration for Meantendo Gaming Console
 * @author Debyte
 * @version 2.0.0
 * 
 * Provides in-game menu access to OTA updates with visual feedback
 * displayed on the ST7735 TFT screen.
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "../core/Game.hpp"
#include "OTAManager.hpp"
#include "OTAWebUI.hpp"

extern Adafruit_ST7735 tft;

// Color definitions for OTA UI
#define OTA_COLOR_BG          ST77XX_BLACK
#define OTA_COLOR_HEADER      tft.color565(99, 102, 241)   // Primary purple
#define OTA_COLOR_SUCCESS     tft.color565(16, 185, 129)   // Green
#define OTA_COLOR_WARNING     tft.color565(245, 158, 11)   // Orange
#define OTA_COLOR_ERROR       tft.color565(239, 68, 68)    // Red
#define OTA_COLOR_TEXT        ST77XX_WHITE
#define OTA_COLOR_MUTED       tft.color565(148, 163, 184)  // Slate gray

// ============================================================================
// OTA Menu State Machine
// ============================================================================

enum class OTAMenuState : uint8_t {
    MAIN_MENU = 0,
    CONNECTING_WIFI,
    CONNECTION_RESULT,
    CHECKING_UPDATE,
    UPDATE_AVAILABLE,
    NO_UPDATE,
    DOWNLOADING,
    INSTALLING,
    SUCCESS,
    ERROR,
    WIFI_CONFIG,
    WEB_UI_ACTIVE
};

// ============================================================================
// OTA Display Functions
// ============================================================================

namespace OTADisplay {

    inline void drawHeader(const char* title) {
        tft.fillRect(0, 0, tft.width(), 24, OTA_COLOR_HEADER);
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(1);
        int16_t x = (tft.width() - strlen(title) * 6) / 2;
        tft.setCursor(x, 8);
        tft.print(title);
    }
    
    inline void drawStatus(const char* status, uint16_t color) {
        tft.fillRect(0, 30, tft.width(), 20, OTA_COLOR_BG);
        tft.setTextColor(color);
        tft.setTextSize(1);
        int16_t x = (tft.width() - strlen(status) * 6) / 2;
        tft.setCursor(x, 35);
        tft.print(status);
    }
    
    inline void drawProgressBar(uint8_t percent) {
        const int16_t barX = 10;
        const int16_t barY = 60;
        const int16_t barW = tft.width() - 20;
        const int16_t barH = 16;
        
        // Background
        tft.fillRect(barX, barY, barW, barH, tft.color565(30, 41, 59));
        
        // Progress fill with gradient effect
        int16_t fillW = (barW * percent) / 100;
        if (fillW > 0) {
            // Draw gradient progress bar
            for (int16_t i = 0; i < fillW; i++) {
                uint8_t r = 99 + (i * 69 / barW);   // 99 to 168
                uint8_t g = 102 - (i * 30 / barW); // 102 to 72
                uint8_t b = 241 - (i * 70 / barW); // 241 to 171
                uint16_t color = tft.color565(r, g, b);
                tft.drawFastVLine(barX + i, barY, barH, color);
            }
        }
        
        // Border
        tft.drawRect(barX, barY, barW, barH, OTA_COLOR_MUTED);
        
        // Percentage text
        char percentStr[8];
        snprintf(percentStr, sizeof(percentStr), "%d%%", percent);
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(1);
        int16_t textX = (tft.width() - strlen(percentStr) * 6) / 2;
        tft.setCursor(textX, barY + 4);
        tft.print(percentStr);
    }
    
    inline void drawVersionInfo(const char* current, const char* available = nullptr) {
        tft.setTextColor(OTA_COLOR_MUTED);
        tft.setTextSize(1);
        
        tft.setCursor(10, 90);
        tft.print("Current: v");
        tft.print(current);
        
        if (available) {
            tft.setCursor(10, 105);
            tft.setTextColor(OTA_COLOR_SUCCESS);
            tft.print("New: v");
            tft.print(available);
        }
    }
    
    inline void drawButton(int16_t y, const char* label, bool selected) {
        int16_t x = 15;
        int16_t w = tft.width() - 30;
        int16_t h = 18;
        
        if (selected) {
            tft.fillRoundRect(x, y, w, h, 4, OTA_COLOR_HEADER);
            tft.setTextColor(ST77XX_WHITE);
        } else {
            tft.fillRoundRect(x, y, w, h, 4, tft.color565(30, 41, 59));
            tft.setTextColor(OTA_COLOR_MUTED);
        }
        
        tft.setTextSize(1);
        int16_t textX = (tft.width() - strlen(label) * 6) / 2;
        tft.setCursor(textX, y + 5);
        tft.print(label);
    }
    
    inline void drawWiFiInfo() {
        tft.setTextColor(OTA_COLOR_MUTED);
        tft.setTextSize(1);
        
        if (WiFi.status() == WL_CONNECTED) {
            tft.setCursor(10, tft.height() - 20);
            tft.print("IP: ");
            tft.print(WiFi.localIP().toString());
        } else {
            tft.setCursor(10, tft.height() - 20);
            tft.print("WiFi: Disconnected");
        }
    }
    
    inline void drawIcon(int16_t x, int16_t y, const char* icon) {
        tft.setTextSize(2);
        tft.setCursor(x, y);
        tft.print(icon);
    }
    
    inline void showSpinner(int16_t x, int16_t y) {
        static uint8_t frame = 0;
        static const char* spinFrames[] = {"|", "/", "-", "\\"};
        
        tft.fillRect(x, y, 12, 16, OTA_COLOR_BG);
        tft.setTextColor(OTA_COLOR_HEADER);
        tft.setTextSize(2);
        tft.setCursor(x, y);
        tft.print(spinFrames[frame % 4]);
        frame++;
    }
    
} // namespace OTADisplay

// ============================================================================
// OTA Menu Item (registered as a "game")
// ============================================================================

namespace OTAMenuItem {
    
    static OTAMenuState _currentState = OTAMenuState::MAIN_MENU;
    static int8_t _selectedOption = 0;
    static uint8_t _lastProgress = 0;
    static unsigned long _lastUpdate = 0;
    static bool _webUIStarted = false;
    
    // Progress callback for OTA updates
    static void onOTAProgress(OTAStatus status, uint8_t progress, const char* message) {
        if (progress != _lastProgress) {
            _lastProgress = progress;
            OTADisplay::drawProgressBar(progress);
            OTADisplay::drawStatus(message, OTA_COLOR_TEXT);
        }
    }
    
    static void drawMainMenu() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("OTA Update");
        
        OTADisplay::drawVersionInfo(OTA.getVersionString().c_str());
        
        const char* options[] = {
            "Check Updates",
            "Start Web UI",
            "WiFi Config",
            "Rollback",
            "Back"
        };
        const int numOptions = 5;
        
        for (int i = 0; i < numOptions; i++) {
            OTADisplay::drawButton(32 + i * 22, options[i], i == _selectedOption);
        }
        
        OTADisplay::drawWiFiInfo();
    }
    
    static void drawConnecting() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Connecting...");
        OTADisplay::showSpinner(tft.width()/2 - 6, 50);
        OTADisplay::drawStatus("Connecting to WiFi...", OTA_COLOR_WARNING);
    }
    
    static void drawChecking() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Checking Updates");
        OTADisplay::showSpinner(tft.width()/2 - 6, 50);
        OTADisplay::drawStatus("Fetching release info...", OTA_COLOR_TEXT);
    }
    
    static void drawUpdateAvailable() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Update Found!");
        
        const OTAManifest& update = OTA.getAvailableUpdate();
        OTADisplay::drawVersionInfo(
            OTA.getVersionString().c_str(),
            update.version.toString().c_str()
        );
        
        // Show firmware size
        tft.setTextColor(OTA_COLOR_MUTED);
        tft.setTextSize(1);
        tft.setCursor(10, 75);
        tft.printf("Size: %d KB", update.firmwareSize / 1024);
        
        OTADisplay::drawButton(95, "Install Now", _selectedOption == 0);
        OTADisplay::drawButton(117, "Later", _selectedOption == 1);
    }
    
    static void drawNoUpdate() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Up to Date");
        
        tft.setTextColor(OTA_COLOR_SUCCESS);
        tft.setTextSize(2);
        tft.setCursor(tft.width()/2 - 6, 45);
        tft.print("OK");
        
        OTADisplay::drawStatus("You have the latest version", OTA_COLOR_TEXT);
        OTADisplay::drawVersionInfo(OTA.getVersionString().c_str());
        
        OTADisplay::drawButton(100, "Back", true);
    }
    
    static void drawDownloading() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Downloading...");
        OTADisplay::drawProgressBar(_lastProgress);
        OTADisplay::drawStatus("Downloading firmware...", OTA_COLOR_WARNING);
    }
    
    static void drawInstalling() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Installing...");
        OTADisplay::drawProgressBar(_lastProgress);
        OTADisplay::drawStatus("Flashing firmware...", OTA_COLOR_WARNING);
        
        tft.setTextColor(OTA_COLOR_ERROR);
        tft.setTextSize(1);
        tft.setCursor(10, 90);
        tft.print("DO NOT POWER OFF!");
    }
    
    static void drawWebUIActive() {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Web UI Active");
        
        tft.setTextColor(OTA_COLOR_SUCCESS);
        tft.setTextSize(1);
        tft.setCursor(10, 35);
        tft.print("Connect to:");
        
        tft.setTextColor(ST77XX_WHITE);
        tft.setTextSize(2);
        tft.setCursor(10, 50);
        if (WiFi.status() == WL_CONNECTED) {
            tft.print(WiFi.localIP().toString());
        } else {
            tft.print(WiFi.softAPIP().toString());
        }
        
        tft.setTextColor(OTA_COLOR_MUTED);
        tft.setTextSize(1);
        tft.setCursor(10, 75);
        tft.print("Use any web browser");
        tft.setCursor(10, 88);
        tft.print("to manage updates");
        
        OTADisplay::drawButton(110, "Stop & Back", true);
    }
    
    static void drawError(const char* message) {
        tft.fillScreen(OTA_COLOR_BG);
        OTADisplay::drawHeader("Error");
        
        tft.setTextColor(OTA_COLOR_ERROR);
        tft.setTextSize(2);
        tft.setCursor(tft.width()/2 - 6, 45);
        tft.print("!");
        
        OTADisplay::drawStatus(message, OTA_COLOR_ERROR);
        OTADisplay::drawButton(100, "Back", true);
    }
    
    // ========================================================================
    // Menu Item Implementation
    // ========================================================================
    
    inline void start() {
        _currentState = OTAMenuState::MAIN_MENU;
        _selectedOption = 0;
        _lastProgress = 0;
        _webUIStarted = false;
        
        // Initialize OTA manager
        OTAConfig config;
        config.progressCallback = onOTAProgress;
        OTA.begin(config);
        
        drawMainMenu();
    }
    
    inline void loop() {
        // Handle state-specific logic
        switch (_currentState) {
            case OTAMenuState::CONNECTING_WIFI:
                if (millis() - _lastUpdate > 200) {
                    _lastUpdate = millis();
                    OTADisplay::showSpinner(tft.width()/2 - 6, 50);
                }
                
                if (OTA.isWiFiConnected()) {
                    _currentState = OTAMenuState::CHECKING_UPDATE;
                    drawChecking();
                } else if (millis() - _lastUpdate > OTA_WIFI_TIMEOUT_MS) {
                    _currentState = OTAMenuState::ERROR;
                    drawError("WiFi Failed");
                }
                break;
                
            case OTAMenuState::CHECKING_UPDATE: {
                OTAManifest manifest;
                if (OTA.checkForUpdate(manifest)) {
                    _currentState = OTAMenuState::UPDATE_AVAILABLE;
                    _selectedOption = 0;
                    drawUpdateAvailable();
                } else {
                    _currentState = OTAMenuState::NO_UPDATE;
                    drawNoUpdate();
                }
                break;
            }
            
            case OTAMenuState::DOWNLOADING:
            case OTAMenuState::INSTALLING:
                // Progress is handled by callback
                if (OTA.getStatus() == OTAStatus::ERROR_DOWNLOAD ||
                    OTA.getStatus() == OTAStatus::ERROR_FLASH) {
                    _currentState = OTAMenuState::ERROR;
                    drawError(OTA.getStatusMessage());
                }
                break;
                
            case OTAMenuState::WEB_UI_ACTIVE:
                WebUI.loop();
                OTA.loop();
                break;
                
            default:
                break;
        }
    }
    
    // Menu navigation handled externally
    inline void handleInput(int8_t direction, bool select) {
        switch (_currentState) {
            case OTAMenuState::MAIN_MENU:
                if (direction != 0) {
                    _selectedOption = constrain(_selectedOption + direction, 0, 4);
                    drawMainMenu();
                }
                
                if (select) {
                    switch (_selectedOption) {
                        case 0: // Check Updates
                            _currentState = OTAMenuState::CONNECTING_WIFI;
                            _lastUpdate = millis();
                            drawConnecting();
                            OTA.connectWiFi();
                            break;
                            
                        case 1: // Start Web UI
                            if (!OTA.isWiFiConnected()) {
                                if (!OTA.connectWiFi()) {
                                    OTA.startAPMode();
                                }
                            }
                            WebUI.begin();
                            _webUIStarted = true;
                            _currentState = OTAMenuState::WEB_UI_ACTIVE;
                            drawWebUIActive();
                            break;
                            
                        case 2: // WiFi Config
                            OTA.startAPMode();
                            WebUI.begin();
                            _webUIStarted = true;
                            _currentState = OTAMenuState::WEB_UI_ACTIVE;
                            drawWebUIActive();
                            break;
                            
                        case 3: // Rollback
                            if (OTA.canRollback()) {
                                OTA.rollback();
                            } else {
                                _currentState = OTAMenuState::ERROR;
                                drawError("No rollback avail");
                            }
                            break;
                            
                        case 4: // Back - exit handled by caller
                            break;
                    }
                }
                break;
                
            case OTAMenuState::UPDATE_AVAILABLE:
                if (direction != 0) {
                    _selectedOption = constrain(_selectedOption + direction, 0, 1);
                    drawUpdateAvailable();
                }
                
                if (select) {
                    if (_selectedOption == 0) {
                        // Install now
                        _currentState = OTAMenuState::DOWNLOADING;
                        _lastProgress = 0;
                        drawDownloading();
                        OTA.startUpdate();
                    } else {
                        // Later
                        _currentState = OTAMenuState::MAIN_MENU;
                        _selectedOption = 0;
                        drawMainMenu();
                    }
                }
                break;
                
            case OTAMenuState::NO_UPDATE:
            case OTAMenuState::ERROR:
                if (select) {
                    _currentState = OTAMenuState::MAIN_MENU;
                    _selectedOption = 0;
                    drawMainMenu();
                }
                break;
                
            case OTAMenuState::WEB_UI_ACTIVE:
                if (select) {
                    if (_webUIStarted) {
                        WebUI.end();
                        _webUIStarted = false;
                    }
                    OTA.disconnectWiFi();
                    _currentState = OTAMenuState::MAIN_MENU;
                    _selectedOption = 0;
                    drawMainMenu();
                }
                break;
                
            default:
                break;
        }
    }
    
    inline bool shouldExit() {
        return _currentState == OTAMenuState::MAIN_MENU && _selectedOption == 4;
    }
    
} // namespace OTAMenuItem

// ============================================================================
// Register as a GameDef for the menu system
// ============================================================================

static GameDef otaUpdateGame = {
    .name = "OTA Update",
    .start = OTAMenuItem::start,
    .loop = OTAMenuItem::loop
};

#endif // MEANTENDO_OTA_MENU_ITEM_HPP
