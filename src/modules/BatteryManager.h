#pragma once
// =====================================================================
//  BatteryManager — измерение напряжения и заряда аккумулятора
//  - Калибровка ADC через esp_adc_cal (заводские данные из eFuse)
//  - Нелинейная LUT-таблица для кривой разряда Li-Ion
//  - EMA-фильтр для стабилизации показаний
//  - Программная компенсация просадки под нагрузкой (IR Drop)
//  - Детекция зарядки без барьера 4.05В
// =====================================================================
#include <Arduino.h>
#include <esp_adc_cal.h>
#include "../Config.h"

class BatteryManager {
public:
  void    begin();
  void    tick();                 // неблокирующее обновление раз в BATTERY_UPDATE_MS
  float   getVoltage();           // измеренное напряжение на аккумуляторе, В
  uint8_t getPercent();           // заряд, % (по LUT с компенсацией нагрузки)
  bool    isCharging();           // идёт зарядка
  bool    isCharged();            // заряд завершён (напряжение стабильно > 4.16 В)
  bool    isCritical() const { return _critical; } // критический разряд (< 3.0В)

private:
  float    _voltage        = 0.0f;
  float    _emaVoltage     = 0.0f;   // отфильтрованное фактическое (EMA)
  float    _emaCompVoltage = 0.0f;   // отфильтрованное скомпенсированное под нагрузку
  uint8_t  _percent        = 0;
  bool     _charging       = false;
  bool     _charged        = false;  // зарядка завершена
  uint32_t _lastUpdate     = 0;
  uint8_t  _risingCount    = 0;      // счётчик подтверждений зарядки
  uint8_t  _dropCount      = 0;      // счётчик подтверждений отключения
  uint8_t  _stableCount    = 0;      // счётчик стабильных замеров > 4.16 В
  uint8_t  _criticalCount  = 0;      // счётчик подтверждений низкого напряжения
  bool     _critical       = false;
  bool     _prevLampOn     = false;  // отслеживание состояния подсветки
  uint32_t _lastLampToggle = 0;      // время последнего переключения света
  esp_adc_cal_characteristics_t _adcChars;
  bool     _calibrated     = false;

  float   measureRaw();            // один замер с калибрацией, В
  uint8_t voltageToPct(float v);   // перевод через LUT
};

extern BatteryManager Battery;
