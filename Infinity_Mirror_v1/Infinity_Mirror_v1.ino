#include <Wire.h>
#include "RTClib.h"
#include "FastLED.h"
#include <SoftwareSerial.h>
#include <SerialCommand.h>
#include "EEPROM.h"
#include "TimerOne.h"

#define NUM_LEDS 60
#define DATA_PIN 13
#define LDRPIN A3
CRGB leds[NUM_LEDS],minutes,hours,seconds,l,bg;
RTC_DS1307 rtc;
SerialCommand sCmd;
boolean missed=0, ledState = 1, lastsec=1;
byte lastsecond, rain;
int light_low, light_high;

void(* resetFunc) (void) = 0;

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  rtc.begin();
  Timer1.initialize();
  Timer1.attachInterrupt(state, 500000);
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  if (EEPROM.read(99) != 1){               // Check if colours have been set or not
    EEPROM.write(0,255);                   // Seconds Colour - R-G-B - White
    EEPROM.write(1,255);
    EEPROM.write(2,255);
    EEPROM.write(3,255);                   // Minutes Colour - R-G-B - Red
    EEPROM.write(4,0);
    EEPROM.write(5,0);
    EEPROM.write(6,0);                     // Hours Colour - R-G-B - Green
    EEPROM.write(7,255);
    EEPROM.write(8,0);
    EEPROM.write(9,0);                     // BG Colour - R-G-B - Black
    EEPROM.write(10,0);
    EEPROM.write(11,0);
    EEPROM.write(12, 0);                   // Light sensitivity - low
    EEPROM.write(13, 55);                  // Light sensitivity - high 
    EEPROM.write(14, 15);                  // Minutes for each rainbow   
    EEPROM.write(99,1);
  } 
  seconds.r = EEPROM.read(0);
  seconds.g = EEPROM.read(1);
  seconds.b = EEPROM.read(2);  
  minutes.r = EEPROM.read(3);
  minutes.g = EEPROM.read(4);
  minutes.b = EEPROM.read(5);
  hours.r = EEPROM.read(6);
  hours.g = EEPROM.read(7);
  hours.b = EEPROM.read(8);
  bg.r = EEPROM.read(9);
  bg.g = EEPROM.read(10);
  bg.b = EEPROM.read(11);
  light_low = EEPROM.read(12);
  light_high = EEPROM.read(13); 
  rain = EEPROM.read(14); 
  sCmd.addCommand("STAT", clockstatus);
  sCmd.addCommand("SETRAIN", set_rainbow);
  sCmd.addCommand("MISSED", missedCall);
  sCmd.addCommand("MISSEDOFF", missedOff);
  sCmd.addCommand("HOUR", set_hour);
  sCmd.addCommand("MIN", set_minute);
  sCmd.addCommand("SEC", set_second);
  sCmd.addCommand("BG", set_bg);
  sCmd.addCommand("RAINBOW", effects);
  sCmd.addCommand("LIGHT", set_light);
  sCmd.addCommand("TIME", set_time);
  sCmd.addDefaultHandler(effects);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = bg;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  sCmd.readSerial();
  if(ledState){
    DateTime now =  rtc.now();// DateTime(2014,5,2,22,20,0); //
    int x = analogRead(LDRPIN);
    // Serial.println(x);
    x = map(x,light_low,light_high,20,100);
    x = constrain(x,20,100);
    if(( now.minute() % rain == 0 && now.second() == 0)){
       effects();
    }
    for(byte i=0; i<=now.minute();i++){
      //Serial.println(minutes);
      leds[i] = minutes;
    }
    //Serial.println(now.hour(),DEC);
    for(byte i=(now.hour()%12)*5; i<=((now.hour())%12)*5+(now.minute()/15);i++){
      leds[i] = hours;
    }
    LEDS.setBrightness(x);
    if(lastsec){
      l=leds[now.second()];
      leds[now.second()] = seconds;
      lastsecond = now.second();
      lastsec = 0;
      // Serial.println("ON");
    } else {
      leds[lastsecond] = l;
      if(missed) all_off();
      // Serial.println("OFF");
      lastsec = 1;
    }
    FastLED.show();
    ledState = 0;
  }
  //delay(250);
} 

