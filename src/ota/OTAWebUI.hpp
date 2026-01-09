/**
 * @file OTAWebUI.hpp
 * @brief Web-based OTA Management Interface for Meantendo Gaming Console
 * @author Debyte
 * @version 2.0.0
 */

#ifndef MEANTENDO_OTA_WEBUI_HPP
#define MEANTENDO_OTA_WEBUI_HPP

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "OTAManager.hpp"

class OTAManager;

class OTAWebUI {
public:
    static OTAWebUI& getInstance();
    
    OTAWebUI(const OTAWebUI&) = delete;
    OTAWebUI& operator=(const OTAWebUI&) = delete;
    
    bool begin(uint16_t port = OTA_WEB_UI_PORT);
    void end();
    void loop();
    bool isRunning() const;
    void broadcastProgress(uint8_t progress, const char* status);
    void broadcastStatus(const char* jsonStatus);
    
private:
    OTAWebUI();
    ~OTAWebUI();
    
    bool _running;
    WebServer* _server;
    WebSocketsServer* _webSocket;
    
    void _handleRoot();
    void _handleStatus();
    void _handleCheckUpdate();
    void _handleStartUpdate();
    void _handleRollback();
    void _handleUpload();
    void _handleUploadComplete();
    void _handleWiFiConfig();
    void _handleSaveWiFi();
    void _handleReboot();
    void _handleNotFound();
    
    void _onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    
    static void _staticHandleRoot();
    static void _staticHandleStatus();
    static void _staticHandleCheckUpdate();
    static void _staticHandleStartUpdate();
    static void _staticHandleRollback();
    static void _staticHandleUploadComplete();
    static void _staticHandleWiFiConfig();
    static void _staticHandleSaveWiFi();
    static void _staticHandleReboot();
    static void _staticHandleNotFound();
};

#define WebUI OTAWebUI::getInstance()

#endif // MEANTENDO_OTA_WEBUI_HPP
