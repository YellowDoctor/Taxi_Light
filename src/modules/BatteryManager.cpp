// =====================================================================
//  BatteryManager.cpp — точное измерение аккумулятора
//  - esp_adc_cal: калибровка ADC по заводским данным из eFuse ESP32
//  - LUT 11 точек: реальная кривая разряда Li-Ion 21700
//  - EMA-фильтр (α = 0.15): убирает скачки при изменении нагрузки
//  - Программная компенсация просадки под нагрузкой светодиодов (IR Drop)
//  - Детектирование зарядки по скачку шага и тренду (без порога 4.05В)
// =====================================================================
#include "BatteryManager.h"
#include "SettingsManager.h"

BatteryManager Battery;

// ---------- LUT кривой разряда Li-Ion ----------
// Реальная кривая разряда Li-Ion (NCR21700 / Samsung 40T и подобные).
// Напряжение → процент заряда. Между точками — линейная интерполяция.
static const float LUT_V[]   = { 3.20, 3.40, 3.50, 3.60, 3.70, 3.75, 3.80, 3.85, 3.95, 4.10, 4.20 };
static const float LUT_PCT[] = { 0,    5,    10,   20,   35,   45,   55,   65,   80,   95,   100  };
static const int   LUT_SIZE  = sizeof(LUT_V) / sizeof(LUT_V[0]);

// ---------- EMA-фильтр ----------
static const float EMA_ALPHA = 0.15f;   // сглаживание

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

  // Первый замер
  float v = measureRaw();
  _emaVoltage = v;
  _voltage = v;

  float vSag = Config.data.isOn ? (0.14f * ((float)Config.data.brightness / 255.0f)) : 0.0f;
  _emaCompVoltage = v + vSag;
  _percent = voltageToPct(_emaCompVoltage);

  _prevLampOn = Config.data.isOn;
  _lastLampToggle = millis();
  _lastUpdate = millis();

  // Если при старте напряжение уже высокое — устройство на зарядке
  if (v >= 4.08f) {
    _charging = true;
  }

  Serial.printf("[BAT] Стартовое напряжение: %.3f В (скомпенс: %.3f В, %d%%)\n",
                _voltage, _emaCompVoltage, _percent);
}

// ---------- Замер напряжения с калибровкой ----------
float BatteryManager::measureRaw() {
  uint32_t sum = 0;

  if (_calibrated) {
    // esp_adc_cal даёт результат в милливольтах
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
      sum += esp_adc_cal_raw_to_voltage(analogRead(BATTERY_PIN), &_adcChars);
      delayMicroseconds(600);
    }
    float pinMv = (float)sum / BATTERY_SAMPLES;
    return (pinMv / 1000.0f) * BATTERY_DIVIDER_RATIO;
  } else {
    // Фолбэк: обычный расчёт
    for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
      sum += analogRead(BATTERY_PIN);
      delayMicroseconds(600);
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

  // Отслеживаем переключение питания подсветки для подавления ложных скачков
  if (Config.data.isOn != _prevLampOn) {
    _prevLampOn = Config.data.isOn;
    _lastLampToggle = now;
  }
  bool lampRecentlyToggled = (now - _lastLampToggle < 20000);

  // Компенсация просадки напряжения под нагрузкой светодиодов (IR Drop)
  // При 34 светодиодах и токе до 1.2А на сопротивлении цепи ~0.15 Ом просадка ~0.14 В
  float vSag = Config.data.isOn ? (0.14f * ((float)Config.data.brightness / 255.0f)) : 0.0f;
  float compV = raw + vSag;

  // EMA фильтрация
  _emaVoltage     = EMA_ALPHA * raw   + (1.0f - EMA_ALPHA) * _emaVoltage;
  _emaCompVoltage = EMA_ALPHA * compV + (1.0f - EMA_ALPHA) * _emaCompVoltage;

  _voltage = _emaVoltage;
  _percent = voltageToPct(_emaCompVoltage);

  // ---- Детектирование зарядки (без барьера 4.05В) ----
  const float CHG_DONE_THRESH = 4.16f; // Порог полного заряда
  float deltaComp = compV - _emaCompVoltage;

  if (!_charging) {
    // Включение режима зарядки:
    // 1. Ступенчатый скачок вверх (подключение кабеля дает +30..80 мВ)
    if (!lampRecentlyToggled && deltaComp >= 0.025f) {
      _risingCount += 2;
    }
    // 2. Устойчивый рост напряжения во времени (тренд вверх при CC-зарядке)
    else if (deltaComp > 0.002f) {
      if (_risingCount < 255) _risingCount++;
    }
    // 3. Если уже высокое напряжение (аккумулятор почти полон на зарядке)
    else if (_emaVoltage >= 4.08f && deltaComp >= -0.003f) {
      if (_risingCount < 255) _risingCount++;
    }
    // Если напряжение явно падает — сбрасываем счётчик
    else if (deltaComp < -0.006f) {
      _risingCount = 0;
    }

    if (_risingCount >= 3) {
      _charging = true;
      _risingCount = 0;
      _dropCount = 0;
      Serial.printf("[BAT] Зарядка включена: %.3f В (скомпенс: %.3f В)\n", _voltage, _emaCompVoltage);
    }
  } else {
    // Выключение режима зарядки (отключение кабеля питания):
    // 1. Ступенчатый скачок вниз (отключение кабеля дает -30..80 мВ)
    if (!lampRecentlyToggled && deltaComp <= -0.025f) {
      _dropCount += 2;
    }
    // 2. Устойчивое падение напряжения во времени
    else if (deltaComp < -0.004f) {
      if (_dropCount < 255) _dropCount++;
    }
    // Напряжение растёт — сбрасываем счётчик падения
    else if (deltaComp >= 0.002f) {
      _dropCount = 0;
    }

    if (_dropCount >= 3) {
      _charging = false;
      _charged = false;
      _dropCount = 0;
      _risingCount = 0;
      _stableCount = 0;
      Serial.printf("[BAT] Зарядка отключена: %.3f В\n", _voltage);
    }
  }

  // Определение "Заряд завершён"
  if (_charging && _emaVoltage >= CHG_DONE_THRESH) {
    if (fabs(raw - _emaVoltage) < 0.008f) {
      if (_stableCount < 255) _stableCount++;
      if (_stableCount >= 5) _charged = true;
    } else {
      _stableCount = 0;
    }
  } else {
    if (!_charging) {
      _stableCount = 0;
      _charged = false;
    }
  }

  // Защита от глубокого разряда Li-Ion (< 3.00 В)
  // Проверяем скомпенсированное напряжение, чтобы не уснуть ложно при кратковременной просадке от света
  if (_emaCompVoltage > 1.0f && _emaCompVoltage < 3.00f && !_charging) {
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
