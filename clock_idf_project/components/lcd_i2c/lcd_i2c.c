/*
 * LCD I2C Driver for ESP-IDF
 * Implementation
 */

#include "lcd_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "LCD_I2C";

#define I2C_MASTER_TIMEOUT_MS 1000

// Low level functions
static esp_err_t lcd_i2c_write(lcd_handle_t lcd, uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (lcd->i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(lcd->i2c_port, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void lcd_expand_write(lcd_handle_t lcd, uint8_t data)
{
    lcd_i2c_write(lcd, data | lcd->backlight_val);
}

static void lcd_pulse_enable(lcd_handle_t lcd, uint8_t data)
{
    lcd_expand_write(lcd, data | En);
    vTaskDelay(pdMS_TO_TICKS(1));

    lcd_expand_write(lcd, data & ~En);
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void lcd_write_4bits(lcd_handle_t lcd, uint8_t value)
{
    lcd_expand_write(lcd, value);
    lcd_pulse_enable(lcd, value);
}

static void lcd_send(lcd_handle_t lcd, uint8_t value, uint8_t mode)
{
    uint8_t highnib = value & 0xF0;
    uint8_t lownib = (value << 4) & 0xF0;
    lcd_write_4bits(lcd, highnib | mode);
    lcd_write_4bits(lcd, lownib | mode);
}

static void lcd_command(lcd_handle_t lcd, uint8_t value)
{
    lcd_send(lcd, value, 0);
}

static void lcd_write(lcd_handle_t lcd, uint8_t value)
{
    lcd_send(lcd, value, Rs);
}

// Public API functions

lcd_handle_t lcd_init(i2c_port_t i2c_port, uint8_t addr, uint8_t rows, uint8_t cols)
{
    lcd_handle_t lcd = (lcd_handle_t)malloc(sizeof(lcd_t));
    if (lcd == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for LCD");
        return NULL;
    }

    lcd->i2c_port = i2c_port;
    lcd->i2c_addr = addr;
    lcd->rows = rows;
    lcd->cols = cols;
    lcd->backlight_val = LCD_BACKLIGHT;

    lcd->displayfunction = LCD_4BITMODE | LCD_2LINE | LCD_5x8DOTS;

    // Wait for LCD to power up
    vTaskDelay(pdMS_TO_TICKS(50));

    // Initialize LCD in 4-bit mode
    lcd_expand_write(lcd, lcd->backlight_val);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Put LCD into 4-bit mode
    lcd_write_4bits(lcd, 0x03 << 4);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_write_4bits(lcd, 0x03 << 4);
    vTaskDelay(pdMS_TO_TICKS(5));

    lcd_write_4bits(lcd, 0x03 << 4);
    vTaskDelay(pdMS_TO_TICKS(1));

    lcd_write_4bits(lcd, 0x02 << 4);

    // Set function
    lcd_command(lcd, LCD_FUNCTIONSET | lcd->displayfunction);

    // Turn on display with no cursor and no blinking
    lcd->displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    lcd_display_on(lcd);

    // Clear display
    lcd_clear(lcd);

    // Set entry mode
    lcd->displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    lcd_command(lcd, LCD_ENTRYMODESET | lcd->displaymode);

    lcd_home(lcd);

    ESP_LOGI(TAG, "LCD initialized successfully");
    return lcd;
}

void lcd_destroy(lcd_handle_t lcd)
{
    if (lcd != NULL) {
        free(lcd);
    }
}

void lcd_clear(lcd_handle_t lcd)
{
    lcd_command(lcd, LCD_CLEARDISPLAY);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_home(lcd_handle_t lcd)
{
    lcd_command(lcd, LCD_RETURNHOME);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_set_cursor(lcd_handle_t lcd, uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if (row >= lcd->rows) {
        row = lcd->rows - 1;
    }
    lcd_command(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void lcd_display_on(lcd_handle_t lcd)
{
    lcd->displaycontrol |= LCD_DISPLAYON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void lcd_display_off(lcd_handle_t lcd)
{
    lcd->displaycontrol &= ~LCD_DISPLAYON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void lcd_cursor_on(lcd_handle_t lcd)
{
    lcd->displaycontrol |= LCD_CURSORON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void lcd_cursor_off(lcd_handle_t lcd)
{
    lcd->displaycontrol &= ~LCD_CURSORON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void lcd_blink_on(lcd_handle_t lcd)
{
    lcd->displaycontrol |= LCD_BLINKON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void lcd_blink_off(lcd_handle_t lcd)
{
    lcd->displaycontrol &= ~LCD_BLINKON;
    lcd_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

void lcd_backlight_on(lcd_handle_t lcd)
{
    lcd->backlight_val = LCD_BACKLIGHT;
    lcd_expand_write(lcd, 0);
}

void lcd_backlight_off(lcd_handle_t lcd)
{
    lcd->backlight_val = LCD_NOBACKLIGHT;
    lcd_expand_write(lcd, 0);
}

void lcd_print(lcd_handle_t lcd, const char* str)
{
    while (*str) {
        lcd_write(lcd, *str++);
    }
}

void lcd_write_char(lcd_handle_t lcd, uint8_t value)
{
    lcd_write(lcd, value);
}

void lcd_create_char(lcd_handle_t lcd, uint8_t location, uint8_t charmap[])
{
    location &= 0x7;
    lcd_command(lcd, LCD_SETCGRAMADDR | (location << 3));
    for (int i = 0; i < 8; i++) {
        lcd_write(lcd, charmap[i]);
    }
}
