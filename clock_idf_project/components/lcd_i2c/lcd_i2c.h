/*
 * LCD I2C Driver for ESP-IDF
 * Based on LiquidCrystal_I2C Arduino library
 */

#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "driver/i2c.h"
#include <stdint.h>
#include <stdbool.h>

// LCD Commands
#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT    0x10
#define LCD_FUNCTIONSET    0x20
#define LCD_SETCGRAMADDR   0x40
#define LCD_SETDDRAMADDR   0x80

// Flags for display entry mode
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// Flags for display on/off control
#define LCD_DISPLAYON  0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON   0x02
#define LCD_CURSOROFF  0x00
#define LCD_BLINKON    0x01
#define LCD_BLINKOFF   0x00

// Flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT   0x04
#define LCD_MOVELEFT    0x00

// Flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE    0x08
#define LCD_1LINE    0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS  0x00

// Flags for backlight control
#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0x04  // Enable bit
#define Rw 0x02  // Read/Write bit
#define Rs 0x01  // Register select bit

typedef struct {
    i2c_port_t i2c_port;
    uint8_t i2c_addr;
    uint8_t rows;
    uint8_t cols;
    uint8_t backlight_val;
    uint8_t displayfunction;
    uint8_t displaycontrol;
    uint8_t displaymode;
} lcd_t;

typedef lcd_t* lcd_handle_t;

// Function prototypes
lcd_handle_t lcd_init(i2c_port_t i2c_port, uint8_t addr, uint8_t rows, uint8_t cols);
void lcd_destroy(lcd_handle_t lcd);
void lcd_clear(lcd_handle_t lcd);
void lcd_home(lcd_handle_t lcd);
void lcd_set_cursor(lcd_handle_t lcd, uint8_t col, uint8_t row);
void lcd_display_on(lcd_handle_t lcd);
void lcd_display_off(lcd_handle_t lcd);
void lcd_cursor_on(lcd_handle_t lcd);
void lcd_cursor_off(lcd_handle_t lcd);
void lcd_blink_on(lcd_handle_t lcd);
void lcd_blink_off(lcd_handle_t lcd);
void lcd_backlight_on(lcd_handle_t lcd);
void lcd_backlight_off(lcd_handle_t lcd);
void lcd_print(lcd_handle_t lcd, const char* str);
void lcd_write_char(lcd_handle_t lcd, uint8_t value);
void lcd_create_char(lcd_handle_t lcd, uint8_t location, uint8_t charmap[]);

#endif // LCD_I2C_H
