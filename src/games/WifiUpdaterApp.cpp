#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include "../core/Buzzer.hpp"
#include "../core/CloudUpdater.hpp"
#include "../core/Config.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  Meantendo Full-Width Wi-Fi Manager & GitHub Cloud Updater
//  Screen 0: Network Selector (Full width, zero overlap)
//  Screen 1: Full-Width 10-Col QWERTY On-Screen TV Keyboard
// ═══════════════════════════════════════════════════════════

enum WifiAppState {
    STATE_NETWORKS_LIST,
    STATE_KEYBOARD_ENTRY,
    STATE_CONNECTING,
    STATE_CONNECTED_CHECK,
    STATE_UPDATE_PROMPT,
    STATE_FLASHING_OTA
};

static WifiAppState appState = STATE_NETWORKS_LIST;
static int selectedNetIdx = 0;
static int listScrollOffset = 0;
static const int VISIBLE_NETWORKS = 4;

static String enteredPassword = "";
static String targetSSID = "";
static String statusMessage = "Press Y to Rescan";
static uint16_t statusColor = ST77XX_YELLOW;

static bool shiftActive = true; // true = UPPERCASE, false = lowercase
static int kbCol = 0;
static int kbRow = 0;

static CloudUpdater::ReleaseInfo releaseInfo;

