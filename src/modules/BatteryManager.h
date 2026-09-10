#pragma once
// =====================================================================
//  BatteryManager — измерение напряжения и заряда аккумулятора
//  - Калибровка ADC через esp_adc_cal (заводские данные из eFuse)
//  - Нелинейная LUT-таблица для кривой разряда Li-Ion
//  - EMA-фильтр для стабилизации показаний
// =====================================================================
#include <Arduino.h>
#include <esp_adc_cal.h>
#include "../Config.h"

class BatteryManager {
public:
  void    begin();
  void    tick();                 // неблокирующее обновление раз в BATTERY_UPDATE_MS
  float   getVoltage();           // напряжение на аккумуляторе, В
  uint8_t getPercent();           // заряд, % (по LUT)
  bool    isCharging();           // определение зарядки по тренду
  bool    isCritical() const { return _critical; } // критический разряд (< 3.0В)

private:
  float    _voltage     = 0.0f;
  float    _emaVoltage  = 0.0f;   // отфильтрованное (EMA)
  uint8_t  _percent     = 0;
  bool     _charging    = false;
  uint32_t _lastUpdate  = 0;
  uint8_t  _risingCount = 0;      // счётчик подряд растущих замеров
  uint8_t  _criticalCount = 0;    // счётчик подтверждений низкого напряжения
  bool     _critical    = false;
  esp_adc_cal_characteristics_t _adcChars;
  bool     _calibrated  = false;

  float   measureRaw();            // один замер с калибрацией, В
  uint8_t voltageToPct(float v);   // перевод через LUT
};

extern BatteryManager Battery;
