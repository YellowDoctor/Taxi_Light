// =====================================================================
//  LedManager.cpp — реализация управления лентой WS2812B
// =====================================================================
#include "LedManager.h"

LedManager Led;   // глобальный экземпляр

void LedManager::begin() {
  FastLED.addLeds<LED_TYPE, LED_PIN, LED_COLOR_ORDER>(leds, LED_COUNT)
         .setCorrection(TypicalLEDStrip);
  // Ограничение по току, чтобы не перегружать buck-boost/аккумулятор
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_MAX_MILLIAMPS);
  FastLED.setBrightness(150);
  FastLED.clear(true);
  _on = true;
  Serial.printf("[LED] FastLED инициализирован: %d светодиодов на GPIO%d\n",
                LED_COUNT, LED_PIN);
}

void LedManager::setAll(uint8_t r, uint8_t g, uint8_t b) {
  fill_solid(leds, LED_COUNT, CRGB(r, g, b));
}

void LedManager::setBrightness(uint8_t v) {
  FastLED.setBrightness(v);
}

void LedManager::turnOn() {
  _on = true;
}

void LedManager::turnOff() {
  _on = false;
  fill_solid(leds, LED_COUNT, CRGB::Black);
  FastLED.show();
}

void LedManager::show() {
  FastLED.show();
}
