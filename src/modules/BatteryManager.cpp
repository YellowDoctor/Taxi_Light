// =====================================================================
//  BatteryManager.cpp — точное измерение аккумулятора
//  - esp_adc_cal: калибровка ADC по заводским данным из eFuse ESP32
//  - LUT 11 точек: реальная кривая разряда Li-Ion 21700
//  - EMA-фильтр (α = 0.15): убирает скачки при изменении нагрузки
//  - Зарядка: 3+ замера подряд с ростом напряжения
// =====================================================================
#include "BatteryManager.h"

BatteryManager Battery;

// ---------- LUT кривой разряда Li-Ion ----------
// Реальная кривая разряда Li-Ion (NCR21700 / Samsung 40T и подобные).
// Напряжение → процент заряда. Между точками — линейная интерполяция.
static const float LUT_V[]   = { 3.20, 3.40, 3.50, 3.60, 3.70, 3.75, 3.80, 3.85, 3.95, 4.10, 4.20 };
static const float LUT_PCT[] = { 0,    5,    10,   20,   35,   45,   55,   65,   80,   95,   100  };
static const int   LUT_SIZE  = sizeof(LUT_V) / sizeof(LUT_V[0]);

// ---------- EMA-фильтр ----------
static const float EMA_ALPHA = 0.15f;   // чем меньше — тем плавнее (0.1..0.3)

void BatteryManager::begin() {
  analogReadResolution(12);  // 0..4095
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  // esp_adc_cal: калибровка по заводским данным
  // ADC1, канал 6 = GPIO34, аттенюация 12dB (0..3.3V)
  _calibrated = (esp_adc_cal_characterize(
    ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &_adcChars
  ) != ESP_ADC_CAL_VAL_NOT_SUPPORTED);

  if (_calibrated) {
    Serial.println(F("[BAT] ADC откалиброван по eFuse"));
  } else {
    Serial.println(F("[BAT] Калибровка eFuse недоступна, используем формулу"));
  }


  // Первый замер — инициализируем EMA текущим значением
  float v = measureRaw();
  _emaVoltage = v;
  _voltage = v;
  _percent = voltageToPct(v);
  _lastUpdate = millis();

  Serial.printf("[BAT] Стартовое напряжение: %.3f В (%d%%)\n", _voltage, _percent);
}

// ---------- Замер напряжения с калибровкой ----------
float BatteryManager::measureRaw() {
  uint32_t sum = 0;

  if (_calibrated) {
    // esp_adc_cal даёт результат в милливольтах
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
      sum += esp_adc_cal_raw_to_voltage(analogRead(BATTERY_PIN), &_adcChars);
      delayMicroseconds(200);
    }
    float pinMv = (float)sum / BATTERY_SAMPLES;
    return (pinMv / 1000.0f) * BATTERY_DIVIDER_RATIO;
  } else {
    // Фолбэк: обычный расчёт
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
      sum += analogRead(BATTERY_PIN);
      delayMicroseconds(200);
    }
    float pinVoltage = ((float)sum / BATTERY_SAMPLES / 4095.0f) * 3.3f;
    return pinVoltage * BATTERY_DIVIDER_RATIO;
  }
}

// ---------- LUT-перевод напряжения в проценты ----------
uint8_t BatteryManager::voltageToPct(float v) {
  if (v <= LUT_V[0]) return 0;
  if (v >= LUT_V[LUT_SIZE - 1]) return 100;

  // Линейная интерполяция между ближайшими точками
  for (int i = 1; i < LUT_SIZE; i++) {
    if (v <= LUT_V[i]) {
      float t = (v - LUT_V[i - 1]) / (LUT_V[i] - LUT_V[i - 1]);
      float pct = LUT_PCT[i - 1] + t * (LUT_PCT[i] - LUT_PCT[i - 1]);
      return (uint8_t)(pct + 0.5f);
    }
  }
  return 100;
}

// ---------- Периодическое обновление ----------
void BatteryManager::tick() {
  uint32_t now = millis();
  if (now - _lastUpdate < BATTERY_UPDATE_MS) return;
  _lastUpdate = now;

  float raw = measureRaw();

  // Сохраняем состояние до обновления (для детектирования смены)
  _prevCharging = _charging;

  // ---- Детектирование зарядки ----
  // Признаки зарядки WITHOUT CHRG-пина (Вариант A):
  //   1. Напряжение > 4.05 В (Li-Ion при разряде редко превышает 4.05 В под нагрузкой)
  //   2. И EMA-тренд растущий (raw > emaVoltage + ε)
  // Гистерезис: сбросить charging только при V < 4.00 В ИЛИ устойчивом падении

  const float CHG_ON_THRESH  = 4.05f;  // включить режим зарядки
  const float CHG_OFF_THRESH = 4.00f;  // выключить режим зарядки (гистерезис)
  const float CHG_DONE_THRESH = 4.18f; // "заряд завершён" (TP4056 STDBY ~4.2 В)
  const float RISE_EPS        = 0.008f; // минимальный рост для учёта

  // Если зарядник только подключили — мгновенно сбрасываем EMA на raw
  // (иначе фильтр будет "догонять" новое напряжение ~3–5 минут)
  if (!_prevCharging && raw > CHG_ON_THRESH && raw > _emaVoltage + 0.10f) {
    _emaVoltage = raw;
  }

  // EMA-фильтр: новое = α * замер + (1 − α) * старое
  _emaVoltage = EMA_ALPHA * raw + (1.0f - EMA_ALPHA) * _emaVoltage;
  _voltage    = _emaVoltage;
  _percent    = voltageToPct(_voltage);

  // Тренд: raw растёт относительно EMA (устраняет дрейф)
  bool rising = (raw - _emaVoltage) > RISE_EPS;

  if (!_charging) {
    // Включить зарядку: напряжение высокое И тренд вверх
    if (_emaVoltage >= CHG_ON_THRESH && rising) {
      if (_risingCount < 255) _risingCount++;
      if (_risingCount >= 2) _charging = true;
    } else {
      _risingCount = 0;
    }
  } else {
    // Выключить зарядку: напряжение упало под гистерезис ИЛИ устойчивое падение
    if (_emaVoltage < CHG_OFF_THRESH || (!rising && raw < _emaVoltage - RISE_EPS)) {
      _charging    = false;
      _risingCount = 0;
      _stableCount = 0;
      _charged     = false;
    }
  }

  // "Заряд завершён": charging=true, напряжение стабильно ≥ 4.18 В (не растёт)
  if (_charging && _emaVoltage >= CHG_DONE_THRESH && !rising) {
    if (_stableCount < 255) _stableCount++;
    if (_stableCount >= 5) _charged = true;
  } else {
    if (!_charging) {
      _stableCount = 0;
      _charged     = false;
    }
  }

  // Защита от глубокого разряда Li-Ion (< 3.00 В при отсутствии зарядки)
  // Проверяем > 1.0 В, чтобы исключить неподключенный пин при стендовых тестах
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

