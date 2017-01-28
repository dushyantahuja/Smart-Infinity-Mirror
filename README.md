Smart-Infinity-Mirror
=====================

Smart Infinity Mirror that would have a clock, proximity sensor and a smart display
Ever since my addressable RGB LED strips (WS2812B) came from Aliexpress, I've been fascinated with LED projects. Following up on my success with my ​Charlieplexed LED clock, I wanted to create something with more Jazz..
While browsing google images, I came across an instructable for an ​Arduino Infinity Mirror and the brain cells started working in overdrive. Why shouldn't I make an infinity mirror, that's also a clock !!

Links: 
http://dushyant.ahuja.ws/2014/07/infinity-mirror-clock/

Code revised to work on an ESP8266 instead of Arduino + RTC. The code has been revised to use NTP and hence does not require resetting of the clock at regular intervals (though still requires change of offset for daylight savings)

Also, allows for changing colours using MQTT and OpenHAB. This takes the default values from a color slider in OpenHAB (HSV values) and no rules are required to convert HSV to RGB.

