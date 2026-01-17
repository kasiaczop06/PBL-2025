#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <HX711_ADC.h>
#pragma GCC optimize ("O3")
#define COLUMNS 16
#define ROWS 2
const int HX711_dout = 18;
const int HX711_sck = 5;
HX711_ADC LoadCell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd(0x27, COLUMNS, ROWS);  // I2C address 0x27, try 0x3F if not working
const int calVal_eepromAdress = 0;
unsigned long t = 0;


void waga_setup(){
  LoadCell.begin();
    float calibrationValue; 
    calibrationValue = 696.0;
    unsigned long stabilizingtime = 2000;
    boolean _tare = true;
    LoadCell.start(stabilizingtime, _tare);
    LoadCell.setCalFactor(calibrationValue); 
}

void lcd_setup(){
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.println(F("00:00"));
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.println(F("00:00"));
    delay(1000);
}
void setup() {
  Serial.begin(115200);
  waga_setup();
  lcd_setup();
}
void loop() {
    const int serialPrintInterval = 500;
    if (LoadCell.update()&&(millis() > t + serialPrintInterval)) {
      float i = LoadCell.getData();
      lcd.setCursor(8, 0);
      lcd.println(F("waga:"));
      lcd.setCursor(8, 1);
      lcd.println(i, 2);
      t = millis();
    }
    delay(1000);
}
