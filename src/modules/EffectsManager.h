#pragma once
// =====================================================================
//  EffectsManager — неблокирующие световые эффекты
// =====================================================================
#include <Arduino.h>
#include <FastLED.h>
#include "../Config.h"
#include "LedManager.h"

class EffectsManager {
public:
  void begin();
  void tick();                                 // вызывать из loop() каждый цикл
  void setEffect(uint8_t id);                  // выбрать эффект
  void setSpeed(uint8_t v);                    // скорость 0–255
  void setColor(uint8_t r, uint8_t g, uint8_t b);
  uint8_t getEffect() const { return _effect; }

private:
  uint8_t  _effect = EFFECT_STATIC;
  uint8_t  _speed  = 128;
  CRGB     _color  = CRGB(255, 176, 0);
  uint32_t _lastTick = 0;
  uint8_t  _hue = 0;               // общий счётчик оттенка
  byte     _heat[LED_COUNT];       // тепловая карта для эффекта «огонь»

  // Возвращает интервал обновления (мс) в зависимости от скорости
  uint16_t frameInterval();

  // Реализации эффектов
  void effStatic();
  void effBreathe();
  void effRainbow();
  void effFire();
  void effCandle();
  void effColorflow();
};

extern EffectsManager Effects;   // глобальный экземпляр
