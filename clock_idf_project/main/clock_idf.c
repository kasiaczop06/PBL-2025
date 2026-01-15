/*
 * ESP32 Clock with Scale and Servo - ESP-IDF Implementation
 * Converted from Arduino to ESP-IDF with FreeRTOS
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// Project includes
#include "hx711.h"
#include "lcd_i2c.h"

static const char *TAG = "CLOCK_IDF";

// Pin definitions
#define PIN_SPEAKER         GPIO_NUM_14
#define PIN_SERVO           GPIO_NUM_13
#define PIN_HX711_DOUT      GPIO_NUM_18
#define PIN_HX711_SCK       GPIO_NUM_19
#define PIN_BUTTON_1        GPIO_NUM_10
#define PIN_BUTTON_2        GPIO_NUM_9
#define PIN_BUTTON_3_IN     GPIO_NUM_20
#define PIN_BUTTON_3_OUT    GPIO_NUM_8

// LCD settings
#define LCD_COLUMNS         20
#define LCD_ROWS            4
#define I2C_MASTER_SCL_IO   GPIO_NUM_22
#define I2C_MASTER_SDA_IO   GPIO_NUM_21
#define I2C_MASTER_FREQ_HZ  100000
#define LCD_I2C_ADDR        0x27

// WiFi credentials
#define WIFI_SSID           "Xiaomi 11T Pro"
#define WIFI_PASS           "rodaknieznany"

// Servo PWM settings
#define SERVO_LEDC_TIMER    LEDC_TIMER_0
#define SERVO_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define SERVO_LEDC_CHANNEL  LEDC_CHANNEL_0
#define SERVO_FREQ          50  // 50 Hz for servo
#define SERVO_RESOLUTION    LEDC_TIMER_13_BIT

// Global variables
static char time_buf[9];      // HH:MM:SS
static char date_buf[32];     // DD/MM/YYYY (increased for safety)
static const char* day_buf = "---";
static char alarm_buf[13];
static struct tm current_time;

static int p1_presses = 0;
static int p2_presses = 0;
static bool last_p1_state = false;
static bool last_p2_state = false;
static uint8_t last_p3_state = 0;
static uint8_t p3_out_state = 0;

static int movement_time = 0;
static int alarm_tab[4] = {0, 0, 0, 0}; // day, hour, minute, target amount
static float collected_amount = 0;

static TickType_t last_p3_check = 0;

static bool wifi_connected = false;
static bool time_synced = false;
static SemaphoreHandle_t time_mutex = NULL;

// HX711 and LCD handles
static hx711_t scale;
static lcd_handle_t lcd = NULL;

// Function prototypes
static void wifi_init(void);
static void time_sync_notification_cb(struct timeval *tv);
static void initialize_sntp(void);
static void setup_gpio(void);
static void setup_servo(void);
static void servo_write_angle(int angle);
static void button_1_check(void);
static void button_2_check(void);
static void button_3_check(void);
static void alarm_setup(void);
static void update_time(void);
static void display_time(void);
static void check_alarm(void);
static bool check_scale(void);
static void cat_paw_control(void);
static void print_line_word(int row, int col, const char *text);
static void print_line_num(int row, int col, int v);
static void setup_scale(void);

// FreeRTOS tasks
static void time_update_task(void *pvParameters);
static void display_task(void *pvParameters);
static void button_task(void *pvParameters);
static void scale_servo_task(void *pvParameters);

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from WiFi, reconnecting...");
        wifi_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected to WiFi, IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

static void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");
}

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized");
    time_synced = true;
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb = time_sync_notification_cb;

    esp_err_t ret = esp_netif_sntp_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SNTP: %s", esp_err_to_name(ret));
    }
}

static void setup_gpio(void)
{
    gpio_config_t io_conf = {};

    // Configure output pins
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BUTTON_3_OUT);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Configure input pins
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = ((1ULL << PIN_BUTTON_1) | (1ULL << PIN_BUTTON_2) | (1ULL << PIN_BUTTON_3_IN));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    gpio_set_level(PIN_BUTTON_3_OUT, 0);
}

static void setup_servo(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = SERVO_LEDC_MODE,
        .timer_num        = SERVO_LEDC_TIMER,
        .duty_resolution  = SERVO_RESOLUTION,
        .freq_hz          = SERVO_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = SERVO_LEDC_MODE,
        .channel        = SERVO_LEDC_CHANNEL,
        .timer_sel      = SERVO_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_SERVO,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

static void servo_write_angle(int angle)
{
    // Convert angle (0-180) to duty cycle
    // For 50Hz PWM, pulse width: 0.5ms (0°) to 2.5ms (180°)
    // With 13-bit resolution: 8192 levels
    int min_duty = 205;  // ~0.5ms
    int max_duty = 1024; // ~2.5ms
    int duty = min_duty + (angle * (max_duty - min_duty) / 180);

    ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
    ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
}

static void setup_scale(void)
{
    scale.dout = PIN_HX711_DOUT;
    scale.pd_sck = PIN_HX711_SCK;
    scale.gain = HX711_GAIN_A_64;

    hx711_init(&scale);

    // Wait for stabilization
    vTaskDelay(pdMS_TO_TICKS(2000));

    hx711_tare(&scale, 10);
    hx711_set_scale(&scale, -16347.25); // Calibration factor from Arduino code

    ESP_LOGI(TAG, "Scale initialized");
}

static void button_1_check(void)
{
    bool current = (gpio_get_level(PIN_BUTTON_1) == 1);

    if (current && !last_p1_state) {
        p1_presses++;
        if (p1_presses >= 5) {
            p1_presses = 1;
        }
        p2_presses = 0;
        last_p1_state = true;
    } else if (!current) {
        last_p1_state = false;
    }
}

static void button_2_check(void)
{
    bool current = (gpio_get_level(PIN_BUTTON_2) == 1);

    if (current && !last_p2_state) {
        p2_presses++;
        last_p2_state = true;
    } else if (!current) {
        last_p2_state = false;
    }
}

static void button_3_check(void)
{
    TickType_t now = xTaskGetTickCount();

    if ((now - last_p3_check) >= pdMS_TO_TICKS(50)) {
        uint8_t button_state = gpio_get_level(PIN_BUTTON_3_IN);

        if (button_state != last_p3_state) {
            last_p3_check = now;
            last_p3_state = button_state;

            if (button_state == 0) { // Button pressed (LOW)
                if (p3_out_state == 1) {
                    p3_out_state = 0;
                } else {
                    p3_out_state = 1;
                }
                gpio_set_level(PIN_BUTTON_3_OUT, p3_out_state);
            }
        }
    }
}

static void alarm_setup(void)
{
    button_1_check();
    button_2_check();

    if (p1_presses >= 1 && p1_presses <= 5) {
        if (p1_presses == 1) {
            alarm_tab[0] = p2_presses + current_time.tm_wday;
        } else {
            alarm_tab[p1_presses - 1] = p2_presses;
        }

        switch (p1_presses) {
            case 1:
                if (alarm_tab[0] > 7) {
                    p2_presses = 0;
                }
                break;
            case 2:
                if (alarm_tab[1] >= 24) {
                    alarm_tab[0]++;
                    p2_presses = 0;
                }
                break;
            case 3:
                if (alarm_tab[2] >= 60) {
                    alarm_tab[1]++;
                    p2_presses = 0;
                }
                break;
            case 4:
                if (alarm_tab[3] > 100) {
                    p2_presses = 0;
                }
                break;
        }
    }
}

static void print_line_word(int row, int col, const char *text)
{
    lcd_set_cursor(lcd, row, col);
    char buffer[LCD_COLUMNS + 1];
    snprintf(buffer, sizeof(buffer), "%-*s", LCD_COLUMNS, text);
    lcd_print(lcd, buffer);
}

static void print_line_num(int row, int col, int v)
{
    lcd_set_cursor(lcd, row, col);
    if (v < 10) {
        lcd_print(lcd, "0");
    }
    char num_str[10];
    snprintf(num_str, sizeof(num_str), "%d", v);
    lcd_print(lcd, num_str);
}

static void update_time(void)
{
    if (wifi_connected && time_synced) {
        time_t now;
        time(&now);

        if (xSemaphoreTake(time_mutex, portMAX_DELAY)) {
            localtime_r(&now, &current_time);

            snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d",
                    current_time.tm_hour, current_time.tm_min, current_time.tm_sec);

            snprintf(date_buf, sizeof(date_buf), "%02d/%02d/%04d",
                    current_time.tm_mday, current_time.tm_mon + 1, current_time.tm_year + 1900);

            int w = current_time.tm_wday;
            if (w >= 0 && w <= 6) {
                const char *days[7] = {
                    "Niedziela", "Poniedzialek", "Wtorek",
                    "Sroda", "Czwartek", "Piatek", "Sobota"
                };
                day_buf = days[w];
            }

            xSemaphoreGive(time_mutex);
        }
    } else {
        lcd_clear(lcd);
        print_line_word(5, 0, "==========");
        print_line_word(4, 1, "Brak WiFi");
        print_line_word(3, 2, "Sprawdz siec");
        print_line_word(5, 3, "==========");
    }
}

static void display_time(void)
{
    snprintf(alarm_buf, sizeof(alarm_buf), "%02d:%02d:%02d",
            alarm_tab[0], alarm_tab[1], alarm_tab[2]);

    print_line_word(0, 3, "ALARM: ");
    lcd_set_cursor(lcd, 6, 3);
    lcd_print(lcd, alarm_buf);

    lcd_set_cursor(lcd, (LCD_COLUMNS - 8) / 2, 1);
    lcd_print(lcd, time_buf);

    lcd_set_cursor(lcd, 0, 2);
    lcd_print(lcd, day_buf);

    lcd_set_cursor(lcd, 10, 2);
    lcd_print(lcd, date_buf);
}

static void check_alarm(void)
{
    if (!time_synced) return;

    if (gpio_get_level(PIN_BUTTON_3_OUT) == 0) {
        if ((alarm_tab[0] <= current_time.tm_mday) &&
            (alarm_tab[1] <= current_time.tm_hour) &&
            (alarm_tab[2] <= current_time.tm_min)) {
            if (alarm_tab[3] <= collected_amount) {
                ESP_LOGI(TAG, "ALARM!");
            }
        }
    }
}

static bool check_scale(void)
{
    float weight = hx711_get_units(&scale, 10);

    if (weight >= 0.5) {
        return true;
    }
    return false;
}

static void cat_paw_control(void)
{
    if (check_scale()) {
        servo_write_angle(90);

        if (movement_time == 7) {
            float weight = hx711_get_units(&scale, 10);
            if (weight >= 4.8 && weight <= 5.4) {
                collected_amount += 5;
            }
        }
    } else {
        servo_write_angle(0);
    }
}

// FreeRTOS Tasks

static void time_update_task(void *pvParameters)
{
    while (1) {
        update_time();

        if (check_scale()) {
            movement_time++;
        } else {
            movement_time = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void display_task(void *pvParameters)
{
    while (1) {
        display_time();

        if (gpio_get_level(PIN_BUTTON_3_OUT) == 1) {
            alarm_setup();
            print_line_word(0, 0, "Do uzbierania:");
            print_line_num(16, 0, alarm_tab[3]);
        } else {
            print_line_word(0, 0, "Zebrane:");
            print_line_num(12, 0, (int)collected_amount);
            print_line_word(14, 0, "/");
            print_line_num(15, 0, alarm_tab[3]);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void button_task(void *pvParameters)
{
    while (1) {
        button_3_check();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void scale_servo_task(void *pvParameters)
{
    while (1) {
        cat_paw_control();
        check_alarm();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP32 Clock starting...");

    // Create mutex for time access
    time_mutex = xSemaphoreCreateMutex();

    // Initialize peripherals
    setup_gpio();
    setup_servo();
    servo_write_angle(0);

    // Initialize I2C and LCD
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));

    lcd = lcd_init(I2C_NUM_0, LCD_I2C_ADDR, LCD_ROWS, LCD_COLUMNS);
    lcd_backlight_on(lcd);
    lcd_clear(lcd);

    // Initialize WiFi
    wifi_init();

    // Wait for WiFi connection
    int wait_count = 0;
    while (!wifi_connected && wait_count < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        wait_count++;
    }

    if (wifi_connected) {
        print_line_word(0, 0, "Polaczono WiFi");
        initialize_sntp();

        // Set timezone to CET (UTC+1)
        setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
        tzset();
    } else {
        print_line_word(0, 0, "Blad WiFi");
    }

    // Initialize scale
    setup_scale();

    // Create FreeRTOS tasks
    xTaskCreate(time_update_task, "time_update", 4096, NULL, 5, NULL);
    xTaskCreate(display_task, "display", 4096, NULL, 4, NULL);
    xTaskCreate(button_task, "button", 2048, NULL, 3, NULL);
    xTaskCreate(scale_servo_task, "scale_servo", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "All tasks created, system running");
}
