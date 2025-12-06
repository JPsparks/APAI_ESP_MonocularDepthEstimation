#ifndef __PW_APAI_B_LED_H
#define __PW_APAI_B_LED_H

#include <FastLED.h>

// LED WS2812 config
#define LED_PIN      48  // This should be ok fot ESP32s3 board
#define NUM_LEDS     1   // Amount of involved leds
#define LED_TYPE     WS2812B 
#define COLOR_ORDER  GRB

// Some basic color definition
#define COLOR_OFF      CRGB::Black
#define COLOR_ERROR    CRGB::Red
#define COLOR_READY    CRGB::Green
#define COLOR_CAPTURE  CRGB::Blue

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