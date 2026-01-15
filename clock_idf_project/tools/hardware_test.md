# Hardware Connection Test Guide

This guide helps you verify that all hardware components are properly connected before running the full application.

## Pre-flight Checklist

Before powering on:
- [ ] All components have common ground (GND)
- [ ] Power supply is 5V with at least 2A capacity
- [ ] ESP32 is not connected to USB yet
- [ ] Servo has separate power if needed
- [ ] Double-check all pin connections

## Component-by-Component Testing

### Test 1: ESP32 Basic Function

**What to test**: ESP32 boots and responds

**Steps**:
1. Connect only ESP32 to USB (no other components)
2. Open serial monitor: `idf.py monitor`
3. Press EN button on ESP32
4. You should see boot messages

**Expected output**:
```
ESP-ROM:esp32c3-api1-20210207
Build:Feb  7 2021
rst:0x1 (POWERON),boot:0xc (SPI_FAST_FLASH_BOOT)
```

**If failed**:
- Check USB cable (use data cable, not charge-only)
- Install CP2102 or CH340 drivers
- Try different USB port

---

### Test 2: I2C LCD Display

**What to test**: LCD receives I2C signals

**Wiring**:
```
ESP32 GPIO 21 → LCD SDA
ESP32 GPIO 22 → LCD SCL
ESP32 5V      → LCD VCC
ESP32 GND     → LCD GND
```

**Test code** (modify main/clock_idf.c temporarily):
```c
void app_main(void) {
    // Initialize I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    // Initialize LCD
    lcd_handle_t lcd = lcd_init(I2C_NUM_0, 0x27, 4, 20);
    lcd_backlight_on(lcd);
    lcd_clear(lcd);
    lcd_print(lcd, "LCD Test OK!");

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

**Expected result**: LCD shows "LCD Test OK!"

**If failed**:
- LCD shows nothing:
  - Check VCC is 5V (not 3.3V)
  - Verify I2C address (try 0x3F instead of 0x27)
  - Adjust contrast potentiometer on I2C backpack
  - Measure voltage on SDA/SCL (should be ~3.3V when idle)

- LCD backlight on but no text:
  - I2C address wrong (use i2cdetect tool)
  - SDA/SCL swapped
  - Check solder joints on I2C backpack

---

### Test 3: HX711 Load Cell

**What to test**: Scale reads values

**Wiring**:
```
ESP32 GPIO 18 → HX711 DT (DOUT)
ESP32 GPIO 19 → HX711 SCK
ESP32 5V      → HX711 VCC
ESP32 GND     → HX711 GND

