//git push -u origin master dla git huba
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <HX711_ADC.h>
#include <Servo.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#pragma GCC optimize ("O3") // pszyspiesza kompilacje ale powoduje więcej błędów ewentualnie do usunięcia
#define COLUMNS 20
#define ROWS 4

const int servo_in=10; //połączenie servo
const int HX711_dout = 18; // połączenia wagi
const int HX711_sck = 5; // połączenia wagi
const char* ssid = "iPhone (Kasia)";
const char* password = "kasiakasia1357";
unsigned long t = 0;
unsigned long last = 0;
unsigned long alarm_czas = 0;
int p1wcisniecia=0, p2wcisniecia =0;
bool p3wcisniety =0; 
bool ostatnio1=0, ostatnio2=0;
bool alarmActive = false;
int w;
int p1 = 12;
int p2 = 13;
int p3_in = 14; 
int p3_out = 15; // do ustawienia 3 
int tab[5]; // tab{dzień, godzina, minuta, sekundy, ilość pieniędzy do uzbierania}
float i;

Servo lapka;
HX711_ADC LoadCell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE); // LiquidCrystal_I2C *lcd;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // Strefa czasowa +1 (CEST), update co 60s


void waga_setup(); 
bool waga_servo(float &i);
void glosnik();
void lapka_kota(int (&tab)[5]);
void p2czyWcisniety(int &p2wcisniecia, int &p1wcisniecia);
void p1czyWcisniety(int &p1wcisniecia, int &p2wcisniecia);
void p3czyWcisniety(bool &p3wcisniety);
void czas(int &w);
void alarm_ustaw(int (&tab)[5]);
void printLine_word(int row, int col, const String &text);
void printLine_num(int row, int col, int v);
void alarm_wys();
void wifi();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  lcd.begin(COLUMNS, ROWS, LCD_5x8DOTS);
  lcd.backlight();
  lcd.clear();

  wifi();

  pinMode(p1,INPUT);
  pinMode(p2,INPUT);
  pinMode(p3_in,INPUT);
  pinMode(p3_out,OUTPUT);

  waga_setup();
  lapka.attach(servo_in);

  printLine_word(0, 1, "Alarm: 00:00");
  p3wcisniety =0;
}

void loop() {
  if (millis() - last < 1000) {
  last = millis();
  czas(w);
  }
  tab[0]=w;
  p3czyWcisniety(p3wcisniety);
  if(p3wcisniety==1)  alarm_ustaw(tab);
}

void wifi()
{
WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.print("Połączono, IP: ");
    timeClient.begin();
    timeClient.update();
  }else{
    Serial.print("nie połączono się");
  }
}


void p1czyWcisniety(int &p1wcisniecia, int &p2wcisniecia) {
  if((digitalRead(p1)==HIGH)&&(ostatnio1==false)){
    p1wcisniecia++;
    if(p1wcisniecia==6){
      p1wcisniecia=0;
    }
    p2wcisniecia=0;
    ostatnio1=true;
  }else if(digitalRead(p1)==LOW){
    ostatnio1=false;
  }
}

void p2czyWcisniety(int &p2wcisniecia, int &p1wcisniecia){
  if((digitalRead(p2)==HIGH)&&(ostatnio2==false)){
    p2wcisniecia++;
    switch(p1wcisniecia)
    {
      case 1:
      p2wcisniecia=p2wcisniecia %24; break;
      case 2:
      case 3:
      p2wcisniecia=p2wcisniecia %60; break;
    }
    ostatnio2=true;
  }else if(digitalRead(p2)==LOW){
    ostatnio2=false;
  }
}

void p3czyWcisniety(bool &p3wcisniety)
{
  bool lastP3State=digitalRead(p3_in);  
  if(digitalRead(p3_in)==LOW){
    if(p3wcisniety==0){
      digitalWrite(p3_out, HIGH);
      p3wcisniety=1;
    }else{
      digitalWrite(p3_out, LOW);
      p3wcisniety=0;
    }
  }

}

