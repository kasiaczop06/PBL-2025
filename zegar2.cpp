//git push -u origin master dla git huba
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <HX711_ADC.h>
#include <Servo.h>
#include "pitches.h"
#include <WiFi.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#pragma GCC optimize ("O3") // pszyspiesza kompilacje ale powoduje więcej błędów
#define COLUMNS 20
#define ROWS 4

const int servo_in=10; //połączenie servo
const int HX711_dout = 18; // połączenia wagi
const int HX711_sck = 5; // połączenia wagi
const char* ssid = "iPhone (Kasia)";
const char* password = "kasiakasia1357";
unsigned long t = 0;
int p1wcisniecia=0, p2wcisniecia =0;
bool p3wcisniecia =0;
bool ostatnio1=0, ostatnio2=0, ostatnio3 =0;
int h, m, s, day;
int p1 = 12;
int p2 = 13;
int p3 = 14;  // do ustawienia 3 
int tab[5];
float i;

Servo lapka;
HX711_ADC LoadCell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // Strefa czasowa +1 (CEST), update co 60s



void lcd_setup(){ // do testu 
    lcd.setCursor(0, 0);
    lcd.print(F("00:00")); // print(F()) nie wrzuca wartości do pamięci ramu 
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.print(F("00:00"));
    delay(1000);
}

void waga_lcd_wypisz(){ // do testu
  const int f = 500;  // częstotliwość  przeysłania 
    if (LoadCell.update()&&(millis() > t + f)) { //LoadCell.update() - sprawdza czy HX711 posiada nowe informacje do oczytu a reszta to mini zegar
      float i = LoadCell.getData(); // 
      lcd.setCursor(8, 0);
      lcd.print(F("waga:"));
      lcd.setCursor(8, 1);
      lcd.print(i, 2); // wypisanie masy z dokładnością 2 miejsc po przecinku
      t = millis(); // resetuje t 
    }
    delay(1000);
}

String twoDigits(int v) {
  return (v < 10) ? "0" + String(v) : String(v); // zmienia czad godziny i minuty w wersje 0X kiedy X jest jednocyfrowy
}

void waga_setup();
bool waga_servo(float &i);
void glosnik();
void lapka_kota(int (&tab)[5], float i);
void p2czyWcisniety(int &p2wcisniecia, int p1wcisniecia);
void p1czyWcisniety(int &p1wcisniecia, int &p2wcisniecia);
void czas(int &h, int &m, int &s, int &day);
int alarm_ustaw(int (&tab)[5]);
void alaram_check();

void setup() {
  Wire.begin();
  lcd.begin(COLUMNS, ROWS, LCD_5x8DOTS);
  lapka.attach(servo_in);
  Serial.begin(115200);
  pinMode(p1,INPUT);
  pinMode(p2,INPUT);
  pinMode(p3,INPUT);
  waga_setup();
}

void loop() {
  czas(h, m, s, day);
    if(p3wcisniecia==1)  alaram_check();
  if(waga_servo(i)==1)  lapka_kota(tab, i);
}

void p1czyWcisniety(int &p1wcisniecia, int &p2wcisniecia) {
  if((digitalRead(p1)==HIGH)&&(ostatnio1==false)){
    p1wcisniecia++;
    if(p1wcisniecia==5){
      p1wcisniecia=0;
    }
    p2wcisniecia=0;
    ostatnio1=true;
  }else if(digitalRead(p1)==LOW){
    ostatnio1=false;
  }
}

void p2czyWcisniety(int &p2wcisniecia, int p1wcisniecia){
  if((digitalRead(p2)==HIGH)&&(ostatnio2==false)){
    p2wcisniecia++;
    switch(p1wcisniecia)
    {
      case 1:
      p2wcisniecia %24; break;
      case 2:
      p2wcisniecia %60; break;
      case 3:
      p2wcisniecia %60; break;
    }
    ostatnio2=true;
  }else if(digitalRead(p2)==LOW){
    ostatnio2=false;
  }
}


int alarm_ustaw(int (&tab)[5])
{
  p1czyWcisniety(p1wcisniecia, p2wcisniecia);
  p2czyWcisniety(p2wcisniecia, p1wcisniecia);
  tab[0]=tab[0]+day;
  if(p1wcisniecia>0){
  tab[p1wcisniecia-1]=p2wcisniecia;
    if(tab[0]>=7)   tab[0]=0;
    if(tab[1]>=24)  tab[0]=tab[0]+1;
    if(tab[2]>=60)  tab[1]=tab[1]+1;
    if(tab[3]>=60)  tab[2]=tab[2]+1;
  }
    return tab[4];
}
void alaram_check()
{
  int temp=alarm_ustaw(tab);
  if((tab[0]==day)&&(tab[1]==h)&&(tab[2]==m)&&(tab[3]==s))
  {
    while(temp<=0)
    {
      glosnik();
    }
  }
}