// ── 10-Column QWERTY Layout ──
static const char qwertyUpper[4][10] = {
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
    {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
    {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '_'},
    {'Z', 'X', 'C', 'V', 'B', 'N', 'M', '-', '.', '@'}
};

static const char qwertyLower[4][10] = {
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p'},
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', '_'},
    {'z', 'x', 'c', 'v', 'b', 'n', 'm', '-', '.', '@'}
};

// ── SCREEN 0: NETWORK LIST RENDERING ──

static void drawNetworkListScreen() {
    tft.fillScreen(ST77XX_BLACK);

    // Header
    tft.fillRect(0, 0, tft.width(), 16, tft.color565(15, 25, 45));
    tft.drawFastHLine(0, 16, tft.width(), tft.color565(0, 200, 255));
    tft.setTextColor(tft.color565(0, 220, 255));
    tft.setTextSize(1);
    tft.setCursor(6, 4);
    tft.print("SELECT WI-FI NETWORK");

    int netCount = CloudUpdater::getScanCount();
    int startY = 22;

    if (netCount == 0) {
        tft.setTextColor(tft.color565(180, 180, 180));
        tft.setCursor(16, 45);
        tft.print("No Networks Detected");
        tft.setCursor(16, 60);
        tft.print("Press Y to Scan RF");
    } else {
        for (int i = 0; i < VISIBLE_NETWORKS; i++) {
            int idx = listScrollOffset + i;
            if (idx >= netCount) break;

            CloudUpdater::ScannedAP ap = CloudUpdater::getScannedNetwork(idx);
            bool isSel = (idx == selectedNetIdx);
            int y = startY + i * 16;

            if (isSel) {
                tft.fillRoundRect(4, y, 152, 14, 2, tft.color565(0, 120, 200));
                tft.setTextColor(ST77XX_WHITE);
            } else {
                tft.drawRoundRect(4, y, 152, 14, 2, tft.color565(40, 45, 65));
                tft.setTextColor(tft.color565(200, 210, 230));
            }

            // Security tag
            tft.setCursor(8, y + 3);
            tft.print(ap.encrypted ? "[L]" : "[O]");

            // SSID name (up to 14 chars)
            String name = ap.ssid.length() > 14 ? ap.ssid.substring(0, 14) : ap.ssid;
            tft.setCursor(30, y + 3);
            tft.print(name);

            // Signal indicator
            int bars = ap.rssi > -60 ? 3 : (ap.rssi > -75 ? 2 : 1);
            tft.setCursor(126, y + 3);
            if (bars == 3) tft.print("[|||]");
            else if (bars == 2) tft.print("[||.]");
            else tft.print("[|..]");
        }
    }

    // Status Area (Dedicated band at y=90)
    tft.fillRect(0, 90, tft.width(), 20, tft.color565(10, 12, 18));
    tft.setTextColor(statusColor);
    tft.setCursor(8, 95);
    tft.print(statusMessage);

    // Footer
    tft.fillRect(0, tft.height() - 12, tft.width(), 12, tft.color565(15, 15, 25));
    tft.drawFastHLine(0, tft.height() - 12, tft.width(), tft.color565(50, 50, 70));
    tft.setTextColor(tft.color565(160, 170, 190));
    tft.setCursor(6, tft.height() - 9);
    tft.print("A: Select  Y: Scan  B: Exit");
}

// ── SCREEN 1: FULL-WIDTH KEYBOARD RENDERING ──

static void drawKeyboardScreen() {
    tft.fillScreen(ST77XX_BLACK);

    // Header: SSID name
    tft.fillRect(0, 0, tft.width(), 14, tft.color565(15, 22, 38));
    tft.drawFastHLine(0, 14, tft.width(), tft.color565(0, 200, 255));
    tft.setTextColor(tft.color565(0, 220, 255));
    tft.setTextSize(1);
    tft.setCursor(4, 3);
    tft.printf("SSID: %s", targetSSID.length() > 14 ? targetSSID.substring(0, 14).c_str() : targetSSID.c_str());

    // Password Entry Box (x: 4, y: 17, w: 152, h: 14)
    tft.drawRoundRect(4, 17, 152, 14, 2, tft.color565(0, 200, 255));
    tft.fillRect(5, 18, 150, 12, tft.color565(8, 12, 20));

    // Show typed password with visible characters & cursor
    String disp = enteredPassword;
    if (disp.length() > 22) disp = disp.substring(disp.length() - 22);
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(8, 20);
    tft.print(disp);

    // Blinking cursor
    int cursorX = 8 + disp.length() * 6;
    if (cursorX < 150) {
        tft.drawFastVLine(cursorX, 19, 10, ST77XX_WHITE);
    }

    // ── Draw 10-Column QWERTY Grid ──
    const int startX = 4;
    const int startY = 35;
    const int keyW = 14;
    const int keyH = 13;
    const int gap = 1;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 10; c++) {
            int kx = startX + c * (keyW + gap);
            int ky = startY + r * (keyH + gap);

            char ch = shiftActive ? qwertyUpper[r][c] : qwertyLower[r][c];
            bool isFocused = (kbRow == r && kbCol == c);

            if (isFocused) {
                tft.fillRect(kx, ky, keyW, keyH, tft.color565(255, 220, 0));
                tft.setTextColor(ST77XX_BLACK);
            } else {
                tft.fillRect(kx, ky, keyW, keyH, tft.color565(22, 28, 40));
                tft.drawRect(kx, ky, keyW, keyH, tft.color565(40, 50, 70));
                tft.setTextColor(tft.color565(200, 215, 235));
            }

            tft.setCursor(kx + 4, ky + 3);
            if (ch == '_') tft.print(" ");
            else tft.print(ch);
        }
    }

    // ── Row 4: Action Buttons ──
    int row4Y = startY + 4 * (keyH + gap);

    struct ActionBtn {
        int colStart;
        int colSpan;
        const char* label;
        int x;
        int w;
    };

    ActionBtn actions[4] = {
        { 0, 2, shiftActive ? "ABC" : "abc", 4, 30 },
        { 2, 2, "DEL", 36, 30 },
        { 4, 2, "CLR", 68, 30 },
        { 6, 4, "CONNECT", 100, 56 }
    };

    for (int a = 0; a < 4; a++) {
        bool isFocused = (kbRow == 4 && kbCol >= actions[a].colStart && kbCol < actions[a].colStart + actions[a].colSpan);

        if (isFocused) {
            tft.fillRect(actions[a].x, row4Y, actions[a].w, keyH, tft.color565(255, 220, 0));
            tft.setTextColor(ST77XX_BLACK);
        } else {
            uint16_t bg = (a == 3) ? tft.color565(0, 100, 160) : tft.color565(25, 32, 45);
            tft.fillRect(actions[a].x, row4Y, actions[a].w, keyH, bg);
            tft.drawRect(actions[a].x, row4Y, actions[a].w, keyH, tft.color565(50, 60, 85));
            tft.setTextColor(ST77XX_WHITE);
        }

        int labelLen = strlen(actions[a].label);
        int textX = actions[a].x + (actions[a].w - labelLen * 6) / 2;
        tft.setCursor(textX, row4Y + 3);
        tft.print(actions[a].label);
    }

    // Footer
    tft.fillRect(0, tft.height() - 11, tft.width(), 11, tft.color565(12, 12, 20));
    tft.drawFastHLine(0, tft.height() - 11, tft.width(), tft.color565(40, 45, 60));
    tft.setTextColor(tft.color565(150, 160, 180));
    tft.setCursor(4, tft.height() - 8);
    tft.print("A:Type  X:Del  Y:Conn  B:Back");
}

// ── OTA PROGRESS CALLBACK ──