void set_rainbow(){
  rain = atoi(sCmd.next());
  EEPROM.write(14,rain);
  Serial.println("RAINBOW TIME SET");
}
void clockstatus(){
  Serial.println("Status: ");
  Serial.print("BG: ");
  Serial.print(bg.r);
  Serial.print(" ");
  Serial.print(bg.g);
  Serial.print(" ");
  Serial.println(bg.b);
  Serial.print("SEC: ");
  Serial.print(seconds.r);
  Serial.print(" ");
  Serial.print(seconds.g);
  Serial.print(" ");
  Serial.println(seconds.b);
  Serial.print("MINUTE: ");
  Serial.print(minutes.r);
  Serial.print(" ");
  Serial.print(minutes.g);
  Serial.print(" ");
  Serial.println(minutes.b);
  Serial.print("HOUR: ");
  Serial.print(hours.r);
  Serial.print(" ");
  Serial.print(hours.g);
  Serial.print(" ");
  Serial.println(hours.b);
  Serial.print("Ambient Light: ");
  Serial.println(analogRead(LDRPIN));
  Serial.print("Date: ");
  DateTime now =  rtc.now(); // DateTime(2014,5,2,22,30,0); 
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.println(now.year(), DEC);
  Serial.print("Time: ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

void state(){
  ledState = 1;
}

const int colorWheelAngle = 255 / NUM_LEDS;

void effects(){
  Serial.println("RAINBOW");
  // LEDS.setBrightness(100);
  // uint8_t color[3];
  for (int i = 0; i < 300; i++) { 
    for (byte j = 0; j < NUM_LEDS; j++) {
      leds[j]=CHSV(i+j*colorWheelAngle,255,150);
    }
    FastLED.show();
    delay(2);
  }
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = bg;
  } 
  lastsec = 1;
  // FastLED.show();
}

void missedCall()
{
    missed = 1;
}

void missedOff()
{
    missed = 0;
}

void all_off(){
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

void set_hour(){
  hours.r = atoi(sCmd.next());
  hours.g = atoi(sCmd.next());
  hours.b = atoi(sCmd.next());
  EEPROM.write(6,hours.r);
  EEPROM.write(7,hours.g);
  EEPROM.write(8,hours.b);
  Serial.println("HOUR COLOUR SET");  
}

void set_minute(){
  minutes.r = atoi(sCmd.next());
  minutes.g = atoi(sCmd.next());
  minutes.b = atoi(sCmd.next());
  EEPROM.write(3,minutes.r);
  EEPROM.write(4,minutes.g);
  EEPROM.write(5,minutes.b); 
  Serial.println("MINUTE COLOUR SET");  
}

void set_second(){
  seconds.r = atoi(sCmd.next());
  seconds.g = atoi(sCmd.next());
  seconds.b = atoi(sCmd.next());
  EEPROM.write(0,seconds.r);
  EEPROM.write(1,seconds.g);
  EEPROM.write(2,seconds.b);
  Serial.println("SECOND COLOUR SET"); 
}

void set_bg(){
  bg.r = atoi(sCmd.next());
  bg.g = atoi(sCmd.next());
  bg.b = atoi(sCmd.next());
  EEPROM.write(9,bg.r);
  EEPROM.write(10,bg.g);
  EEPROM.write(11,bg.b);
  Serial.println("BG COLOUR SET");
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = bg;
  }  
}

void set_light(){
  light_low = atoi(sCmd.next());
  light_high = atoi(sCmd.next());
  EEPROM.write(12,light_low);
  EEPROM.write(13,light_high);
  Serial.println("LIGHT SET"); 
}

void set_time(){
  String set_date, set_time;
  set_date = (String)sCmd.next() + ' ' + (String)sCmd.next() + ' ' + (String)sCmd.next();
  set_time = (String)sCmd.next();
  rtc.adjust(DateTime(set_date.c_str(),set_time.c_str()));
}
  
