#include <Wire.h>
#include "RTClib.h"
#include "FastLED.h"
#include <SoftwareSerial.h>
#include <SerialCommand.h>
#include "EEPROM.h"
#include "TimerOne.h"

#define NUM_LEDS 60
#define DATA_PIN 8
#define UPDATES_PER_SECOND 100
#define SWITCHPIN 2

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

CRGB leds[NUM_LEDS],minutes,hours,seconds,l,bg,lines;
RTC_DS1307 rtc;
SerialCommand sCmd;
boolean missed=0, ledState = 1, lastsec=1, multieffects = 0;
byte lastsecond, rain;
int light_low, light_high;
DateTime now;

void setup() {
  Wire.begin();
  rtc.begin();
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  currentPalette = PartyColors_p;
  // currentPalette = RainbowColors_p;
  currentBlending = BLEND;
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
  // Else read the parameters from the EEPROM
  else {
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
  }
  
  // ********** Setup the serial commands
  
  sCmd.addCommand("MULTI", set_multi);
  sCmd.addCommand("STAT", clockstatus);
  sCmd.addCommand("SETRAIN", set_rainbow);
  sCmd.addCommand("HOUR", set_hour);
  sCmd.addCommand("MIN", set_minute);
  sCmd.addCommand("SEC", set_second);
  sCmd.addCommand("BG", set_bg);
  sCmd.addCommand("LIGHT", set_light);
  sCmd.addCommand("RAINBOW", effects);
  sCmd.addCommand("TIME", set_time);
  // sCmd.addCommand("SETPARA", set_para);
  sCmd.addCommand("MISSED", missedCall);
  sCmd.addCommand("MISSEDOFF", missedOff);
  sCmd.addCommand("RAINBOW", effects);
  sCmd.addCommand("MISSED", missedCall);
  sCmd.addCommand("MISSEDOFF", missedOff);
  sCmd.addDefaultHandler(effects);
  
  // ********** Set all LEDs to background colour
  fill_solid(leds, NUM_LEDS, bg);
  clockstatus();
  Timer1.initialize();
  Timer1.attachInterrupt(state, 500000);
}

void loop() {
  sCmd.readSerial();
  // fill_solid(leds, NUM_LEDS, bg);
  if(multieffects){                 // Check if the button for multi-effects has been pressed
    uint8_t secondHand;
    secondHand = now.second();
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1;
    currentPalette = RainbowStripeColors_p;
    currentBlending = BLEND;
    FillLEDsFromPaletteColors( startIndex);
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
  }
  else 
  if(ledState){                // Main clock code
    // FastLED.delay(1000 / UPDATES_PER_SECOND);
    int x = light_high;      
    now = rtc.now();               //  DateTime(2014,5,2,22,20,0); //
    if(( now.minute() % rain == 0 && now.second() == 0)){
       effects();
    }
    LEDS.setBrightness(constrain(light_low,0,100));
    fill_solid(leds, NUM_LEDS, bg);
    if(now.hour() < 6) LEDS.setBrightness(constrain(light_low,0,100)); // Set brightness to light_low during night - cools down LEDs and power supplies.
    else LEDS.setBrightness(constrain(light_high,10,255));
    for(byte i=0; i<=now.minute();i++){
      leds[i] = minutes;
    }
    leds[now.hour()%12*5]=hours;
    leds[now.hour()%12*5+1]=hours;
    if(now.hour()%12*5-1 > 0)
      leds[now.hour()%12*5-1]=hours;
    else leds[59]=hours; 
    /*for(byte i=(now.hour()%12)*5; i<=((now.hour())%12)*5+(now.minute()/12);i++){
      leds[i] = hours;
    }*/
    for(byte i = 0; i<60; i+=5){
      leds[i]=CRGB(50,50,50);
    }
    if(lastsec){
      l=leds[now.second()];
      leds[now.second()] = seconds;
      lastsecond = now.second();
      lastsec = 0;
      // Serial.println("ON");
    } else {
      leds[lastsecond] = l;
      // if(shuttle()) all_off();
      // Serial.println("OFF");
      lastsec = 1;
    }
    FastLED.show();
    // show_at_max_brightness_for_power();
    ledState = 0;
   }
} 

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
  uint8_t brightness = light_high;
  
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void set_multi(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 200) 
  {
    if(multieffects){
      fill_solid(leds, NUM_LEDS, bg);
    }
    multieffects = !multieffects;
    Serial.println(multieffects);
  }
  last_interrupt_time = interrupt_time;
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
  Serial.print("Light set - High:");
  Serial.println(light_high,DEC);
  Serial.print("Light set - Low:");
  Serial.println(light_low,DEC);
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
  for( int j = 0; j< 300; j++){
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
  FastLED.show();
  FastLED.delay(1000/UPDATES_PER_SECOND);
  }
  /*for (int j=0; j<3; j++){
    for (int i = 0; i < 60; i++) { 
      FillLEDsFromPaletteColors(i);
      FastLED.show();
      // show_at_max_brightness_for_power();
      // delay_at_max_brightness_for_power( 30);
      delay(30);
    }
  }*/
  lastsec = 1;
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
    fill_solid(leds, NUM_LEDS, CRGB::Black);
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
  fill_solid(leds, NUM_LEDS, bg);
}

void set_light(){
  light_low = atoi(sCmd.next());
  light_high = atoi(sCmd.next());
  EEPROM.write(12,light_low);
  EEPROM.write(13,light_high);
  Serial.println("LIGHT SET"); 
}

void set_time(){
  DateTime now = rtc.now();
  Serial.println("TIME SET");
  uint8_t hr = (uint8_t)atoi(sCmd.next());
  uint8_t minu = (uint8_t)atoi(sCmd.next());
  uint8_t sec = (uint8_t)atoi(sCmd.next());
  // Serial.println(hr);
  // Serial.println(minu);
  // Serial.println(sec);
  rtc.adjust(DateTime(now.year(),now.month(),now.day(),hr,minu,sec));
}
