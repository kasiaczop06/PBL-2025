#include <Arduino.h>




void setup() {
  // put your setup code here, to run once:
  Serial.begin(921600);
  pinMode(14,INPUT);
  pinMode(18,INPUT);
  pinMode(19, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

const int d1=32;
const int d2=33;
const int d3= 25;
const int d4= 21;
const int p1on= 12;
const int p1=14;
const int p2on=19;
const int p2=18;
int wcisniecia=0;
bool ostatnio=false;
bool ustawiono=false;


void p2czyWcisniety(){
  if(digitalRead(p2)==HIGH){
    ustawiono=true;
  }
}

void p1czyWcisniety() {
  if((digitalRead(p1)==HIGH)&&(ostatnio==false)){
    wcisniecia++;
    ostatnio=true;
  }else if(digitalRead(p1)==LOW){
    ostatnio=false;
  }
}

void ustawianie(){
  ustawiono=false;
  ostatnio=false;
  wcisniecia=0;
  while(ustawiono==false){
    switch(wcisniecia%3){
    case 0:
      digitalWrite(d1,HIGH);
      for(int i=0;i<20;i++){
        p1czyWcisniety();
        p2czyWcisniety();
        if((ostatnio==true)||(ustawiono==true)){
          break;
        }
        delay(10);
      }
      digitalWrite(d1,LOW);
      for(int i=0;i<20;i++){
        p1czyWcisniety();
        p2czyWcisniety();
        if((ostatnio==true)||(ustawiono==true)){
          break;
        }
        delay(10);
      }
      break;
    case 1:
      Serial.println("case1");
      digitalWrite(d2,HIGH);
      for(int i=0;i<20;i++){
        p1czyWcisniety();
        p2czyWcisniety();
        if((ostatnio==true)||(ustawiono==true)){
          break;
        }
        delay(10);
      }
      digitalWrite(d2,LOW);
      for(int i=0;i<20;i++){
        p1czyWcisniety();
        p2czyWcisniety();
        if((ostatnio==true)||(ustawiono==true)){
          Serial.println("break");
          //Serial.prinln("break");
          break;
        }
        delay(10);
      }
      break;
    case 2:
      Serial.println("case 2");
      digitalWrite(d3,HIGH);
      for(int i=0;i<20;i++){
        p1czyWcisniety();
        p2czyWcisniety();
        if((ostatnio==true)||(ustawiono==true)){
          break;
        }
        delay(10);
      }
      digitalWrite(d3,LOW);
      for(int i=0;i<20;i++){
        p1czyWcisniety();
        p2czyWcisniety();
        if((ostatnio==true)||(ustawiono==true)){
          break;
        }
        delay(10);
      }
      break;
    }
  }
}

void odliczanie(){
  digitalWrite(d1,LOW);
  digitalWrite(d2,LOW);
  digitalWrite(d3,LOW);
  for(int i=wcisniecia%3; i>=0; i--){
    switch(i){
      case 0:
        digitalWrite(d1, HIGH);
        break;
      case 1:
        digitalWrite(d2, HIGH);
        break;
      case 2:
        digitalWrite(d3, HIGH);
    }
    delay(10000);
    digitalWrite(d1,LOW);
    digitalWrite(d2,LOW);
    digitalWrite(d3,LOW);
  }
}

void alarm(){
  while (ustawiono==false){
    digitalWrite(d4, HIGH);
    for(int i=0;i<20;i++){
      delay(10);
      p2czyWcisniety();
      if(ustawiono==true){
        break;
      }
    }
    digitalWrite(d4, LOW);
    for(int i=0;i<20;i++){
      delay(10);
      p2czyWcisniety();
      if(ustawiono==true){
        break;
      }
    }
  }
}

void loop() {

  digitalWrite(LED_BUILTIN, HIGH);

  digitalWrite(p1on,HIGH);
  digitalWrite(p2on,HIGH);

  ustawianie();
  odliczanie();
  ustawiono=false;
  alarm();
  delay(2000);

}