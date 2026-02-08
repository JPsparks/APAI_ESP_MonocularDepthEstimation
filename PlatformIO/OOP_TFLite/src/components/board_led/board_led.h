#ifndef __PW_APAI_B_LED_H
#define __PW_APAI_B_LED_H

#include <FastLED.h>

// LED WS2812 config 
// this il also the first lib where this developement started, 
// again taken inspiration by Freenove tutorials
#define LED_PIN      48         // This should be ok for ESP32s3 board
#define NUM_LEDS     1          // Amount of involved leds
#define LED_TYPE     WS2812B 
#define COLOR_ORDER  GRB

// Some basic color definition
#define COLOR_OFF      CRGB::Black
#define COLOR_ERROR    CRGB::Red
#define COLOR_READY    CRGB::Green
#define COLOR_CAPTURE  CRGB::Blue

// Yellow for warnings
#define COLOR_WARN     CRGB::Yellow
//etc
#define COLOR_INFO     CRGB::Aqua
#define COLOR_WAIT     CRGB::Purple
#define COLOR_TEMP     CRGB::Orange
#define COLOR_SUCCESS  CRGB::Lime
#define COLOR_DEBUG    CRGB::White

#define COLOR_PINK     CRGB::Pink
#define COLOR_CYAN     CRGB::Cyan
#define COLOR_MAGENTA  CRGB::Magenta

#include <Arduino.h>

class BoardLed {
  public:
    BoardLed();
    void ledSetColor(CRGB color);
    void ledInit();
    void ledBlink(CRGB color, int times = 3, int delayMs = 200);

  private:
    CRGB leds[NUM_LEDS];

};



#endif 