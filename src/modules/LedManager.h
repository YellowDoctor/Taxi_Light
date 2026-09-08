#pragma once
// =====================================================================
//  LedManager — управление лентой WS2812B через FastLED
// =====================================================================
#include <Arduino.h>
#include <FastLED.h>
#include "../Config.h"

class LedManager {
public:
  CRGB leds[LED_COUNT];      // буфер кадра

  void begin();
  void setAll(uint8_t r, uint8_t g, uint8_t b);   // залить весь массив цветом
  void setBrightness(uint8_t v);                  // общая яркость 0–255
  void turnOn();
  void turnOff();
  void show();               // вывести буфер на ленту
  bool isOn() const { return _on; }

  // Индикация процесса обновления
  void showOtaProgress();    // зелёный на 50% яркости
  void showOtaSuccess();     // 3 мигания зелёным
  void showOtaError();       // 3 мигания красным

private:
  bool _on = true;
};

extern LedManager Led;   // глобальный экземпляр
