//git push -u origin master dla git huba
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <HX711_ADC.h>
#include <Servo.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
//#pragma GCC optimize ("O3") // pszyspiesza kompilacje ale powoduje więcej błędów ewentualnie do usunięcia
#define COLUMNS 20
#define ROWS 4

char timeBuf[9];     // HH:MM:SS
char dateBuf[11]; // DD/MM/YYYY
const char* dayBuf = "---";
char alarmBuf[13];
struct tm *tm;
int tempo = 140;
int glosnik_in = 17;
const int servo_in=20; //połączenie servo
const int HX711_dout = 18; // połączenia wagi
const int HX711_sck = 19; // połączenia wagi

const char* ssid = "iPhone (Kasia)";
const char* password = "kasiakasia1357";

unsigned long t = 0;
unsigned long last = 0;
unsigned long last3=0;
unsigned long alarm_czas = 0;
unsigned long l=millis();
int p1wcisniecia=0, p2wcisniecia =0;
bool p3wcisniety = false;
bool ostatnio1=0, ostatnio2=0;
int czas_ruchu_lapki=0;
int w;

int p1 = 0;//15;
int p2 = 1;//17;
int p3_in = 2;//4; 
int p3_out = 21;//16; 

int tab[4]={0, 0, 0, 0}; // tab{dzień, godzina, minuta, ilość pieniędzy do uzbierania}
float i=0;
byte last1;
byte last2 =LOW;

Servo lapka;
HX711_ADC LoadCell(HX711_dout, HX711_sck);
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE); // LiquidCrystal_I2C *lcd;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // Strefa czasowa +1 (CEST), update co 60s


void waga_setup(); 
bool spr_wage();
void glosnik();
void lapka_kota();
void p2czyWcisniety();
void p1czyWcisniety();
void p3czyWcisniety();
void czas();
void wys_czas();
void alarm_ustaw();
void spr_alarm();
void printLine_word(int row, int col, const String &text);
void printLine_num(int row, int col, int v);
void wifi();
void waga_lcd_wypisz();

void setup() {
  Serial.begin(115200);
  Serial.println("Start");
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
  lapka.write(0);

  p1wcisniecia=0;
  p2wcisniecia=0;
  last1 =  digitalRead(p3_in);
}

void loop() {
  if (millis() - last >= 1000) {
  last = millis();
  czas();
  if(spr_wage()==true) {czas_ruchu_lapki++;}else{
    czas_ruchu_lapki=0;
  }
  }
  p3czyWcisniety();

  if(millis()-last3 >=100){
    last3 = millis();
    wys_czas();
    if(digitalRead(p3_out)==HIGH) {
    alarm_ustaw();
    printLine_word(0, 0, "Do uzbierania:");
    printLine_num(16, 0, tab[3]);
  }else{
    printLine_word(0, 0, "Zebrane:");
    printLine_num(12, 0, i);
    printLine_word(14, 0, "/");
    printLine_num(15, 0, tab[3]);
  }
  }

  lapka_kota();
  spr_alarm();
}


void wifi()
{
WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  if(WiFi.status() == WL_CONNECTED){
    printLine_word(0,0, "Polaczono, IP: ");
    timeClient.begin();
    timeClient.update();
  }else{
    printLine_word(0,0, "nie Połączono, IP: ");
  }
}


void p1czyWcisniety() {
  if((digitalRead(p1)==HIGH)&&(ostatnio1==false)){
    p1wcisniecia++;
    if(p1wcisniecia>=5){
      p1wcisniecia=1;
    }
    p2wcisniecia=0;
    ostatnio1=true;
  }else if(digitalRead(p1)==LOW){
    ostatnio1=false;
  }
}

void p2czyWcisniety(){
  if((digitalRead(p2)==HIGH)&&(ostatnio2==false)){
    p2wcisniecia++;
    ostatnio2=true;
  }else if(digitalRead(p2)==LOW){
    ostatnio2=false;
  }
}

void p3czyWcisniety()
{
  if(millis() - l >=50 ){
  byte buttonstate = digitalRead(p3_in);
  if(buttonstate != last1){
    l = millis();
    last1 = buttonstate;
    if(buttonstate == LOW){
      if(last2 ==HIGH){
        last2 =LOW;
      } else{
            last2=HIGH;
      }
      digitalWrite(p3_out, last2);
    }
  }
}
}

