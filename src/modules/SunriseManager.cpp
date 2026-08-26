// =====================================================================
//  SunriseManager.cpp — реализация будильника-рассвета
// =====================================================================
#include "SunriseManager.h"
#include "LedManager.h"
#include "SettingsManager.h"

SunriseManager Sunrise;   // глобальный экземпляр

void SunriseManager::begin() {
  _active = false;
}

void SunriseManager::start(uint8_t durationMin, uint8_t targetBrightness) {
  if (durationMin < 1)  durationMin = 1;
  if (durationMin > SUNRISE_MAX_MINUTES) durationMin = SUNRISE_MAX_MINUTES;
  if (targetBrightness < 5) targetBrightness = 5;

  _active           = true;
  _startMs          = millis();
  _durationMs       = (uint32_t)durationMin * 60UL * 1000UL;
  _targetBrightness = targetBrightness;
  _lastTick         = millis();

  // Включаем ленту
  if (!Config.data.isOn) {
    Config.data.isOn = true;
    Led.turnOn();
  }

  // Начинаем с нулевой яркости
  Led.setBrightness(1);
  Serial.printf("[Sunrise] Запуск: %d мин, целевая яркость %d\n", durationMin, targetBrightness);
}

void SunriseManager::cancel() {
  if (!_active) return;
  _active = false;
  // Восстанавливаем яркость из настроек
  Led.setBrightness(Config.data.brightness);
  Serial.println(F("[Sunrise] Отменён"));
}

uint32_t SunriseManager::secondsLeft() const {
  if (!_active) return 0;
  uint32_t elapsed = millis() - _startMs;
  if (elapsed >= _durationMs) return 0;
  return (_durationMs - elapsed) / 1000UL;
}

void SunriseManager::tick() {
  if (!_active) return;

  uint32_t now     = millis();
  uint32_t elapsed = now - _startMs;

  // Обновляем не чаще 1 раза в секунду (рассвет медленный)
  if (now - _lastTick < 1000) return;
  _lastTick = now;

  if (elapsed >= _durationMs) {
    // Рассвет завершён — фиксируем финальную яркость
    Led.setBrightness(_targetBrightness);
    Config.data.brightness = _targetBrightness;
    Config.save();
    _active = false;
    Serial.println(F("[Sunrise] Завершён"));
    return;
  }

  // Прогресс 0.0 .. 1.0
  float t = (float)elapsed / (float)_durationMs;

  // Яркость: квадратичная кривая (медленный старт)
  uint8_t bright = (uint8_t)(_targetBrightness * t * t);
  if (bright < 1) bright = 1;
  Led.setBrightness(bright);

  // Цвет меняется от глубокого красного (рассвет) к тёплому белому
  // Начальный цвет: CRGB(255, 10, 0) — тёмно-красный
  // Конечный цвет: тот, что задан в настройках (Config.data.color)
  uint8_t cr = (Config.data.color >> 16) & 0xFF;
  uint8_t cg = (Config.data.color >>  8) & 0xFF;
  uint8_t cb =  Config.data.color        & 0xFF;

  uint8_t r = lerp8by8(255, cr, (uint8_t)(t * 255));
  uint8_t g = lerp8by8(10,  cg, (uint8_t)(t * 255));
  uint8_t b = lerp8by8(0,   cb, (uint8_t)(t * 255));

  fill_solid(Led.leds, LED_COUNT, CRGB(r, g, b));
  Led.show();
}
