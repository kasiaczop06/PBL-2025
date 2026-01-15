/*
 * HX711 ADC Driver for ESP-IDF
 * Based on HX711_ADC Arduino library
 */

#ifndef HX711_H
#define HX711_H

#include "driver/gpio.h"
#include <stdint.h>
#include <stdbool.h>

// HX711 gain options
typedef enum {
    HX711_GAIN_A_128 = 1,  // Channel A, gain 128
    HX711_GAIN_B_32 = 2,   // Channel B, gain 32
    HX711_GAIN_A_64 = 3    // Channel A, gain 64
} hx711_gain_t;

typedef struct {
    gpio_num_t dout;        // Data output pin
    gpio_num_t pd_sck;      // Power down and serial clock pin
    hx711_gain_t gain;      // Gain selection
    long offset;            // Offset for tare
    float scale;            // Scale factor for calibration
} hx711_t;

// Function prototypes
void hx711_init(hx711_t *hx711);
bool hx711_is_ready(hx711_t *hx711);
void hx711_set_gain(hx711_t *hx711, hx711_gain_t gain);
long hx711_read(hx711_t *hx711);
long hx711_read_average(hx711_t *hx711, uint8_t times);
float hx711_get_value(hx711_t *hx711, uint8_t times);
float hx711_get_units(hx711_t *hx711, uint8_t times);
void hx711_tare(hx711_t *hx711, uint8_t times);
void hx711_set_scale(hx711_t *hx711, float scale);
float hx711_get_scale(hx711_t *hx711);
void hx711_set_offset(hx711_t *hx711, long offset);
long hx711_get_offset(hx711_t *hx711);
void hx711_power_down(hx711_t *hx711);
void hx711_power_up(hx711_t *hx711);

#endif // HX711_H
