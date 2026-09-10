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
  void pause() { _paused = true; }
  void resume() { _paused = false; _dirty = true; }
  bool isPaused() const { return _paused; }
  void markDirty() { _dirty = true; }

private:
  bool     _paused = false;
  bool     _dirty  = true;
  uint32_t _lastStaticShow = 0;
  uint8_t  _effect = EFFECT_STATIC;
  uint8_t  _speed  = 128;
  CRGB     _color  = CRGB(255, 176, 0);
  uint32_t _lastTick = 0;
  uint8_t  _hue = 0;               // общий счётчик оттенка
  byte     _heat[LED_COUNT];       // тепловая карта для эффекта «огонь»
  
  // Состояние полицейской мигалки
  uint8_t _policeStep = 0;
  uint32_t _policeLastChange = 0;

  // Метеор
  uint8_t  _meteorPos = 0;
  // Лазер
  int8_t   _laserPos  = 0;
  int8_t   _laserDir  = 1;
  // Стробоскоп
  bool     _strobeOn  = false;

  // Возвращает интервал обновления (мс) в зависимости от скорости
  uint16_t frameInterval();

  // Реализации эффектов
  void effStatic();
  void effBreathe();
  void effRainbow();
  void effFire();
  void effCandle();
  void effColorflow();
  void effAurora();
  void effOcean();
  void effPolice();
  void effPoliceFlash();
  void effMeteor();     // 10 — метеор с хвостом угасания
  void effStrobe();     // 11 — стробоскоп
  void effRunning();    // 12 — бегущие огни (гирлянда)
  void effNightsky();   // 13 — ночное небо (мерцающие звёзды)
  void effLaser();      // 14 — лазер (сканирующая точка)
  void effFlash();      // 15 — случайные вспышки
};

extern EffectsManager Effects;   // глобальный экземпляр
