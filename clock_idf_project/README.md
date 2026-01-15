# ESP32 Clock with Scale and Servo - ESP-IDF Implementation

This project is a complete ESP-IDF port of the Arduino-based clock system. It includes WiFi connectivity, NTP time synchronization, LCD display, HX711 scale integration, servo control, and alarm functionality.

## Features

- **WiFi & NTP Time Sync**: Automatically synchronizes time from NTP servers
- **LCD Display (20x4 I2C)**: Shows current time, date, day of week, alarm settings, and collected amount
- **HX711 Load Cell**: Weight measurement for coin detection
- **Servo Motor Control**: "Cat paw" mechanism activated by weight detection
- **Alarm System**: Configurable day, hour, minute, and target amount
- **Button Interface**: Three buttons for alarm configuration and mode switching
- **FreeRTOS Tasks**: Multi-threaded design for concurrent operations

## Hardware Requirements

- ESP32 Development Board
- 20x4 LCD with I2C adapter (PCF8574)
- HX711 Load Cell Amplifier + Load Cell
- Servo Motor
- 3 Push Buttons
- Speaker/Buzzer (optional, for alarm sound)

## Pin Configuration

| Component | GPIO Pin |
|-----------|----------|
| LCD SDA | GPIO 21 |
| LCD SCL | GPIO 22 |
| HX711 DOUT | GPIO 18 |
| HX711 SCK | GPIO 19 |
| Servo | GPIO 13 |
| Button 1 | GPIO 10 |
| Button 2 | GPIO 9 |
| Button 3 IN | GPIO 20 |
| Button 3 OUT | GPIO 8 |
| Speaker | GPIO 14 |

## Project Structure

```
clock_idf_project/
├── main/
│   ├── clock_idf.c         # Main application code
│   └── CMakeLists.txt      # Main component build config
├── components/
│   ├── lcd_i2c/            # LCD I2C driver component
│   │   ├── lcd_i2c.h
│   │   ├── lcd_i2c.c
│   │   └── CMakeLists.txt
│   └── hx711/              # HX711 ADC driver component
│       ├── hx711.h
│       ├── hx711.c
│       └── CMakeLists.txt
├── CMakeLists.txt          # Project root build config
├── sdkconfig.defaults      # Default SDK configuration
└── README.md
```

## Software Requirements

- ESP-IDF v5.0 or later
- Python 3.7 or later
- CMake 3.16 or later

## Building the Project

### 1. Setup ESP-IDF Environment

First, ensure ESP-IDF is installed and the environment is set up:

```bash
# Linux/macOS
. $HOME/esp/esp-idf/export.sh

# Windows (Command Prompt)
%userprofile%\esp\esp-idf\export.bat

# Windows (PowerShell)
$HOME\esp\esp-idf\export.ps1
```

### 2. Configure WiFi Credentials

Edit `main/clock_idf.c` and update the WiFi credentials:

```c
#define WIFI_SSID           "YourWiFiSSID"
#define WIFI_PASS           "YourWiFiPassword"
```

### 3. Build the Project

```bash
cd clock_idf_project
idf.py set-target esp32
idf.py build
```

### 4. Flash to ESP32

```bash
idf.py -p PORT flash monitor
```

Replace `PORT` with your ESP32's serial port (e.g., `COM3` on Windows, `/dev/ttyUSB0` on Linux).

## Configuration

### Timezone

The code is set to CET (Central European Time, UTC+1). To change the timezone, modify this line in `clock_idf.c`:

```c
setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
```

### Scale Calibration

The HX711 scale uses a calibration factor. Adjust this value in `clock_idf.c`:

```c
hx711_set_scale(&scale, -16347.25); // Adjust this value
```

To calibrate:
1. Place a known weight on the scale
2. Read the raw value
3. Calculate: calibration_factor = raw_value / known_weight

### Servo Angles

Servo movement is configured with two positions:
- 0° (retracted)
- 90° (extended)

Adjust in the `cat_paw_control()` function if needed.

## Usage

### Normal Mode (Display Time)

When button 3 output is LOW, the display shows:
- Line 1: Current time (centered)
- Line 2: Day of week and date
- Line 3: "Zebrane: XX/YY" (Collected/Target)
- Line 4: Alarm time

### Alarm Setup Mode

Press Button 3 to toggle to setup mode (Button 3 output HIGH):
- Display shows "Do uzbierania: XX" (To collect)

**Button Functions:**
- **Button 1**: Cycles through settings (Day → Hour → Minute → Target Amount)
- **Button 2**: Increments the current setting value
- **Button 3**: Toggle between normal and setup modes

### Alarm Trigger

The alarm triggers when:
1. Current day ≥ Alarm day
2. Current hour ≥ Alarm hour
3. Current minute ≥ Alarm minute
4. Collected amount ≥ Target amount
5. Setup mode is OFF (Button 3 output LOW)

## FreeRTOS Tasks

The application uses four main tasks:

1. **time_update_task** (1000ms): Updates time from NTP, checks scale continuously
2. **display_task** (100ms): Updates LCD display with current information
3. **button_task** (10ms): Monitors button states with debouncing
4. **scale_servo_task** (100ms): Controls servo based on scale readings, checks alarm

## Functional Logic

### Cat Paw Control
1. Scale detects weight ≥ 0.5 units → Servo moves to 90°
2. After 7 seconds of continuous weight detection
3. If weight is between 4.8-5.4 units → Add 5 to collected amount
4. When weight is removed → Servo returns to 0°

### Alarm System
- Day: 0-7 (0=Sunday, 1=Monday, etc.)
- Hour: 0-23
- Minute: 0-59
- Target amount: 0-100

## Troubleshooting

### LCD Not Working
- Check I2C address (default: 0x27)
- Verify SDA/SCL connections
- Check pull-up resistors on I2C lines

### WiFi Connection Issues
- Verify SSID and password
- Check WiFi signal strength
- Monitor serial output for connection status

### Scale Not Responding
- Check HX711 connections
- Verify power supply to load cell
- Re-calibrate scale

### Servo Not Moving
- Check power supply (servos need 5V, sufficient current)
- Verify GPIO 13 connection
- Test with simple angle commands

## Original Arduino Code

This ESP-IDF implementation is based on the Arduino code in `zegar2.1.cpp` and maintains the same functionality with the following improvements:

- FreeRTOS multi-tasking for better performance
- Proper hardware abstraction through ESP-IDF drivers
- Improved error handling
- Thread-safe time access with mutexes
- More efficient GPIO handling

## License

This project is provided as-is for educational purposes.

## Author

Converted from Arduino to ESP-IDF with FreeRTOS
Original Arduino implementation: zegar2.1.cpp