static void onOtaProgress(int percent, size_t bytesDownloaded, size_t totalBytes) {
    int bx = 10, by = 25, bw = 140, bh = 76;
    tft.fillRect(bx, by, bw, bh, tft.color565(12, 18, 32));
    tft.drawRect(bx, by, bw, bh, tft.color565(0, 200, 255));

    tft.setTextColor(tft.color565(0, 220, 255));
    tft.setCursor(bx + 14, by + 8);
    tft.print("UPDATING FIRMWARE");

    int barX = bx + 8;
    int barY = by + 24;
    int barW = bw - 16;
    int barH = 12;

    tft.drawRect(barX, barY, barW, barH, ST77XX_WHITE);
    int fillW = (barW - 4) * percent / 100;
    tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, ST77XX_GREEN);
    tft.fillRect(barX + 2 + fillW, barY + 2, (barW - 4) - fillW, barH - 4, ST77XX_BLACK);

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(bx + 16, by + 42);
    tft.printf("%3d%% (%u KB / %u KB)", percent, (unsigned int)(bytesDownloaded / 1024), (unsigned int)(totalBytes / 1024));

    tft.setTextColor(ST77XX_RED);
    tft.setCursor(bx + 14, by + 58);
    tft.print("DO NOT POWER OFF!");
}

// ── APPLICATION MAIN ──

static void startWifiUpdaterApp() {
    CloudUpdater::init();
    targetSSID = CloudUpdater::getSavedSSID();

    // Only load saved password if there was an explicitly saved SSID
    String savedPass = CloudUpdater::getSavedPassword();
    if (targetSSID.length() > 0 && savedPass.length() > 0) {
        enteredPassword = savedPass;
    } else {
        enteredPassword = ""; // Start clean so user types their exact password!
    }

    appState = STATE_NETWORKS_LIST;
    selectedNetIdx = 0;
    listScrollOffset = 0;

    statusMessage = "Scanning 2.4GHz RF...";
    statusColor = tft.color565(255, 180, 0);

    drawNetworkListScreen();

    // Start background RF scan
    CloudUpdater::startScan();
}

