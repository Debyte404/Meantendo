#pragma once

// ═══════════════════════════════════════════════════════════
//  Meantendo Configuration & Cloud OTA Settings
// ═══════════════════════════════════════════════════════════

#define MEANTENDO_VERSION         "v2.2.1"
#define MEANTENDO_GITHUB_OWNER    "Debyte404"
#define MEANTENDO_GITHUB_REPO     "Meantendo"
#define MEANTENDO_ASSET_NAME      "firmware.bin"

// Fallback Default WiFi (Can be customized or changed in NVS via Serial / SmartConfig)
#ifndef MEANTENDO_DEFAULT_SSID
#define MEANTENDO_DEFAULT_SSID    "MeantendoHotspot"
#endif

#ifndef MEANTENDO_DEFAULT_PASS
#define MEANTENDO_DEFAULT_PASS    "12345678"
#endif

// Serial command help string
#define MEANTENDO_SERIAL_HELP     "Commands: SETWIFI <ssid> <pass> | CHECKUPDATE | REBOOT | STATUS"
