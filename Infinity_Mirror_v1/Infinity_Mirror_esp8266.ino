#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include "FastLED.h"
#include "EEPROM.h"

#define NUM_LEDS 60
#define DATA_PIN 4
#define UPDATES_PER_SECOND 60
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0
#define MQTT_MAX_PACKET_SIZE 256


CRGBPalette16 currentPalette;
TBlendType    currentBlending;

CRGB leds[NUM_LEDS],minutes,hours,seconds,l,bg,lines;
int light_low, light_high;
boolean missed=0, ledState = 1, lastsec=1, multieffects = 0;
byte lastsecond, rain;

//WiFi variables............................................................
const char ssid[] = "***********";        //your network SSID (name)
const char password[] = "**********";       // your network password

WiFiClient espClient;
IPAddress server(192, 168, 1, 236);       // your MQTT server
PubSubClient client(espClient, server);

long lastMsg = 0;
char msg[50];
int value = 0;

// NTP Servers:
//static const char ntpServerName[] = "us.pool.ntp.org";
static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
float timeZone = -5;

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

void setup() {

  delay(100);
  // Serial.begin(115200);
  wdt_disable();
  ArduinoOTA.setHostname("infinityclock");
  ArduinoOTA.setPassword((const char *)"avin");
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  // currentPalette = nrwc_gp;
  /*for( int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }*/
  currentBlending = LINEARBLEND;
  fill_solid(leds, NUM_LEDS, bg);
  
  Udp.begin(localPort);
  setup_wifi();
  client.set_callback(callback);
  setSyncProvider(getNtpTime);
  setSyncInterval(3600);
  if (hour() == 0 && second() < 3) {
    setSyncProvider(getNtpTime);
  }
  EEPROM.begin(512);
  if (EEPROM.read(101) != 1){               // Check if colours have been set or not
    
    seconds.r = 0;
    seconds.g = 0;
    seconds.b = 0;  
    minutes.r = 127;
    minutes.g = 25;
    minutes.b = 10;
    hours.r = 0;
    hours.g = 255;
    hours.b = 255;
    bg.r = 0;
    bg.g = 0;
    bg.b = 5;
    light_low = 0;
    light_high = 120; 
    rain = 30; 
    
    EEPROM.write(0,0);                   // Seconds Colour 
    EEPROM.write(1,0);
    EEPROM.write(2,0);
    EEPROM.write(3,127);                   // Minutes Colour 
    EEPROM.write(4,25);
    EEPROM.write(5,10);
    EEPROM.write(6,0);                     // Hours Colour 
    EEPROM.write(7,255);
    EEPROM.write(8,255);
    EEPROM.write(9,0);                     // BG Colour 
    EEPROM.write(10,0);
    EEPROM.write(11,5);
    EEPROM.write(12, 0);                   // Light sensitivity - low
    EEPROM.write(13, 100);                  // Light sensitivity - high 
    EEPROM.write(14, 30);                  // Minutes for each rainbow   
    EEPROM.write(101,1);
    EEPROM.commit();
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
  LEDS.setBrightness(constrain(light_high,10,255));
  wdt_enable(WDTO_8S);
}

void loop() {
  ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED)
    setup_wifi();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  EVERY_N_MILLISECONDS(500) showTime(int(hour()),int(minute()),int(second()));
  // EVERY_N_MILLISECONDS(5000) clockstatus();
  /*EVERY_N_MILLISECONDS(5000){
    Serial.println(hour());
    Serial.println(minute());
    Serial.println(second());
    
  }*/
  // FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void showTime(int hr, int mn, int sec) {

  if(sec==0) fill_solid(leds, NUM_LEDS, bg);
  if(( mn % rain == 0 && sec == 0)){
       effects();
    }
  if(hr < 6) 
    LEDS.setBrightness(constrain(light_low,0,100)); // Set brightness to light_low during night - cools down LEDs and power supplies.
  else 
    LEDS.setBrightness(constrain(light_high,10,255));
  // fill_palette( leds, mn, 0, 6, currentPalette);
  for(byte i=0; i<=mn;i++){
      leds[i] = minutes; //ColorFromPalette( currentPalette, i*6); 
    }
  //Fire2012(mn);
  leds[hr%12*5]=hours;
  leds[hr%12*5+1]=hours;
  if(hr%12*5-1 > 0)
    leds[hr%12*5-1]=hours;
  else leds[59]=hours; for(byte i = 0; i<60; i+=5){
    leds[i]=CRGB(50,50,50);
  }
  if(lastsec){
      l=leds[sec];
      leds[sec] = seconds;
      lastsecond = sec;
      lastsec = 0;
      // Serial.println("ON");
    } else {
      leds[lastsecond] = l;
      // Serial.println("OFF");
      lastsec = 1;
    }
  FastLED.show();  
}

