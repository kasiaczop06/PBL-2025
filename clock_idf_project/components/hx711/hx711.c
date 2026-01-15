/*
 * HX711 ADC Driver for ESP-IDF
 * Implementation
 */

#include "hx711.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

static const char *TAG = "HX711";

// Mutex for critical section
static portMUX_TYPE hx711_mux = portMUX_INITIALIZER_UNLOCKED;

// Delay in microseconds
static void delay_us(uint32_t us)
{
    ets_delay_us(us);
}

void hx711_init(hx711_t *hx711)
{
    // Configure GPIO pins
    gpio_config_t io_conf = {};

    // Configure DOUT as input
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << hx711->dout);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Configure PD_SCK as output
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << hx711->pd_sck);
    gpio_config(&io_conf);

    // Set initial values
    gpio_set_level(hx711->pd_sck, 0);
    hx711->offset = 0;
    hx711->scale = 1.0;

    if (hx711->gain == 0) {
        hx711->gain = HX711_GAIN_A_128;
    }

    ESP_LOGI(TAG, "HX711 initialized");
}

bool hx711_is_ready(hx711_t *hx711)
{
    return (gpio_get_level(hx711->dout) == 0);
}

void hx711_set_gain(hx711_t *hx711, hx711_gain_t gain)
{
    hx711->gain = gain;
    gpio_set_level(hx711->pd_sck, 0);
    hx711_read(hx711);
}

long hx711_read(hx711_t *hx711)
{
    // Wait for the chip to become ready
    int timeout = 0;
    while (!hx711_is_ready(hx711)) {
        vTaskDelay(pdMS_TO_TICKS(1));
        timeout++;
        if (timeout > 100) {
            ESP_LOGW(TAG, "HX711 timeout waiting for ready");
            return 0;
        }
    }

    unsigned long value = 0;
    uint8_t data[3] = {0};
    uint8_t filler = 0x00;

    // Pulse the clock pin 24 times to read the data
    taskENTER_CRITICAL(&hx711_mux);

    for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 8; i++) {
            gpio_set_level(hx711->pd_sck, 1);
            delay_us(1);
            data[j] = (data[j] << 1);
            if (gpio_get_level(hx711->dout)) {
                data[j]++;
            }
            gpio_set_level(hx711->pd_sck, 0);
            delay_us(1);
        }
    }

    // Set the gain for next reading
    for (unsigned int i = 0; i < hx711->gain; i++) {
        gpio_set_level(hx711->pd_sck, 1);
        delay_us(1);
        gpio_set_level(hx711->pd_sck, 0);
        delay_us(1);
    }

    taskEXIT_CRITICAL(&hx711_mux);

    // Construct the 24-bit value
    value = ((unsigned long)data[0] << 16) | ((unsigned long)data[1] << 8) | (unsigned long)data[2];

    // Check if the value is negative (two's complement)
    if (data[0] & 0x80) {
        filler = 0xFF;
    }

    // Extend to 32-bit signed value
    value = (unsigned long)(filler << 24) | value;

    return (long)value;
}

long hx711_read_average(hx711_t *hx711, uint8_t times)
{
    long sum = 0;
    for (uint8_t i = 0; i < times; i++) {
        sum += hx711_read(hx711);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return sum / times;
}

float hx711_get_value(hx711_t *hx711, uint8_t times)
{
    return (float)(hx711_read_average(hx711, times) - hx711->offset);
}

float hx711_get_units(hx711_t *hx711, uint8_t times)
{
    return hx711_get_value(hx711, times) / hx711->scale;
}

void hx711_tare(hx711_t *hx711, uint8_t times)
{
    long sum = hx711_read_average(hx711, times);
    hx711_set_offset(hx711, sum);
    ESP_LOGI(TAG, "Tared with offset: %ld", sum);
}

void hx711_set_scale(hx711_t *hx711, float scale)
{
    hx711->scale = scale;
}

float hx711_get_scale(hx711_t *hx711)
{
    return hx711->scale;
}

void hx711_set_offset(hx711_t *hx711, long offset)
{
    hx711->offset = offset;
}

long hx711_get_offset(hx711_t *hx711)
{
    return hx711->offset;
}

void hx711_power_down(hx711_t *hx711)
{
    gpio_set_level(hx711->pd_sck, 0);
    gpio_set_level(hx711->pd_sck, 1);
    delay_us(60);
}

void hx711_power_up(hx711_t *hx711)
{
    gpio_set_level(hx711->pd_sck, 0);
}
