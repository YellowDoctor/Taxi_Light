#pragma once
// =====================================================================
//  SunriseManager — плавное нарастание яркости (будильник-рассвет)
//
//  Работает поверх текущего эффекта: во время восхода заменяет
//  управление яркостью и цветом, имитируя рассвет.
//  По завершении восстанавливает нормальный режим.
// =====================================================================
#include <Arduino.h>
#include <FastLED.h>
#include "../Config.h"

class SunriseManager {
public:
  void begin();
  void tick();

  // Запустить рассвет. durationMin — длительность в минутах (1..SUNRISE_MAX_MINUTES)
  // targetBrightness — финальная яркость (1..255)
  void start(uint8_t durationMin, uint8_t targetBrightness);
  void cancel();

  bool     isActive()    const { return _active; }
  uint32_t secondsLeft() const;

private:
  bool     _active           = false;
  uint32_t _startMs          = 0;
  uint32_t _durationMs       = 0;
  uint8_t  _targetBrightness = 200;
  uint32_t _lastTick         = 0;
};

extern SunriseManager Sunrise;   // глобальный экземпляр
