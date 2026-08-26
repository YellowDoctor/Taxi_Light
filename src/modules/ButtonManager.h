#pragma once
// =====================================================================
//  ButtonManager — сенсорная кнопка (touchRead) с детектированием жестов
// =====================================================================
#include <Arduino.h>
#include <functional>
#include "../Config.h"

class ButtonManager {
public:
  // Колбэки жестов (назначаются из main.cpp)
  std::function<void()> onSingle;       // одно касание
  std::function<void()> onDouble;       // двойное касание
  std::function<void()> onHold;         // удержание 400–2000мс
  std::function<void()> onLongHold;     // долгое удержание > 5000мс

  void begin();
  void tick();                          // неблокирующая обработка

private:
  bool     _pressed      = false;       // текущее состояние касания
  uint32_t _pressStart   = 0;           // момент начала касания
  uint32_t _releaseTime  = 0;           // момент последнего отпускания
  bool     _waitDouble   = false;       // ожидание второго касания
  bool     _holdFired    = false;       // сработало ли удержание
  bool     _longFired    = false;       // сработало ли долгое удержание
  uint32_t _lastHoldStep = 0;           // для повторов при удержании

  bool readTouch();                     // true, если палец на сенсоре
};

extern ButtonManager Button;   // глобальный экземпляр