static void loopWifiUpdaterApp() {
    static unsigned long lastCursorBlink = 0;

    // Handle background scan completion
    if (appState == STATE_NETWORKS_LIST && CloudUpdater::isScanComplete()) {
        int count = CloudUpdater::getScanCount();
        if (count > 0) {
            statusMessage = String("Found: ") + count + " Networks";
            statusColor = ST77XX_GREEN;
        } else {
            statusMessage = "No APs detected";
            statusColor = ST77XX_RED;
        }
        drawNetworkListScreen();
    }

    Direction dir = readJoystickAutoRepeat(280, 110);

    // ═══════════════════════════════════════════════════════════
    //  STATE 0: NETWORK LIST NAVIGATION
    // ═══════════════════════════════════════════════════════════
    if (appState == STATE_NETWORKS_LIST) {
        int netCount = CloudUpdater::getScanCount();

        if (dir == DIR_UP) {
            if (selectedNetIdx > 0) {
                selectedNetIdx--;
                if (selectedNetIdx < listScrollOffset) listScrollOffset = selectedNetIdx;
                drawNetworkListScreen();
            }
        } else if (dir == DIR_DOWN) {
            if (selectedNetIdx < netCount - 1) {
                selectedNetIdx++;
                if (selectedNetIdx >= listScrollOffset + VISIBLE_NETWORKS) {
                    listScrollOffset = selectedNetIdx - VISIBLE_NETWORKS + 1;
                }
                drawNetworkListScreen();
            }
        }

        // Button A: Select network and proceed to TV Keyboard
        if (aPressedD()) {
            Buzzer::play(Buzzer::Sound::Click);
            if (netCount > 0 && selectedNetIdx < netCount) {
                targetSSID = CloudUpdater::getScannedNetwork(selectedNetIdx).ssid;
                String savedSSID = CloudUpdater::getSavedSSID();
                if (targetSSID == savedSSID) {
                    enteredPassword = CloudUpdater::getSavedPassword();
                } else {
                    enteredPassword = ""; // Fresh network: blank field!
                }
            } else {
                targetSSID = CloudUpdater::getSavedSSID();
                enteredPassword = CloudUpdater::getSavedPassword();
            }
            appState = STATE_KEYBOARD_ENTRY;
            kbRow = 0;
            kbCol = 0;
            drawKeyboardScreen();
            return;
        }

        // Button Y: Rescan APs
        if (yPressedD()) {
            Buzzer::play(Buzzer::Sound::Confirm);
            CloudUpdater::startScan();
            statusMessage = "Scanning 2.4GHz RF...";
            statusColor = tft.color565(255, 180, 0);
            drawNetworkListScreen();
            return;
        }
    }

    // ═══════════════════════════════════════════════════════════
    //  STATE 1: TV KEYBOARD PASSWORD ENTRY
    // ═══════════════════════════════════════════════════════════
    else if (appState == STATE_KEYBOARD_ENTRY) {
        // Grid navigation
        if (dir == DIR_UP) {
            kbRow = (kbRow + 4) % 5;
            drawKeyboardScreen();
        } else if (dir == DIR_DOWN) {
            kbRow = (kbRow + 1) % 5;
            drawKeyboardScreen();
        } else if (dir == DIR_LEFT) {
            kbCol = (kbCol + 9) % 10;
            drawKeyboardScreen();
        } else if (dir == DIR_RIGHT) {
            kbCol = (kbCol + 1) % 10;
            drawKeyboardScreen();
        }

        // Button A: Type character or trigger action
        if (aPressedD()) {
            Buzzer::play(Buzzer::Sound::Click);

            if (kbRow < 4) {
                // Type character
                char ch = shiftActive ? qwertyUpper[kbRow][kbCol] : qwertyLower[kbRow][kbCol];
                if (ch == '_') enteredPassword += ' ';
                else enteredPassword += ch;
                drawKeyboardScreen();
            } else {
                // Row 4 Actions
                if (kbCol < 2) {
                    // SHIFT toggle
                    shiftActive = !shiftActive;
                    drawKeyboardScreen();
                } else if (kbCol < 4) {
                    // DEL
                    if (enteredPassword.length() > 0) {
                        enteredPassword.remove(enteredPassword.length() - 1);
                        drawKeyboardScreen();
                    }
                } else if (kbCol < 6) {
                    // CLEAR
                    enteredPassword = "";
                    drawKeyboardScreen();
                } else {
                    // CONNECT action
                    goto trigger_connect;
                }
            }
            return;
        }

        // Button X: Quick Backspace
        if (xPressedD()) {
            Buzzer::play(Buzzer::Sound::Click);
            if (enteredPassword.length() > 0) {
                enteredPassword.remove(enteredPassword.length() - 1);
                drawKeyboardScreen();
            }
            return;
        }

        // Button SELECT: Quick Shift Toggle
        if (selectPressedD()) {
            Buzzer::play(Buzzer::Sound::Click);
            shiftActive = !shiftActive;
            drawKeyboardScreen();
            return;
        }

        // Button Y: Quick Connect Shortcut
        if (yPressedD()) {
trigger_connect:
            Buzzer::play(Buzzer::Sound::Confirm);
            appState = STATE_CONNECTING;

            // Render Connecting Modal
            tft.fillRect(10, 32, 140, 64, tft.color565(12, 18, 30));
            tft.drawRoundRect(10, 32, 140, 64, 3, tft.color565(0, 200, 255));
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(18, 40);
            tft.print("CONNECTING WI-FI...");
            tft.setTextColor(tft.color565(0, 220, 255));
            tft.setCursor(18, 54);
            tft.print(targetSSID.length() > 14 ? targetSSID.substring(0, 14) : targetSSID);

            tft.setTextColor(ST77XX_YELLOW);
            tft.setCursor(18, 68);
            tft.print("Associating & DHCP...");

            // Execute robust connection
            CloudUpdater::ConnectStatus res = CloudUpdater::connect(targetSSID, enteredPassword, 15);

            if (res == CloudUpdater::CONN_SUCCESS) {
                Buzzer::play(Buzzer::Sound::Confirm);
                tft.fillRect(12, 34, 136, 60, tft.color565(12, 18, 30));
                tft.setTextColor(ST77XX_GREEN);
                tft.setCursor(18, 42);
                tft.print("WI-FI CONNECTED!");
                tft.setTextColor(ST77XX_WHITE);
                tft.setCursor(18, 55);
                tft.printf("IP: %s", CloudUpdater::getLocalIP().c_str());

                tft.setTextColor(tft.color565(0, 200, 255));
                tft.setCursor(18, 68);
                tft.print("Checking GitHub...");
                delay(1200);

                // Query GitHub release
                bool found = CloudUpdater::checkLatestRelease(releaseInfo);
                if (found && releaseInfo.isNewer) {
                    appState = STATE_UPDATE_PROMPT;
                    tft.fillRect(10, 25, 140, 78, tft.color565(15, 20, 35));
                    tft.drawRoundRect(10, 25, 140, 78, 3, tft.color565(0, 220, 255));

                    tft.setTextColor(ST77XX_WHITE);
                    tft.setCursor(20, 32);
                    tft.print("NEW FIRMWARE FOUND!");

                    tft.setTextColor(ST77XX_YELLOW);
                    tft.setCursor(20, 46);
                    tft.printf("Remote:  %s", releaseInfo.tagName.c_str());
                    tft.setCursor(20, 58);
                    tft.printf("Current: %s", MEANTENDO_VERSION);

                    tft.setTextColor(tft.color565(0, 255, 180));
                    tft.setCursor(16, 74);
                    tft.print("A: Install   B: Skip");

                    while (true) {
                        if (aPressedD()) {
                            appState = STATE_FLASHING_OTA;
                            CloudUpdater::performOtaUpdate(releaseInfo.downloadUrl, releaseInfo.assetSize, onOtaProgress);
                            Buzzer::play(Buzzer::Sound::Confirm);
                            tft.fillScreen(ST77XX_BLACK);
                            tft.setTextColor(ST77XX_GREEN);
                            tft.setCursor(16, 55);
                            tft.print("UPDATE COMPLETE!");
                            tft.setCursor(16, 70);
                            tft.print("REBOOTING CONSOLE");
                            delay(1500);
                            ESP.restart();
                            break;
                        }
                        if (bPressedD() || backPressedD()) {
                            break;
                        }
                        delay(50);
                    }
                } else if (found) {
                    tft.fillRect(12, 66, 136, 18, tft.color565(12, 18, 30));
                    tft.setTextColor(ST77XX_GREEN);
                    tft.setCursor(18, 70);
                    tft.print("Console Up to Date!");
                    delay(1500);
                } else {
                    tft.fillRect(12, 66, 136, 18, tft.color565(12, 18, 30));
                    tft.setTextColor(tft.color565(180, 180, 180));
                    tft.setCursor(18, 70);
                    tft.print("No Releases on Repo");
                    delay(1500);
                }

                appState = STATE_NETWORKS_LIST;
                drawNetworkListScreen();
            } else {
                Buzzer::play(Buzzer::Sound::Error);
                tft.fillRect(10, 30, 140, 68, tft.color565(15, 18, 30));
                tft.drawRoundRect(10, 30, 140, 68, 3, ST77XX_RED);
                tft.setTextColor(ST77XX_RED);
                tft.setCursor(18, 38);
                tft.print("CONNECTION FAILED!");

                tft.setTextColor(ST77XX_YELLOW);
                tft.setCursor(18, 52);
                if (res == CloudUpdater::CONN_ERR_WRONG_PASSWORD) {
                    tft.print("Error: Wrong Password");
                } else if (res == CloudUpdater::CONN_ERR_NOT_FOUND) {
                    tft.print("Error: AP Not Found");
                } else {
                    tft.print("Error: Timeout / Reject");
                }

                tft.setTextColor(tft.color565(180, 190, 210));
                tft.setCursor(18, 68);
                tft.print("Press B to edit pass");

                while (!bPressedD() && !aPressedD() && !backPressedD()) {
                    delay(50);
                }
                appState = STATE_KEYBOARD_ENTRY;
                drawKeyboardScreen();
            }
            return;
        }

        // Button B: Return to Network List
        if (bPressedD()) {
            Buzzer::play(Buzzer::Sound::Click);
            appState = STATE_NETWORKS_LIST;
            drawNetworkListScreen();
            return;
        }

        // Periodic cursor blink
        if (millis() - lastCursorBlink > 400) {
            lastCursorBlink = millis();
            String disp = enteredPassword;
            if (disp.length() > 22) disp = disp.substring(disp.length() - 22);
            tft.fillRect(5, 18, 150, 12, tft.color565(8, 12, 20));
            tft.setTextColor(ST77XX_GREEN);
            tft.setCursor(8, 20);
            tft.print(disp);

            if ((millis() / 400) % 2 == 0) {
                int cursorX = 8 + disp.length() * 6;
                if (cursorX < 150) {
                    tft.drawFastVLine(cursorX, 19, 10, ST77XX_WHITE);
                }
            }
        }
    }

    delay(20);
}

GameDef wifiUpdaterApp = {
    "WiFi & Update",
    startWifiUpdaterApp,
    loopWifiUpdaterApp,
    1  // Category 1: Tools & Utilities
};
