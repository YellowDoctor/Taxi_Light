// =====================================================================
//  BatteryManager.cpp — реализация измерения аккумулятора
// =====================================================================
#include "BatteryManager.h"

BatteryManager Battery;   // глобальный экземпляр

void BatteryManager::begin() {
  // GPIO34 — вход ADC. Диапазон 0..3.3В при ослаблении 11dB
  analogReadResolution(12);            // 0..4095
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);
  measure();
  _lastVoltage = _voltage;
  _lastUpdate = millis();
}

void BatteryManager::measure() {
  uint32_t raw = 0;
  for (uint8_t i = 0; i < BATTERY_SAMPLES; i++) {
    raw += analogRead(BATTERY_PIN);
    delayMicroseconds(200);
  }
  raw /= BATTERY_SAMPLES;

  // Перевод отсчёта ADC в напряжение на пине (0..3.3В)
  float pinVoltage = (raw / 4095.0f) * 3.3f;
  // Учёт делителя напряжения
  _voltage = pinVoltage * BATTERY_DIVIDER_RATIO;

  // Перевод в проценты по линейной кривой между min и max
  float pct = (_voltage - BATTERY_MIN_VOLTAGE) /
              (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0f;
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  _percent = (uint8_t)(pct + 0.5f);
}

void BatteryManager::tick() {
  uint32_t now = millis();
  if (now - _lastUpdate < BATTERY_UPDATE_MS) return;
  _lastUpdate = now;

  float prev = _voltage;
  measure();
  // Грубое определение зарядки: устойчивый рост напряжения
  _charging = (_voltage - prev) > 0.02f;
  _lastVoltage = prev;
}

float BatteryManager::getVoltage() { return _voltage; }
uint8_t BatteryManager::getPercent() { return _percent; }
bool BatteryManager::isCharging() { return _charging; }