bool waga_servo(float &i)
{
     if (LoadCell.update()){
      i = LoadCell.getData(); 
      delay(500);
      return true;
     }else{
      return false;
     }
}
void lapka_kota(int (&tab)[5], float i)
{
  lapka.write(0); // ustawia servo na 0(stopni)
  do{
  lapka.write(180); //ustawia servo na 180(stopni)
  delay(1000);
  }while(waga_servo(i)==0); // i trzyma go na 180 aż HX711 przestanie przeysłać dane
  lapka.write(0); // ponownie na 0
  tab[4]=tab[4]-5;
}

void waga_setup()
{
  LoadCell.begin();
    float calibration; 
    calibration = 696.0;  // mnożnik napięcia na masę (do zmienienie ewentualnie)
    unsigned long stabbilization = 2000;  // ile program czeka na stabylizajce mostka
    boolean _tare = true;  // po włączeniu waga ustawia się na masę zero i po tym mierzy dopiero
    LoadCell.start(stabbilization, _tare);  //włączenie mierzenia
    LoadCell.setCalFactor(calibration);  // włączenie mnożnika
}

void czas(int &h, int &m, int &s, int &day)
{
  timeClient.update();
  h = timeClient.getHours();
  m = timeClient.getMinutes();
  s = timeClient.getSeconds();
  String line0 = "Czas: " + twoDigits(h) + ":" + twoDigits(m);//zdanie od czasu do wypisaniu na lcd
  while (line0.length() < COLUMNS) line0 += ' '; // patrzy czy string line0 wychodzi poza szerokość
  lcd.setCursor(0, 0);
  lcd.print(line0);



  day = timeClient.getDay(); // Dzień tygodnia (0-6)
  lcd.setCursor(0, 1);
  lcd.print("Dzien: ");
  const char* days[] = {"Nd","Pon","Wt","Sr","Cz","Pt","So"};
  lcd.setCursor(7, 1);
  lcd.print(days[day]); // day jako int
}


// głośnik ->
// definicja nut
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

  int melody[]{
  NOTE_C5,4, //1
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,

  NOTE_F5,2, NOTE_C5,4, //8 
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4,

  NOTE_F5,4, NOTE_F5,4, NOTE_F5,4,//17
  NOTE_E5,2, NOTE_E5,4,
  NOTE_F5,4, NOTE_E5,4, NOTE_D5,4,
  NOTE_C5,2, NOTE_A5,4,
  NOTE_AS5,4, NOTE_A5,4, NOTE_G5,4,
  NOTE_C6,4, NOTE_C5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4, 
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8, //27
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4,
  NOTE_F5,4, NOTE_F5,4, NOTE_F5,4,
  NOTE_E5,2, NOTE_E5,4,
  NOTE_F5,4, NOTE_E5,4, NOTE_D5,4,
  
  NOTE_C5,2, NOTE_A5,4,//36
  NOTE_AS5,4, NOTE_A5,4, NOTE_G5,4,
  NOTE_C6,4, NOTE_C5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4, 
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8, 
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8,//45
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, NOTE_C5,4,
  NOTE_F5,4, NOTE_F5,8, NOTE_G5,8, NOTE_F5,8, NOTE_E5,8,
  NOTE_D5,4, NOTE_D5,4, NOTE_D5,4,
  NOTE_G5,4, NOTE_G5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_E5,4, NOTE_C5,4, NOTE_C5,4,
  
  NOTE_A5,4, NOTE_A5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, //53
  NOTE_F5,4, NOTE_D5,4, NOTE_C5,8, NOTE_C5,8,
  NOTE_D5,4, NOTE_G5,4, NOTE_E5,4,
  NOTE_F5,2, REST,4
    };
        int tempo = 140;
    int glosnik_in = 11;
  
  int notes = sizeof(melody) / sizeof(melody[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  void glosnik(){
    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
      divider = melody[thisNote + 1];
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; 
    }
     tone(glosnik_in , melody[thisNote], noteDuration * 0.9);
     delay(noteDuration);
     noTone(glosnik_in);
}
}

