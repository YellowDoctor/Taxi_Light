// =====================================================================
//  BatteryManager.cpp тАФ ╤В╨╛╤З╨╜╨╛╨╡ ╨╕╨╖╨╝╨╡╤А╨╡╨╜╨╕╨╡ ╨░╨║╨║╤Г╨╝╤Г╨╗╤П╤В╨╛╤А╨░
//  - esp_adc_cal: ╨║╨░╨╗╨╕╨▒╤А╨╛╨▓╨║╨░ ADC ╨┐╨╛ ╨╖╨░╨▓╨╛╨┤╤Б╨║╨╕╨╝ ╨┤╨░╨╜╨╜╤Л╨╝ ╨╕╨╖ eFuse ESP32
//  - LUT 11 ╤В╨╛╤З╨╡╨║: ╤А╨╡╨░╨╗╤М╨╜╨░╤П ╨║╤А╨╕╨▓╨░╤П ╤А╨░╨╖╤А╤П╨┤╨░ Li-Ion 21700
//  - EMA-╤Д╨╕╨╗╤М╤В╤А (╬▒ = 0.15): ╤Г╨▒╨╕╤А╨░╨╡╤В ╤Б╨║╨░╤З╨║╨╕ ╨┐╤А╨╕ ╨╕╨╖╨╝╨╡╨╜╨╡╨╜╨╕╨╕ ╨╜╨░╨│╤А╤Г╨╖╨║╨╕
//  - ╨Ч╨░╤А╤П╨┤╨║╨░: 3+ ╨╖╨░╨╝╨╡╤А╨░ ╨┐╨╛╨┤╤А╤П╨┤ ╤Б ╤А╨╛╤Б╤В╨╛╨╝ ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╤П
// =====================================================================
#include "BatteryManager.h"

BatteryManager Battery;

// ---------- LUT ╨║╤А╨╕╨▓╨╛╨╣ ╤А╨░╨╖╤А╤П╨┤╨░ Li-Ion ----------
// ╨а╨╡╨░╨╗╤М╨╜╨░╤П ╨║╤А╨╕╨▓╨░╤П ╤А╨░╨╖╤А╤П╨┤╨░ Li-Ion (NCR21700 / Samsung 40T ╨╕ ╨┐╨╛╨┤╨╛╨▒╨╜╤Л╨╡).
// ╨Э╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ тЖТ ╨┐╤А╨╛╤Ж╨╡╨╜╤В ╨╖╨░╤А╤П╨┤╨░. ╨Ь╨╡╨╢╨┤╤Г ╤В╨╛╤З╨║╨░╨╝╨╕ тАФ ╨╗╨╕╨╜╨╡╨╣╨╜╨░╤П ╨╕╨╜╤В╨╡╤А╨┐╨╛╨╗╤П╤Ж╨╕╤П.
static const float LUT_V[]   = { 3.20, 3.40, 3.50, 3.60, 3.70, 3.75, 3.80, 3.85, 3.95, 4.10, 4.20 };
static const float LUT_PCT[] = { 0,    5,    10,   20,   35,   45,   55,   65,   80,   95,   100  };
static const int   LUT_SIZE  = sizeof(LUT_V) / sizeof(LUT_V[0]);

// ---------- EMA-╤Д╨╕╨╗╤М╤В╤А ----------
static const float EMA_ALPHA = 0.15f;   // ╤З╨╡╨╝ ╨╝╨╡╨╜╤М╤И╨╡ тАФ ╤В╨╡╨╝ ╨┐╨╗╨░╨▓╨╜╨╡╨╡ (0.1..0.3)