void alarm_ustaw()
{
  if(tm==NULL) return;
  p1czyWcisniety();
  p2czyWcisniety();
  if(p1wcisniecia>=1 && p1wcisniecia <=5){
    if(p1wcisniecia==1){
       tab[0] =p2wcisniecia+tm->tm_wday;
    }else{
      tab[p1wcisniecia-1]=p2wcisniecia;
    }

    switch(p1wcisniecia){
      case 1: if(tab[0]>7)  {p2wcisniecia=0;} break;
      case 2: if(tab[1]>=24) {tab[0]++; p2wcisniecia=0;}  break;
      case 3: if(tab[2]>=60) {tab[1]++; p2wcisniecia=0;}  break;
      case 4: if(tab[3]>100) {p2wcisniecia=0;} break;
    }
  }

}

void printLine_word(int row,int col, const String &text) {
  // wyświetla tekst i dopełnia spacje do końca linii
  lcd.setCursor(row, col);
  String s = text;
  while (s.length() < COLUMNS) s += ' ';
  lcd.print(s);
}

void printLine_num(int row, int col, int v) {
  // wyświetla tekst i dopełnia spacje do końca linii
  lcd.setCursor(row, col);
  if(v<10){
    lcd.print("0");
  }
  lcd.print(v);
}

void waga_setup()
{
    LoadCell.begin();
    LoadCell.setSamplesInUse(10);
    float calibration; 
    calibration = -16347.25;  // mnożnik napięcia na masę (do zmienienie ewentualnie) //696.0
    unsigned long stabbilization = 2000;  // ile program czeka na stabylizajce mostka
    boolean _tare = true;  // po włączeniu waga ustawia się na masę zero i po tym mierzy dopiero
    LoadCell.start(stabbilization, _tare);  //włączenie mierzenia
    LoadCell.setCalFactor(calibration);  // włączenie mnożnika
}

void czas()
{
  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
    unsigned long epoch = timeClient.getEpochTime();
    time_t rawtime = (time_t)epoch;
    tm = localtime(&rawtime);

    if (tm) { 
  snprintf(timeBuf, sizeof(timeBuf),  "%02d:%02d:%02d",
           tm->tm_hour, tm->tm_min, tm->tm_sec);

  snprintf(dateBuf, sizeof(dateBuf),   "%02d/%02d/%04d",
           tm->tm_mday,  tm->tm_mon + 1,   tm->tm_year + 1900);
      w=tm->tm_wday;
  if (w >= 0 && w <= 6){
    dayBuf = (const char*[7]){
      "Niedziela","Poniedzialek","Wtorek",
      "Sroda","Czwartek","Piatek","Sobota"
    }[w];
  }
}
}else {
    // brak WiFi
    lcd.clear();
    printLine_word(5, 0, "==========");
    printLine_word(4, 1, "Brak WiFi");
    printLine_word(3, 2, "Sprawdz siec");
    printLine_word(5, 3, "==========");

    WiFi.reconnect();
  }
}

void wys_czas(){
    snprintf(alarmBuf, sizeof(alarmBuf), 
              "%02d:%02d:%02d", 
              tab[0],tab[1], tab[2]);
    printLine_word(0, 3, "ALARM: "); 
    lcd.setCursor(6, 3);  lcd.print(alarmBuf);
    lcd.setCursor((COLUMNS - 8) / 2, 1); lcd.print(timeBuf);
    lcd.setCursor(0, 2); lcd.print(dayBuf);
    lcd.setCursor(10, 2); lcd.print(dateBuf);
}

void spr_alarm(){
  if(tm==NULL) return;
    if(digitalRead(p3_out)==LOW){
      if((tab[0]<=tm->tm_mday)&&(tab[1]<=tm->tm_hour)&&(tab[2]<=tm->tm_min))
    {
        if(tab[3]<=i){
          //glosnik();
          Serial.println("alarm");
        }
        }
      }
    }

bool spr_wage() //patrzy 
{
   int temp_i;
     if (LoadCell.update()){
      temp_i = LoadCell.getData();
        if(temp_i>=0.5){
      return true;
     }else{
      return false;
     }
    }else{
      return false;
    }
}

void lapka_kota()
{
  int temp_i2;
  if(spr_wage()==true){
    lapka.write(90);
    if(czas_ruchu_lapki==7){
      temp_i2 = LoadCell.getData();
      if(temp_i2>=4.8 && temp_i2<=5.4){
       i +=5;
      } 
    }
  }else{
    lapka.write(0);
  }
}