void callback(const MQTT::Publish& pub) {
 Serial.print(pub.topic());
 Serial.print(" => ");
 Serial.println(pub.payload_string());
 
 String payload = pub.payload_string();

  if(String(pub.topic()) == "infinity/hour"){
    int c1 = payload.indexOf(',');
    int c2 = payload.indexOf(',',c1+1); 
    int h = map(payload.toInt(),0,360,0,255);
    int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    set_hour_hsv(h,s,v);
 }
 if(String(pub.topic()) == "infinity/minute"){
    int c1 = payload.indexOf(',');
    int c2 = payload.indexOf(',',c1+1); 
    int h = map(payload.toInt(),0,360,0,255);
    int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    set_hour_hsv(h,s,v);
 }
 if(String(pub.topic()) == "infinity/second"){
    int c1 = payload.indexOf(',');
    int c2 = payload.indexOf(',',c1+1); 
    int h = map(payload.toInt(),0,360,0,255);
    int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    set_hour_hsv(h,s,v);
 }
 if(String(pub.topic()) == "infinity/bg"){
    int c1 = payload.indexOf(',');
    int c2 = payload.indexOf(',',c1+1); 
    int h = map(payload.toInt(),0,360,0,255);
    int s = map(payload.substring(c1+1,c2).toInt(),0,100,0,255);
    int v = map(payload.substring(c2+1).toInt(),0,100,0,255);
    set_hour_hsv(h,s,v);
 }
 if(String(pub.topic()) == "infinity/effects"){
    effects();
 }
}

void effects(){
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
  fill_solid(leds, NUM_LEDS, bg);
  client.publish("infinity/status","RAINBOW");
  lastsec = 1;
}


void set_hour_hsv(int h, int s, int v){
  CHSV temp; 
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,hours);
  EEPROM.write(6,hours.r);
  EEPROM.write(7,hours.g);
  EEPROM.write(8,hours.b);
  EEPROM.commit();
  client.publish("infinity/status","HOUR COLOUR SET");  
}

void set_minute_hsv(int h, int s, int v){
  CHSV temp; 
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,minutes);
  EEPROM.write(3,minutes.r);
  EEPROM.write(4,minutes.g);
  EEPROM.write(5,minutes.b);
  EEPROM.commit();
  client.publish("infinity/status","MINUTE COLOUR SET");  
}

void set_second_hsv(int h, int s, int v){
  CHSV temp; 
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,seconds);
  EEPROM.write(0,seconds.r);
  EEPROM.write(1,seconds.g);
  EEPROM.write(2,seconds.b);
  EEPROM.commit();
  client.publish("infinity/status","SECOND COLOUR SET");  
}

void set_bgsecond_hsv(int h, int s, int v){
  CHSV temp; 
  temp.h = h;
  temp.s = s;
  temp.v = v;
  hsv2rgb_rainbow(temp,bg);
  EEPROM.write(9,bg.r);
  EEPROM.write(10,bg.g);
  EEPROM.write(11,bg.b);
  EEPROM.commit();
  client.publish("infinity/status","BG COLOUR SET");
  fill_solid(leds, NUM_LEDS, bg); 
}

void clockstatus(){
  /*client.publish("infinity/status","Status: ");
  client.publish("infinity/status","BG: ");
  client.publish("infinity/status",bg.r);
  client.publish("infinity/status",bg.g);
  client.publish("infinity/status",bg.b);
  client.publish("infinity/status","SEC: ");
  client.publish("infinity/status",seconds.r);
  client.publish("infinity/status",seconds.g);
  client.publish("infinity/status",seconds.b);
  client.publish("infinity/status","MINUTE: ");
  client.publish("infinity/status",minutes.r);
  client.publish("infinity/status",minutes.g);
  client.publish("infinity/status",minutes.b);
  client.publish("infinity/status","HOUR: ");
  client.publish("infinity/status",hours.r);
  client.publish("infinity/status",hours.g);
  client.publish("infinity/status",hours.b);*/
}

/*
void set_light(){
  light_low = atoi(sCmd.next());
  light_high = atoi(sCmd.next());
  EEPROM.write(12,light_low);
  EEPROM.write(13,light_high);
  client.publish("infinity/status","LIGHT SET"); 
}*/

/* --------------- WiFi Code ------------------ */


void setup_wifi() {
  //delay(10);
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (client.connect("infinity")) {
      // Once connected, publish an announcement...
      client.publish("infinity/status", "Infinity Mirror Alive - topics infinity/hour,infinity/minute,infinity/second,infinity/bg subscribed");
      // ... and resubscribe
      client.subscribe("infinity/hour");
      client.subscribe("infinity/minute");
      client.subscribe("infinity/second");
      client.subscribe("infinity/bg");
      client.subscribe("infinity/effects");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1600) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      // client.publish("infinity/status",secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      
    }
  }
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