void BatteryManager::begin() {
  analogReadResolution(12);  // 0..4095
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  // esp_adc_cal: ╨║╨░╨╗╨╕╨▒╤А╨╛╨▓╨║╨░ ╨┐╨╛ ╨╖╨░╨▓╨╛╨┤╤Б╨║╨╕╨╝ ╨┤╨░╨╜╨╜╤Л╨╝
  // ADC1, ╨║╨░╨╜╨░╨╗ 6 = GPIO34, ╨░╤В╤В╨╡╨╜╤О╨░╤Ж╨╕╤П 12dB (0..3.3V)
  _calibrated = (esp_adc_cal_characterize(
    ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &_adcChars
  ) != ESP_ADC_CAL_VAL_NOT_SUPPORTED);

  if (_calibrated) {
    Serial.println(F("[BAT] ADC ╨╛╤В╨║╨░╨╗╨╕╨▒╤А╨╛╨▓╨░╨╜ ╨┐╨╛ eFuse"));
  } else {
    Serial.println(F("[BAT] ╨Ъ╨░╨╗╨╕╨▒╤А╨╛╨▓╨║╨░ eFuse ╨╜╨╡╨┤╨╛╤Б╤В╤Г╨┐╨╜╨░, ╨╕╤Б╨┐╨╛╨╗╤М╨╖╤Г╨╡╨╝ ╤Д╨╛╤А╨╝╤Г╨╗╤Г"));
  }


  // ╨Я╨╡╤А╨▓╤Л╨╣ ╨╖╨░╨╝╨╡╤А тАФ ╨╕╨╜╨╕╤Ж╨╕╨░╨╗╨╕╨╖╨╕╤А╤Г╨╡╨╝ EMA ╤В╨╡╨║╤Г╤Й╨╕╨╝ ╨╖╨╜╨░╤З╨╡╨╜╨╕╨╡╨╝
  float v = measureRaw();
  _emaVoltage = v;
  _voltage = v;
  _percent = voltageToPct(v);
  _lastUpdate = millis();

  Serial.printf("[BAT] ╨б╤В╨░╤А╤В╨╛╨▓╨╛╨╡ ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡: %.3f ╨Т (%d%%)\n", _voltage, _percent);
}

// ---------- ╨Ч╨░╨╝╨╡╤А ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╤П ╤Б ╨║╨░╨╗╨╕╨▒╤А╨╛╨▓╨║╨╛╨╣ ----------
float BatteryManager::measureRaw() {
  uint32_t sum = 0;

  if (_calibrated) {
    // esp_adc_cal ╨┤╨░╤С╤В ╤А╨╡╨╖╤Г╨╗╤М╤В╨░╤В ╨▓ ╨╝╨╕╨╗╨╗╨╕╨▓╨╛╨╗╤М╤В╨░╤Е
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
      sum += esp_adc_cal_raw_to_voltage(analogRead(BATTERY_PIN), &_adcChars);
      delayMicroseconds(200);
    }
    float pinMv = (float)sum / BATTERY_SAMPLES;
    return (pinMv / 1000.0f) * BATTERY_DIVIDER_RATIO;
  } else {
    // ╨д╨╛╨╗╨▒╤Н╨║: ╨╛╨▒╤Л╤З╨╜╤Л╨╣ ╤А╨░╤Б╤З╤С╤В
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
      sum += analogRead(BATTERY_PIN);
      delayMicroseconds(200);
    }
    float pinVoltage = ((float)sum / BATTERY_SAMPLES / 4095.0f) * 3.3f;
    return pinVoltage * BATTERY_DIVIDER_RATIO;
  }
}

// ---------- LUT-╨┐╨╡╤А╨╡╨▓╨╛╨┤ ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╤П ╨▓ ╨┐╤А╨╛╤Ж╨╡╨╜╤В╤Л ----------
uint8_t BatteryManager::voltageToPct(float v) {
  if (v <= LUT_V[0]) return 0;
  if (v >= LUT_V[LUT_SIZE - 1]) return 100;

  // ╨Ы╨╕╨╜╨╡╨╣╨╜╨░╤П ╨╕╨╜╤В╨╡╤А╨┐╨╛╨╗╤П╤Ж╨╕╤П ╨╝╨╡╨╢╨┤╤Г ╨▒╨╗╨╕╨╢╨░╨╣╤И╨╕╨╝╨╕ ╤В╨╛╤З╨║╨░╨╝╨╕
  for (int i = 1; i < LUT_SIZE; i++) {
    if (v <= LUT_V[i]) {
      float t = (v - LUT_V[i - 1]) / (LUT_V[i] - LUT_V[i - 1]);
      float pct = LUT_PCT[i - 1] + t * (LUT_PCT[i] - LUT_PCT[i - 1]);
      return (uint8_t)(pct + 0.5f);
    }
  }
  return 100;
}