void alarm_ustaw(int (&tab)[5])
{
  p1czyWcisniety(p1wcisniecia, p2wcisniecia);
  p2czyWcisniety(p2wcisniecia, p1wcisniecia);
  if(p1wcisniecia>0){
  tab[p1wcisniecia-1]=p2wcisniecia;
    if(tab[0]>=7)   {tab[0]=0;}
    if(tab[1]>=24)  {tab[0]++; tab[1]=0;}
    if(tab[2]>=60)  {tab[1]++; tab[2]=0;}
    if(tab[3]>=60)  {tab[2]++; tab[3]=0;}
    alarm_wys();
  }

}


void alarm_wys()
{
  printLine_num(7, 1, tab[1]);
  printLine_num(10, 1, tab[2]);
  printLine_num(13, 1, tab[3]);
}

void printLine_word(int row,int col, const String &text) {
  // wyświetla tekst i dopełnia spacje do końca linii
  lcd.setCursor(col, row);
  String s = text;
  while (s.length() < COLUMNS) s += ' ';
  lcd.print(s);
}

void printLine_num(int row, int col, int v) {
  // wyświetla tekst i dopełnia spacje do końca linii
  lcd.setCursor(col, row);
  if(v<10){
    lcd.print("0");
  }
  lcd.print(v);
}
bool waga_servo(float &i)
{
     if (LoadCell.update()){
      i = LoadCell.getData(); 
      return true;
     }else{
      return false;
     }
}

void lapka_kota(int (&tab)[5])
{
  lapka.write(0); // ustawia servo na 0(stopni)
  unsigned long ze = millis();
  while(!waga_servo(i)){ // i trzyma go na 180 aż HX711 przestanie przeysłać dane
  lapka.write(180); //ustawia servo na 180(stopni)
  if(millis()-ze >2500) break;
  } 

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

void czas(int &w)
{
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    unsigned long epoch = timeClient.getEpochTime();
    time_t rawtime = (time_t)epoch;
    struct tm *tm = localtime(&rawtime);

    char timeBuf[9] = "--:--:--";     // HH:MM:SS
    char dateBuf[11] = "--/--/----"; // DD/MM/YYYY
    const char* dayBuf = "---";
    if (tm) {
  snprintf(timeBuf, sizeof(timeBuf),
           "%02d:%02d:%02d",
           tm->tm_hour, tm->tm_min, tm->tm_sec);

  snprintf(dateBuf, sizeof(dateBuf),
           "%02d/%02d/%04d",
           tm->tm_mday,
           tm->tm_mon + 1,
           tm->tm_year + 1900);

    w = tm->tm_wday;
  if (w >= 0 && w <= 6)
    dayBuf = (const char*[7]){
      "Niedziela","Poniedzialek","Wtorek",
      "Sroda","Czwartek","Piatek","Sobota"
    }[w];
}


    
    
    // Czyścimy ekran i piszemy prosto i na środku
    lcd.clear();
    char alarmBuf[9];
    snprintf(alarmBuf, sizeof(alarmBuf), 
              "%02d:%02d:%02d", 
              tab[1], tab[2], tab[3]);
    printLine_word(0, 3, "ALARM: "); lcd.print(alarmBuf);
    lcd.setCursor((COLUMNS - 8) / 2, 1); lcd.print(timeBuf);
    lcd.setCursor(0, 2); lcd.print(dayBuf);
    lcd.setCursor(10, 2); lcd.print(dateBuf);
    lcd.setCursor(0, 0); lcd.print("====================");
    if(waga_servo(i)==1)  {lapka_kota(tab);}
    if(p3wcisniety==0){
    if((tab[0]==tm->tm_mday)&&(tab[1]==tm->tm_hour)&&(tab[2]==tm->tm_min)&&(tab[3]==tm->tm_sec))
  {
    if(tab[4]>=0)  alarmActive = true;
  }
    if(alarmActive && millis() - alarm_czas > 500){ 
    glosnik();
    alarm_czas = millis();
    if(tab[4] <= 0) alarmActive = false;
}
  }
}
else {
    // brak WiFi
    lcd.clear();
    printLine_word(5, 0, "==========");
    printLine_word(4, 1, "Brak WiFi");
    printLine_word(3, 2, "Sprawdz siec");
    printLine_word(5, 3, "==========");

    WiFi.reconnect();
  }
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
     tone(glosnik_in , melody[thisNote], noteDuration * 0.9); // ewentualnie ledcWriteTone() jak przestanie działać w testach 
     delay(noteDuration);
     noTone(glosnik_in);
}
}

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
