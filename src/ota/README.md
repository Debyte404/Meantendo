# 🚀 Meantendo OTA Update System

A hyper-advanced Over-The-Air (OTA) update system for the Meantendo ESP32 gaming console, featuring GitHub Releases integration, secure updates, and a beautiful web interface.

## ✨ Features

### Core Capabilities
- **🔄 GitHub Releases Integration** - Automatically fetches updates from GitHub Releases API
- **📦 Semantic Versioning** - Full semver support with pre-release and build metadata
- **💾 NVS Persistence** - Version history and WiFi credentials stored in flash
- **⚡ Non-blocking Updates** - Async update checks don't freeze the console
- **🔙 A/B Rollback** - Dual partition scheme for safe rollback to previous firmware

### Security
- **🔐 ECDSA P-256 Signing** - Firmware signature verification
- **🔒 HTTPS Support** - Secure download from GitHub/custom servers
- **✅ SHA256 Validation** - Integrity verification before flashing

### User Interface
- **📺 TFT Menu Integration** - Check updates directly from the game menu
- **🌐 Web Interface** - Beautiful responsive dashboard at device IP
- **📊 Real-time Progress** - WebSocket-powered live progress updates
- **📁 Drag & Drop Upload** - Easy manual firmware upload via browser

### CI/CD
- **🔨 GitHub Actions** - Automated build on push/tag
- **📋 Auto-Release** - Automatic GitHub Release creation on tags
- **🏷️ Version Injection** - Build-time version from git tags

## 📁 File Structure

```
src/ota/
├── OTAConfig.hpp       # Configuration constants and types
├── OTAManager.hpp      # Main OTA manager class header
├── OTAManager.cpp      # OTA manager implementation
├── OTAWebUI.hpp        # Web interface header
├── OTAWebUI.cpp        # Web interface implementation
└── OTAMenuItem.hpp     # TFT menu integration

partitions/
└── ota_partitions.csv  # A/B OTA partition table

scripts/
└── sign_firmware.py    # Firmware signing tool

.github/workflows/
└── ota-build-release.yml  # CI/CD pipeline
```

## 🛠️ Usage

### From the Console Menu

1. Navigate to **"OTA Update"** in the game menu
2. Select **"Check Updates"** to connect to WiFi and check GitHub
3. If update available, select **"Install Now"**
4. Wait for download and installation (DO NOT POWER OFF!)
5. Console will automatically reboot with new firmware

### From the Web Interface

1. Select **"Start Web UI"** from the OTA menu
2. Connect to the displayed IP address in any web browser
3. Use the dashboard to:
   - Check for updates
   - View system information
   - Upload firmware manually
   - Rollback to previous version
   - Configure WiFi

### WiFi Configuration

On first boot or if WiFi is not configured:

1. Select **"WiFi Config"** from the OTA menu
2. Connect to the `Meantendo-Setup` WiFi network (password: `meantendo123`)
3. Open `http://192.168.4.1` in your browser
4. Enter your WiFi credentials and save

## 🔧 Configuration

### Edit `src/ota/OTAConfig.hpp`:

```cpp
// GitHub Repository
#define OTA_GITHUB_OWNER  "YourUsername"
#define OTA_GITHUB_REPO   "YourRepo"

// WiFi Defaults
#define OTA_DEFAULT_WIFI_SSID  "YourSSID"
#define OTA_DEFAULT_WIFI_PASS  "YourPassword"

// Custom OTA Server (optional)
#define OTA_CUSTOM_SERVER_URL  "https://your-ota-server.com"
```

## 📦 Creating Releases

### Automatic (Recommended)

1. Tag your commit:
   ```bash
   git tag v1.2.3
   git push origin v1.2.3
   ```

2. GitHub Actions will:
   - Build firmware for all variants
   - Create GitHub Release
   - Upload `.bin` files and checksums
   - Generate OTA manifest

### Manual

1. Build with PlatformIO:
   ```bash
   pio run -e esp32dev_release
   ```

2. Sign firmware (optional):
   ```bash
   python scripts/sign_firmware.py \
     --input .pio/build/esp32dev_release/firmware.bin \
     --key keys/private.pem \
     --output firmware-v1.2.3-signed.bin
   ```

3. Create GitHub Release and upload the `.bin` file

## 🔐 Firmware Signing

### Generate Signing Keys

```bash
python scripts/sign_firmware.py --generate-keys --output-dir keys/
```

This creates:
- `keys/meantendo_private.pem` - Keep this secret!
- `keys/meantendo_public.pem` - Embed in firmware
- `keys/meantendo_public_key.h` - C header for embedding

### Sign Firmware

```bash
python scripts/sign_firmware.py \
  --input firmware.bin \
  --key keys/meantendo_private.pem \
  --output firmware_signed.bin
```

### Verify Signature

```bash
python scripts/sign_firmware.py \
  --verify firmware_signed.bin \
  --public-key keys/meantendo_public.pem
```

## 📊 Partition Table

The OTA system uses dual app partitions for safe rollback:

| Partition  | Type   | Size    | Purpose                    |
|------------|--------|---------|----------------------------|
| `nvs`      | data   | 20KB    | WiFi, OTA config, stats    |
| `otadata`  | data   | 8KB     | Active partition tracking  |
| `app0`     | app    | 1.5MB   | Primary application        |
| `app1`     | app    | 1.5MB   | Backup/rollback partition  |
| `spiffs`   | data   | 512KB   | Web UI assets              |
| `ota_config`| data  | 16KB    | OTA-specific settings      |

## 🔄 Update Policies

Configure in code or via NVS:

| Policy         | Description                              |
|----------------|------------------------------------------|
| `AUTO`         | Install updates automatically            |
| `MANUAL`       | User must trigger updates                |
| `SCHEDULED`    | Check at configured intervals            |
| `WIFI_ONLY`    | Only update when on WiFi                 |
| `BATTERY_SAFE` | Only update when battery > threshold     |
| `DEVELOPMENT`  | Accept pre-release versions              |

## 🧪 API Reference

### OTAManager Methods

```cpp
// Initialize
OTA.begin(config);

// WiFi
OTA.connectWiFi("ssid", "pass");
OTA.startAPMode();
OTA.saveWiFiCredentials("ssid", "pass");

// Updates
OTA.checkForUpdateAsync();
OTA.checkForUpdate(manifest);
OTA.startUpdate();
OTA.startUpdateFromURL("https://...");
OTA.getProgress();

// Rollback
OTA.canRollback();
OTA.markFirmwareValid();
OTA.rollback();

// Info
OTA.getCurrentVersion();
OTA.getVersionString();
OTA.getPartitionInfo();
```

## 🐛 Troubleshooting

### Update Check Fails
- Verify WiFi credentials are correct
- Check GitHub API rate limits (60 req/hour for unauthenticated)
- Ensure firmware release has a `.bin` asset

### Download Fails
- Check available flash space (`OTA.getFreeOTASpace()`)
- Verify HTTPS connectivity
- Try with `client.setInsecure()` for testing

### Rollback Not Available
- First boot always has no rollback
- Previous partition must contain valid firmware

### Web UI Not Loading
- Check device IP is correct
- Ensure port 80 and 81 are not blocked
- Try refreshing the page

## 📜 License

MIT License - See [LICENSE](../LICENSE)

---

**Built with ☕ by Debyte**
