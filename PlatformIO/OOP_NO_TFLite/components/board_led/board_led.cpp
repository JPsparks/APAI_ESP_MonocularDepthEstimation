#include "board_led.h"

BoardLed::BoardLed(){

}

void BoardLed::ledSetColor(CRGB color) {
  fill_solid(this->leds, NUM_LEDS, color);
  FastLED.show();
}

void BoardLed::ledInit() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(50); // Luminosità 0-255
  ledSetColor(COLOR_OFF);
}

// Funzione opzionale per lampeggio
void BoardLed::ledBlink(CRGB color, int times, int delayMs) {
  for(int i = 0; i < times; i++) {
    ledSetColor(color);
    delay(delayMs);
    ledSetColor(COLOR_OFF);
    delay(delayMs);
  }
}
