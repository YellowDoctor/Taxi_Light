#pragma once
// =====================================================================
//  BatteryManager — измерение напряжения и заряда аккумулятора
// =====================================================================
#include <Arduino.h>
#include "../Config.h"

class BatteryManager {
public:
  void  begin();
  void  tick();                 // неблокирующее обновление раз в BATTERY_UPDATE_MS
  float getVoltage();           // напряжение на аккумуляторе, В
  uint8_t getPercent();         // заряд, %
  bool  isCharging();           // грубое определение зарядки (рост напряжения)

private:
  float    _voltage   = 0.0f;
  float    _lastVoltage = 0.0f;
  uint8_t  _percent   = 0;
  bool     _charging  = false;
  uint32_t _lastUpdate = 0;

  void measure();               // выполнить замер прямо сейчас
};

extern BatteryManager Battery;   // глобальный экземпляр
