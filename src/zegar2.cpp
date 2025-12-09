#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#pragma GCC optimize ("O3")
#define COLUMS 16
#define ROWS 2
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
void startScreen(){
    lcd.setCursor(0, 0);
    Serial.println("00:");
    delay(1000);
    lcd.setCursor(0, 1);
    Serial.println("00:");
    delay(1000);
}
void setup() {
  Serial.begin(115200);
  lcd.begin(COLUMS, ROWS, LCD_5x8DOTS);
  lcd.setCursor(0, 1);
  lcd.println(F("00:"));
  delay(1000);
}
void loop() {
}