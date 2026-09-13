#pragma once
// =====================================================================
//  BatteryManager тАФ ╨╕╨╖╨╝╨╡╤А╨╡╨╜╨╕╨╡ ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╤П ╨╕ ╨╖╨░╤А╤П╨┤╨░ ╨░╨║╨║╤Г╨╝╤Г╨╗╤П╤В╨╛╤А╨░
//  - ╨Ъ╨░╨╗╨╕╨▒╤А╨╛╨▓╨║╨░ ADC ╤З╨╡╤А╨╡╨╖ esp_adc_cal (╨╖╨░╨▓╨╛╨┤╤Б╨║╨╕╨╡ ╨┤╨░╨╜╨╜╤Л╨╡ ╨╕╨╖ eFuse)
//  - ╨Э╨╡╨╗╨╕╨╜╨╡╨╣╨╜╨░╤П LUT-╤В╨░╨▒╨╗╨╕╤Ж╨░ ╨┤╨╗╤П ╨║╤А╨╕╨▓╨╛╨╣ ╤А╨░╨╖╤А╤П╨┤╨░ Li-Ion
//  - EMA-╤Д╨╕╨╗╤М╤В╤А ╨┤╨╗╤П ╤Б╤В╨░╨▒╨╕╨╗╨╕╨╖╨░╤Ж╨╕╨╕ ╨┐╨╛╨║╨░╨╖╨░╨╜╨╕╨╣
// =====================================================================
#include <Arduino.h>
#include <esp_adc_cal.h>
#include "../Config.h"

class BatteryManager {
public:
  void    begin();
  void    tick();                 // ╨╜╨╡╨▒╨╗╨╛╨║╨╕╤А╤Г╤О╤Й╨╡╨╡ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╤А╨░╨╖ ╨▓ BATTERY_UPDATE_MS
  float   getVoltage();           // ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ ╨╜╨░ ╨░╨║╨║╤Г╨╝╤Г╨╗╤П╤В╨╛╤А╨╡, ╨Т
  uint8_t getPercent();           // ╨╖╨░╤А╤П╨┤, % (╨┐╨╛ LUT)
  bool    isCharging();           // ╨╕╨┤╤С╤В ╨╖╨░╤А╤П╨┤╨║╨░ (╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ ╤А╨░╤Б╤В╤С╤В, ╨┐╨╛╤А╨╛╨│ > 4.05 ╨Т)
  bool    isCharged();            // ╨╖╨░╤А╤П╨┤ ╨╖╨░╨▓╨╡╤А╤И╤С╨╜ (╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ ╤Б╤В╨░╨▒╨╕╨╗╤М╨╜╨╛ > 4.18 ╨Т)
  bool    isCritical() const { return _critical; } // ╨║╤А╨╕╤В╨╕╤З╨╡╤Б╨║╨╕╨╣ ╤А╨░╨╖╤А╤П╨┤ (< 3.0╨Т)

private:
  float    _voltage      = 0.0f;
  float    _emaVoltage   = 0.0f;   // ╨╛╤В╤Д╨╕╨╗╤М╤В╤А╨╛╨▓╨░╨╜╨╜╨╛╨╡ (EMA)
  uint8_t  _percent      = 0;
  bool     _charging     = false;
  bool     _charged      = false;  // ╨╖╨░╤А╤П╨┤╨║╨░ ╨╖╨░╨▓╨╡╤А╤И╨╡╨╜╨░ (full)
  bool     _prevCharging = false;  // ╨┤╨╗╤П ╤Б╨▒╤А╨╛╤Б╨░ EMA ╨┐╤А╨╕ ╤Б╨╝╨╡╨╜╨╡ ╤Б╨╛╤Б╤В╨╛╤П╨╜╨╕╤П
  uint32_t _lastUpdate   = 0;
  uint8_t  _risingCount  = 0;      // ╤Б╤З╤С╤В╤З╨╕╨║ ╨┐╨╛╨┤╤А╤П╨┤ ╤А╨░╤Б╤В╤Г╤Й╨╕╤Е ╨╖╨░╨╝╨╡╤А╨╛╨▓
  uint8_t  _stableCount  = 0;      // ╤Б╤З╤С╤В╤З╨╕╨║ ╤Б╤В╨░╨▒╨╕╨╗╤М╨╜╤Л╤Е ╨╖╨░╨╝╨╡╤А╨╛╨▓ > 4.18 ╨Т
  uint8_t  _criticalCount = 0;     // ╤Б╤З╤С╤В╤З╨╕╨║ ╨┐╨╛╨┤╤В╨▓╨╡╤А╨╢╨┤╨╡╨╜╨╕╨╣ ╨╜╨╕╨╖╨║╨╛╨│╨╛ ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╤П
  bool     _critical     = false;
  esp_adc_cal_characteristics_t _adcChars;
  bool     _calibrated   = false;

  float   measureRaw();            // ╨╛╨┤╨╕╨╜ ╨╖╨░╨╝╨╡╤А ╤Б ╨║╨░╨╗╨╕╨▒╤А╨░╤Ж╨╕╨╡╨╣, ╨Т
  uint8_t voltageToPct(float v);   // ╨┐╨╡╤А╨╡╨▓╨╛╨┤ ╤З╨╡╤А╨╡╨╖ LUT
};

extern BatteryManager Battery;
