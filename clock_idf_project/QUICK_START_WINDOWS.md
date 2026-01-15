# Quick Start Guide for Windows

This guide will help you quickly build and flash the ESP32 clock project on Windows.

## Step 1: Install ESP-IDF (One-time Setup)

1. **Download ESP-IDF Installer**
   - Go to: https://dl.espressif.com/dl/esp-idf/
   - Download: `esp-idf-tools-setup-x.x.x.exe` (latest version)

2. **Run the Installer**
   - Double-click the installer
   - Choose "Express Installation"
   - Select ESP-IDF version: v5.0 or later
   - Click "Install"
   - Wait for installation to complete (10-15 minutes)

3. **Launch ESP-IDF Command Prompt**
   - After installation, find "ESP-IDF 5.x CMD" in Start Menu
   - Or "ESP-IDF 5.x PowerShell"

## Step 2: Configure WiFi

1. **Open the main source file**
   ```
   notepad clock_idf_project\main\clock_idf.c
   ```

2. **Find lines 28-29 and edit:**
   ```c
   #define WIFI_SSID           "YourWiFiName"
   #define WIFI_PASS           "YourWiFiPassword"
   ```

3. **Save and close**

## Step 3: Build the Project

1. **Open ESP-IDF Command Prompt**
   - Find "ESP-IDF 5.x CMD" in Start Menu

2. **Navigate to project**
   ```
   cd c:\temp\kasia\PBL-2025\clock_idf_project
   ```

3. **Set target (first time only)**
   ```
   idf.py set-target esp32
   ```

4. **Build**
   ```
   idf.py build
   ```

   Wait for build to complete (2-5 minutes first time)

## Step 4: Flash to ESP32

1. **Connect ESP32 to USB**
   - Use a quality USB cable (data cable, not charge-only)

2. **Find COM Port**
   - Open Device Manager (Win+X → Device Manager)
   - Expand "Ports (COM & LPT)"
   - Look for "USB-SERIAL CH340" or "Silicon Labs CP210x"
   - Note the COM port (e.g., COM3, COM4)

3. **Flash**
   ```
   idf.py -p COM3 flash
   ```

   Replace COM3 with your actual port

4. **Monitor (Optional)**
   ```
   idf.py -p COM3 monitor
   ```

   Press `Ctrl+]` to exit

## Step 5: Verify Operation

1. **Check LCD Display**
   - Should show "Polaczono WiFi" after WiFi connects
   - Then display current time, date, and day

2. **Test Scale**
   - Place weight on scale
   - Servo should move to 90°

3. **Test Buttons**
   - Press Button 3 to toggle between display mode and setup mode

## Troubleshooting

### Can't Find ESP-IDF Command Prompt
- Reinstall ESP-IDF
- Or manually add to PATH: `C:\Espressif\frameworks\esp-idf\export.bat`

### Build Error: "idf.py not recognized"
- Make sure you're using "ESP-IDF Command Prompt", not regular Command Prompt
- Or run: `C:\Espressif\frameworks\esp-idf\export.bat` first

### Flash Error: "Failed to connect"
1. Hold BOOT button on ESP32
2. Press EN (reset) button
3. Release BOOT button
4. Try flash command again

### Flash Error: "Permission denied"
- Close any serial monitor programs (Arduino IDE, PuTTY, etc.)
- Disconnect and reconnect USB cable
- Try different USB port

### WiFi Not Connecting
- Make sure SSID and password are correct
- ESP32 only supports 2.4GHz WiFi (not 5GHz)
- Check router firewall settings

### LCD Not Working
- Check I2C address (default 0x27)
- Verify connections:
  - SDA → GPIO 21
  - SCL → GPIO 22
  - VCC → 5V
  - GND → GND

## Pin Connections Summary

```
ESP32 → Component
─────────────────────
GPIO 21 → LCD SDA
GPIO 22 → LCD SCL
GPIO 18 → HX711 DOUT
GPIO 19 → HX711 SCK
GPIO 13 → Servo Signal
GPIO 10 → Button 1
GPIO 9  → Button 2
GPIO 20 → Button 3 Input
GPIO 8  → Button 3 Output (internal)
GPIO 14 → Speaker (optional)
5V      → LCD VCC, Servo VCC, HX711 VCC
GND     → All GND pins
```

## Next Steps

1. **Calibrate Scale**
   - Place known weight
   - Calculate calibration factor
   - Update in `clock_idf.c` line 598

2. **Set Alarm**
   - Press Button 3 to enter setup mode
   - Use Button 1 to select field (Day/Hour/Minute/Amount)
   - Use Button 2 to increment value
   - Press Button 3 to save and exit

3. **Test Coin Detection**
   - Place 5 PLN coin (or similar weight 4.8-5.4g)
   - Wait 7 seconds
   - Check if collected amount increases

## Getting Help

If you encounter issues:
1. Check serial monitor output for error messages
2. Read full BUILD_INSTRUCTIONS.md
3. Check ESP-IDF documentation: https://docs.espressif.com/
4. Post on ESP32 forum: https://esp32.com/

## Common Commands Reference

```bash
# Build
idf.py build

# Flash
idf.py -p COM3 flash

# Monitor
idf.py -p COM3 monitor

# Flash and Monitor
idf.py -p COM3 flash monitor

# Clean build
idf.py fullclean

# Configure project
idf.py menuconfig

# Erase flash
idf.py -p COM3 erase-flash
```

## Project Structure

```
clock_idf_project/
├── main/
│   └── clock_idf.c        ← Main code (edit WiFi here)
├── components/
│   ├── lcd_i2c/           ← LCD driver
│   └── hx711/             ← Scale driver
├── CMakeLists.txt         ← Build configuration
└── README.md              ← Full documentation
```

Good luck with your ESP32 clock project!