// ---------- ╨Я╨╡╤А╨╕╨╛╨┤╨╕╤З╨╡╤Б╨║╨╛╨╡ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ----------
void BatteryManager::tick() {
  uint32_t now = millis();
  if (now - _lastUpdate < BATTERY_UPDATE_MS) return;
  _lastUpdate = now;

  float raw = measureRaw();

  // ╨б╨╛╤Е╤А╨░╨╜╤П╨╡╨╝ ╤Б╨╛╤Б╤В╨╛╤П╨╜╨╕╨╡ ╨┤╨╛ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╤П (╨┤╨╗╤П ╨┤╨╡╤В╨╡╨║╤В╨╕╤А╨╛╨▓╨░╨╜╨╕╤П ╤Б╨╝╨╡╨╜╤Л)
  _prevCharging = _charging;

  // ---- ╨Ф╨╡╤В╨╡╨║╤В╨╕╤А╨╛╨▓╨░╨╜╨╕╨╡ ╨╖╨░╤А╤П╨┤╨║╨╕ ----
  // ╨Я╤А╨╕╨╖╨╜╨░╨║╨╕ ╨╖╨░╤А╤П╨┤╨║╨╕ WITHOUT CHRG-╨┐╨╕╨╜╨░ (╨Т╨░╤А╨╕╨░╨╜╤В A):
  //   1. ╨Э╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ > 4.05 ╨Т (Li-Ion ╨┐╤А╨╕ ╤А╨░╨╖╤А╤П╨┤╨╡ ╤А╨╡╨┤╨║╨╛ ╨┐╤А╨╡╨▓╤Л╤И╨░╨╡╤В 4.05 ╨Т ╨┐╨╛╨┤ ╨╜╨░╨│╤А╤Г╨╖╨║╨╛╨╣)
  //   2. ╨Ш EMA-╤В╤А╨╡╨╜╨┤ ╤А╨░╤Б╤В╤Г╤Й╨╕╨╣ (raw > emaVoltage + ╬╡)
  // ╨У╨╕╤Б╤В╨╡╤А╨╡╨╖╨╕╤Б: ╤Б╨▒╤А╨╛╤Б╨╕╤В╤М charging ╤В╨╛╨╗╤М╨║╨╛ ╨┐╤А╨╕ V < 4.00 ╨Т ╨Ш╨Ы╨Ш ╤Г╤Б╤В╨╛╨╣╤З╨╕╨▓╨╛╨╝ ╨┐╨░╨┤╨╡╨╜╨╕╨╕

  const float CHG_ON_THRESH  = 4.05f;  // ╨▓╨║╨╗╤О╤З╨╕╤В╤М ╤А╨╡╨╢╨╕╨╝ ╨╖╨░╤А╤П╨┤╨║╨╕
  const float CHG_OFF_THRESH = 4.00f;  // ╨▓╤Л╨║╨╗╤О╤З╨╕╤В╤М ╤А╨╡╨╢╨╕╨╝ ╨╖╨░╤А╤П╨┤╨║╨╕ (╨│╨╕╤Б╤В╨╡╤А╨╡╨╖╨╕╤Б)
  const float CHG_DONE_THRESH = 4.18f; // "╨╖╨░╤А╤П╨┤ ╨╖╨░╨▓╨╡╤А╤И╤С╨╜" (TP4056 STDBY ~4.2 ╨Т)
  const float RISE_EPS        = 0.008f; // ╨╝╨╕╨╜╨╕╨╝╨░╨╗╤М╨╜╤Л╨╣ ╤А╨╛╤Б╤В ╨┤╨╗╤П ╤Г╤З╤С╤В╨░

  // ╨Х╤Б╨╗╨╕ ╨╖╨░╤А╤П╨┤╨╜╨╕╨║ ╤В╨╛╨╗╤М╨║╨╛ ╨┐╨╛╨┤╨║╨╗╤О╤З╨╕╨╗╨╕ тАФ ╨╝╨│╨╜╨╛╨▓╨╡╨╜╨╜╨╛ ╤Б╨▒╤А╨░╤Б╤Л╨▓╨░╨╡╨╝ EMA ╨╜╨░ raw
  // (╨╕╨╜╨░╤З╨╡ ╤Д╨╕╨╗╤М╤В╤А ╨▒╤Г╨┤╨╡╤В "╨┤╨╛╨│╨╛╨╜╤П╤В╤М" ╨╜╨╛╨▓╨╛╨╡ ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ ~3тАУ5 ╨╝╨╕╨╜╤Г╤В)
  if (!_prevCharging && raw > CHG_ON_THRESH && raw > _emaVoltage + 0.10f) {
    _emaVoltage = raw;
  }

  // EMA-╤Д╨╕╨╗╤М╤В╤А: ╨╜╨╛╨▓╨╛╨╡ = ╬▒ * ╨╖╨░╨╝╨╡╤А + (1 тИТ ╬▒) * ╤Б╤В╨░╤А╨╛╨╡
  _emaVoltage = EMA_ALPHA * raw + (1.0f - EMA_ALPHA) * _emaVoltage;
  _voltage    = _emaVoltage;
  _percent    = voltageToPct(_voltage);

  // ╨в╤А╨╡╨╜╨┤: raw ╤А╨░╤Б╤В╤С╤В ╨╛╤В╨╜╨╛╤Б╨╕╤В╨╡╨╗╤М╨╜╨╛ EMA (╤Г╤Б╤В╤А╨░╨╜╤П╨╡╤В ╨┤╤А╨╡╨╣╤Д)
  bool rising = (raw - _emaVoltage) > RISE_EPS;

  if (!_charging) {
    // ╨Т╨║╨╗╤О╤З╨╕╤В╤М ╨╖╨░╤А╤П╨┤╨║╤Г: ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ ╨▓╤Л╤Б╨╛╨║╨╛╨╡ ╨Ш ╤В╤А╨╡╨╜╨┤ ╨▓╨▓╨╡╤А╤Е
    if (_emaVoltage >= CHG_ON_THRESH && rising) {
      if (_risingCount < 255) _risingCount++;
      if (_risingCount >= 2) _charging = true;
    } else {
      _risingCount = 0;
    }
  } else {
    // ╨Т╤Л╨║╨╗╤О╤З╨╕╤В╤М ╨╖╨░╤А╤П╨┤╨║╤Г: ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ ╤Г╨┐╨░╨╗╨╛ ╨┐╨╛╨┤ ╨│╨╕╤Б╤В╨╡╤А╨╡╨╖╨╕╤Б ╨Ш╨Ы╨Ш ╤Г╤Б╤В╨╛╨╣╤З╨╕╨▓╨╛╨╡ ╨┐╨░╨┤╨╡╨╜╨╕╨╡
    if (_emaVoltage < CHG_OFF_THRESH || (!rising && raw < _emaVoltage - RISE_EPS)) {
      _charging    = false;
      _risingCount = 0;
      _stableCount = 0;
      _charged     = false;
    }
  }

  // "╨Ч╨░╤А╤П╨┤ ╨╖╨░╨▓╨╡╤А╤И╤С╨╜": charging=true, ╨╜╨░╨┐╤А╤П╨╢╨╡╨╜╨╕╨╡ ╤Б╤В╨░╨▒╨╕╨╗╤М╨╜╨╛ тЙе 4.18 ╨Т (╨╜╨╡ ╤А╨░╤Б╤В╤С╤В)
  if (_charging && _emaVoltage >= CHG_DONE_THRESH && !rising) {
    if (_stableCount < 255) _stableCount++;
    if (_stableCount >= 5) _charged = true;
  } else {
    if (!_charging) {
      _stableCount = 0;
      _charged     = false;
    }
  }

  // ╨Ч╨░╤Й╨╕╤В╨░ ╨╛╤В ╨│╨╗╤Г╨▒╨╛╨║╨╛╨│╨╛ ╤А╨░╨╖╤А╤П╨┤╨░ Li-Ion (< 3.00 ╨Т ╨┐╤А╨╕ ╨╛╤В╤Б╤Г╤В╤Б╤В╨▓╨╕╨╕ ╨╖╨░╤А╤П╨┤╨║╨╕)
  // ╨Я╤А╨╛╨▓╨╡╤А╤П╨╡╨╝ > 1.0 ╨Т, ╤З╤В╨╛╨▒╤Л ╨╕╤Б╨║╨╗╤О╤З╨╕╤В╤М ╨╜╨╡╨┐╨╛╨┤╨║╨╗╤О╤З╨╡╨╜╨╜╤Л╨╣ ╨┐╨╕╨╜ ╨┐╤А╨╕ ╤Б╤В╨╡╨╜╨┤╨╛╨▓╤Л╤Е ╤В╨╡╤Б╤В╨░╤Е
  if (_voltage > 1.0f && _voltage < 3.00f && !_charging) {
    if (_criticalCount < 255) _criticalCount++;
    if (_criticalCount >= 3) {
      _critical = true;
    }
  } else {
    _criticalCount = 0;
    _critical = false;
  }
}

float   BatteryManager::getVoltage() { return _voltage; }
uint8_t BatteryManager::getPercent() { return _percent; }
bool    BatteryManager::isCharging() { return _charging && !_charged; }
bool    BatteryManager::isCharged()  { return _charged; }