Load Cell Red   → HX711 E+
Load Cell Black → HX711 E-
Load Cell White → HX711 A-
Load Cell Green → HX711 A+
```

**Test code**:
```c
void app_main(void) {
    hx711_t scale = {
        .dout = GPIO_NUM_18,
        .pd_sck = GPIO_NUM_19,
        .gain = HX711_GAIN_A_64
    };

    hx711_init(&scale);
    vTaskDelay(pdMS_TO_TICKS(2000));
    hx711_tare(&scale, 10);

    while(1) {
        float weight = hx711_get_units(&scale, 10);
        ESP_LOGI("TEST", "Weight: %.2f", weight);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

**Expected result**:
- Serial shows: `Weight: 0.00` (no load)
- Place weight: Value increases
- Remove weight: Returns to ~0

**If failed**:
- Always shows 0:
  - DT/SCK pins swapped
  - HX711 not powered
  - Bad connections

- Noisy readings (jumping values):
  - Poor load cell mounting
  - Loose wires
  - EMI interference (move away from power supplies)

- Negative values:
  - A+/A- reversed on load cell
  - Adjust scale factor sign

---

### Test 4: Servo Motor

**What to test**: Servo responds to PWM

**Wiring**:
```
ESP32 GPIO 13 → Servo Signal (yellow/white)
5V Power      → Servo VCC (red)
GND           → Servo GND (brown/black)
```

**IMPORTANT**:
- Servo draws 500mA+ under load
- May need separate 5V power supply
- Connect grounds together if using separate supply

**Test code**:
```c
void app_main(void) {
    // Setup LEDC for servo
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_NUM_13,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    while(1) {
        // 0 degrees
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 205);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ESP_LOGI("TEST", "Servo: 0 degrees");
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 90 degrees
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ESP_LOGI("TEST", "Servo: 90 degrees");
        vTaskDelay(pdMS_TO_TICKS(2000));

        // 180 degrees
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1024);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        ESP_LOGI("TEST", "Servo: 180 degrees");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

**Expected result**:
- Servo sweeps: 0° → 90° → 180° → repeat
- Movement is smooth
- No jittering

**If failed**:
- Servo jitters:
  - Insufficient power supply
  - Bad ground connection
  - Noisy power line (add capacitor)

- Servo doesn't move:
  - Check signal wire connection
  - Verify 5V power
  - Test servo with multimeter (signal should be 1-2ms pulse)
  - Try different GPIO pin

- Servo moves but wrong angles:
  - Adjust duty cycle values (205, 512, 1024)
  - Different servo models need different timing

---

### Test 5: Push Buttons

**What to test**: Button presses detected

**Wiring**:
```
ESP32 GPIO 10 → Button 1 → GND
ESP32 GPIO 9  → Button 2 → GND
ESP32 GPIO 20 → Button 3 → GND
```

**Note**: Internal pull-ups will be enabled in code

**Test code**:
```c
void app_main(void) {
    // Configure buttons as input with pull-up
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = ((1ULL<<10) | (1ULL<<9) | (1ULL<<20)),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    while(1) {
        bool btn1 = (gpio_get_level(GPIO_NUM_10) == 0);
        bool btn2 = (gpio_get_level(GPIO_NUM_9) == 0);
        bool btn3 = (gpio_get_level(GPIO_NUM_20) == 0);

        ESP_LOGI("TEST", "Buttons: 1=%d 2=%d 3=%d", btn1, btn2, btn3);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
```

**Expected result**:
- Press Button 1: `Buttons: 1=1 2=0 3=0`
- Press Button 2: `Buttons: 1=0 2=1 3=0`
- Press Button 3: `Buttons: 1=0 2=0 3=1`

**If failed**:
- Button always pressed:
  - Check button is not stuck
  - Verify pull-up enabled
  - Button wired backwards

- Button never detected:
  - Check button connection
  - Test button with multimeter (should close circuit)
  - Check GPIO pin number

---

### Test 6: WiFi Connection

**What to test**: ESP32 connects to WiFi

**Test code**:
```c
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("TEST", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void app_main(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "YourSSID",
            .password = "YourPassword",
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

**Expected result**:
```
I (3542) wifi:connected with YourSSID, aid = 1, channel 6
I (4892) TEST: Got IP: 192.168.1.xxx
```

**If failed**:
- Never connects:
  - Wrong SSID or password
  - Out of range
  - 5GHz network (ESP32 needs 2.4GHz)
  - Router MAC filtering enabled

- Connects then disconnects:
  - Weak signal
  - Router DHCP issues
  - Power supply insufficient

---

## Complete System Test

After individual components work, test all together:

### Step 1: Connect Everything
Follow the complete wiring diagram in README.md

### Step 2: Power Budget Check
Calculate total current:
- ESP32 WiFi active: 120 mA
- LCD backlight: 30 mA
- HX711: 10 mA
- Servo idle: 10 mA
- **Total idle**: ~170 mA
- **With servo active**: Add 500 mA = 670 mA

Ensure power supply can provide 2A minimum.

### Step 3: Flash Full Application
```bash
idf.py flash monitor
```

### Step 4: Monitor Boot Sequence
Look for these messages:
1. `ESP32 Clock starting...`
2. `LCD initialized successfully`
3. `HX711 initialized`
4. `WiFi initialization finished.`
5. `Connected to WiFi, IP: xxx.xxx.xxx.xxx`
6. `Initializing SNTP`
7. `Time synchronized`
8. `All tasks created, system running`

### Step 5: Functional Tests

**Test A: Time Display**
- LCD should show current time (after NTP sync)
- Time updates every second
- Date and day of week shown

**Test B: Scale → Servo**
1. Place any weight on scale
2. Servo should move to 90° within 100ms
3. Remove weight
4. Servo returns to 0°

**Test C: Coin Detection**
1. Place 5 PLN coin (or 5g weight)
2. Wait 7 seconds
3. Check LCD: "Zebrane: 5/0" (or target)
4. Collected amount increased

**Test D: Button Configuration**
1. Press Button 3 → Mode changes
2. Press Button 1 → Cycles through settings
3. Press Button 2 → Increments value
4. Press Button 3 → Saves and exits

**Test E: Alarm**
1. Set alarm time to current + 1 minute
2. Set target amount to 0
3. Exit setup mode
4. Wait for alarm time
5. Serial monitor shows "ALARM!"

---

## Troubleshooting Matrix

| Symptom | Possible Cause | Solution |
|---------|---------------|----------|
| Nothing works | No power | Check USB/power supply |
| ESP32 boots, nothing else | Wrong pins | Verify pin numbers in code |
| LCD backlight only | I2C address wrong | Try 0x3F instead of 0x27 |
| LCD garbled text | Bad I2C connection | Check SDA/SCL wires |
| Scale always zero | HX711 not connected | Check DT/SCK pins |
| Scale very noisy | Poor mounting | Secure load cell properly |
| Servo jitters | Low power | Use separate 5V supply |
| Servo doesn't move | Bad signal | Check GPIO 13 connection |
| WiFi fails | Wrong credentials | Update SSID/password |
| Time wrong | Timezone | Set correct TZ string |
| Buttons not working | No pull-up | Check gpio_config pull_up_en |
| Random crashes | Insufficient power | Use 2A+ power supply |

---

## Measurement Points

Use multimeter to verify:

| Test Point | Expected Voltage | Notes |
|------------|------------------|-------|
| ESP32 3.3V pin | 3.3V ±0.1V | Regulated by ESP32 |
| ESP32 5V pin | 5.0V ±0.2V | From USB or VIN |
| LCD VCC | 5.0V | Must be 5V, not 3.3V |
| HX711 VCC | 5.0V | Can be 2.7V-5.0V |
| Servo VCC | 5.0V | Should be 4.8V-6.0V |
| I2C SDA idle | 3.3V | Pull-up voltage |
| I2C SCL idle | 3.3V | Pull-up voltage |
| Servo signal | 0-3.3V PWM | 50Hz square wave |

---

## Logic Analyzer Verification

If available, use logic analyzer on:

### I2C Bus (SDA/SCL)
- Frequency: 100 kHz
- LCD address: 0x27 or 0x3F
- Should see continuous traffic

### PWM Signal (GPIO 13)
- Frequency: 50 Hz (20ms period)
- Pulse width: 1ms (0°) to 2ms (180°)
- Should be clean square wave

### HX711 Clock (GPIO 19)
- Should see 24 pulses per reading
- Additional pulses for gain setting

---

## Final Checklist

Before permanent installation:

- [ ] All tests passed individually
- [ ] Full system test passed
- [ ] No error messages in serial monitor
- [ ] All components respond correctly
- [ ] Power supply adequate (no brownouts)
- [ ] Connections secured (solder or screw terminals)
- [ ] Strain relief on wires
- [ ] Proper grounding (no ground loops)
- [ ] WiFi signal strong (-70 dBm or better)
- [ ] Scale calibrated with known weight
- [ ] Servo angles adjusted correctly
- [ ] Buttons debounce properly
- [ ] LCD readable from desired distance

---

## Tools Required

### Essential:
- Multimeter (voltage/continuity)
- Serial monitor (idf.py monitor)
- Known weights for calibration

### Recommended:
- Logic analyzer
- Oscilloscope
- USB power meter
- Wire stripper/crimper

### Nice to have:
- I2C scanner tool
- Spectrum analyzer (WiFi debugging)
- Thermal camera (overheat detection)

---

## Safety Notes

⚠️ **Important Safety Warnings:**

1. **Power Supply**: Use quality 5V 2A+ supply with overcurrent protection
2. **Servo Power**: Separate servo power if possible to avoid ESP32 brownouts
3. **No Reverse Polarity**: Double-check VCC/GND before powering on
4. **ESD Protection**: Touch grounded metal before handling ESP32
5. **Heat**: Check for hot components during operation
6. **Wire Gauge**: Use appropriate wire gauge for current (22-24 AWG minimum)
7. **Secure Connections**: No loose wires that could short
8. **Enclosure**: Protect from water, dust, and physical damage

---

## Getting Help

If tests fail:

1. **Document the failure**:
   - Take photos of setup
   - Copy error messages
   - Note which test failed

2. **Check forums**:
   - ESP32.com
   - r/esp32 on Reddit
   - Stack Overflow [esp32] tag

3. **Hardware debugging**:
   - Measure voltages
   - Check continuity
   - Verify component orientation

4. **Software debugging**:
   - Enable verbose logging
   - Use idf.py gdb
   - Add more ESP_LOGI statements

Good luck with your hardware testing! 🔧⚡
