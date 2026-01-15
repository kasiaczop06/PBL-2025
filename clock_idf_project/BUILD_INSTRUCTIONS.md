# Detailed Build Instructions

## Prerequisites

### 1. Install ESP-IDF

#### Windows

1. Download and run the ESP-IDF installer from:
   https://dl.espressif.com/dl/esp-idf/

2. Follow the installer wizard to install:
   - ESP-IDF v5.0 or later
   - Python 3.7+
   - Git
   - CMake
   - ESP-IDF Tools

3. The installer will create a shortcut "ESP-IDF Command Prompt" or "ESP-IDF PowerShell"

#### Linux/macOS

```bash
# Install prerequisites
# For Ubuntu/Debian:
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

# For macOS:
brew install cmake ninja dfu-util

# Clone ESP-IDF
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout v5.0  # or latest stable version

# Install tools
./install.sh esp32

# Set up environment variables (add to ~/.bashrc or ~/.zshrc)
. $HOME/esp/esp-idf/export.sh
```

## Project Setup

### 1. Navigate to Project Directory

```bash
cd clock_idf_project
```

### 2. Configure WiFi Credentials

You have two options:

#### Option A: Edit source code directly

Edit `main/clock_idf.c` lines 28-29:
```c
#define WIFI_SSID           "YourWiFiSSID"
#define WIFI_PASS           "YourWiFiPassword"
```

#### Option B: Use menuconfig (recommended)

```bash
idf.py menuconfig
```

Navigate to: `ESP32 Clock Configuration` → Enter your WiFi credentials

### 3. Configure Additional Settings (Optional)

Using `idf.py menuconfig`, you can configure:
- NTP Server
- Timezone
- LCD I2C Address
- HX711 Calibration Factor
- Servo Angles
- Scale Thresholds
- Coin Detection Parameters

## Building

### 1. Set Target (First Time Only)

```bash
idf.py set-target esp32
```

For ESP32-S2/S3/C3:
```bash
idf.py set-target esp32s2
# or
idf.py set-target esp32s3
# or
idf.py set-target esp32c3
```

### 2. Build the Project

```bash
idf.py build
```

The build process will:
1. Configure the project
2. Compile all source files
3. Link libraries
4. Generate firmware binary

Build output will be in the `build/` directory.

## Flashing

### 1. Connect ESP32

Connect your ESP32 to your computer via USB.

### 2. Identify Serial Port

#### Windows
Ports are usually `COM3`, `COM4`, etc.
Check Device Manager → Ports (COM & LPT)

#### Linux
```bash
ls /dev/ttyUSB*
# or
ls /dev/ttyACM*
```

Usually `/dev/ttyUSB0` or `/dev/ttyACM0`

#### macOS
```bash
ls /dev/cu.*
```

Usually `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`

### 3. Flash the Firmware

```bash
idf.py -p PORT flash
```

Replace `PORT` with your serial port:
```bash
# Windows
idf.py -p COM3 flash

# Linux
idf.py -p /dev/ttyUSB0 flash

# macOS
idf.py -p /dev/cu.usbserial-0001 flash
```

### 4. Monitor Output (Optional)

After flashing, you can monitor the serial output:

```bash
idf.py -p PORT monitor
```

Or combine flash and monitor:
```bash
idf.py -p PORT flash monitor
```

To exit the monitor, press `Ctrl+]`

## Troubleshooting

### Build Errors

**Error: `esp_sntp.h: No such file or directory`**
- Solution: Update to ESP-IDF v5.0 or later

**Error: `CMake not found`**
- Solution: Install CMake or add it to PATH

**Error: `Python not found`**
- Solution: Install Python 3.7+ and add to PATH

### Flash Errors

**Error: `Failed to connect to ESP32`**
- Solution:
  1. Hold BOOT button while connecting
  2. Press EN (reset) button
  3. Check USB cable and drivers

**Error: `Permission denied on serial port` (Linux)**
- Solution: Add user to dialout group:
  ```bash
  sudo usermod -a -G dialout $USER
  ```
  Then log out and log back in.

**Error: `Serial port not found`**
- Solution:
  1. Check USB connection
  2. Install CH340/CP2102/FTDI drivers
  3. Try different USB port

### Runtime Issues

**WiFi not connecting**
- Check SSID and password
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check router settings (WPA2 supported)

**LCD not displaying**
- Verify I2C address (try 0x27 or 0x3F)
- Check connections: SDA=GPIO21, SCL=GPIO22
- Test I2C with i2cdetect tool

**Scale not working**
- Check HX711 connections
- Verify load cell wiring (red/black for power, white/green for signal)
- Recalibrate using known weight

**Servo not responding**
- Ensure 5V power supply with sufficient current
- Check PWM signal on GPIO 13
- Test with external power if needed

## Advanced Configuration

### Custom Partition Table

To use custom partitions, edit `partitions.csv` and add to CMakeLists.txt:
```cmake
set(PARTITION_CSV_PATH ${CMAKE_SOURCE_DIR}/partitions.csv)
```

### Enable Logging

In menuconfig:
```
Component config → Log output → Default log verbosity → Verbose
```

### Optimize Binary Size

In menuconfig:
```
Compiler options → Optimization Level → Optimize for size (-Os)
```

### Enable Core Dump

In menuconfig:
```
Component config → ESP System Settings → Core dump → Enable core dump to Flash
```

## Cleaning

To clean build artifacts:
```bash
idf.py fullclean
```

To remove configuration:
```bash
rm sdkconfig
idf.py fullclean
```

## Testing Hardware Components

### Test LCD

```bash
# In monitor, you should see "Polaczono WiFi" or "Blad WiFi"
idf.py -p PORT monitor
```

### Test Scale

Check serial output for "HX711 initialized" and "Tared with offset"

### Test Servo

The servo should move to 0° on startup

### Test Buttons

Press Button 3 to toggle modes and observe LCD changes

## Performance Optimization

For better performance:
1. Enable compiler optimization in menuconfig
2. Increase FreeRTOS tick rate if needed
3. Adjust task priorities in code
4. Enable PSRAM if available

## Next Steps

After successful flashing:
1. Calibrate the scale with known weights
2. Set up alarm parameters using buttons
3. Test coin detection mechanism
4. Adjust servo angles if needed

For issues, check:
- Serial monitor output
- ESP-IDF documentation: https://docs.espressif.com/projects/esp-idf/
- ESP32 forums: https://esp32.com/
