
#define LCD_CS A3    
#define LCD_CD A2    
#define LCD_WR A1   
#define LCD_RD A0    
#define LCD_RESET A4

#include "TFTLCD.h"

TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  tft.reset();
  tft.initDisplay();
  tft.fillScreen(0x0000);
  tft.setRotation(3);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  tft.setCursor(0, 20);
  tft.setTextColor(0x001F);
  tft.setTextSize(3);
  tft.println("Good Morning");
}
