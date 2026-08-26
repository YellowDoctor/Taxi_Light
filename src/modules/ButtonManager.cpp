// =====================================================================
//  ButtonManager.cpp — реализация обработки сенсорной кнопки
// =====================================================================
#include "ButtonManager.h"

ButtonManager Button;   // глобальный экземпляр

// Тайминги жестов, мс
static const uint32_t TAP_MAX      = 400;    // максимум для «одиночного» касания
static const uint32_t DOUBLE_GAP   = 300;    // окно между касаниями
static const uint32_t HOLD_MIN     = 400;    // минимум для удержания
static const uint32_t HOLD_MAX     = 2000;   // максимум «короткого» удержания
static const uint32_t LONG_HOLD    = 5000;   // долгое удержание

void ButtonManager::begin() {
  // touchRead не требует явной инициализации пина на ESP32
  _pressed = false;
  Serial.printf("[Button] Сенсор на touch-пине готов (порог %d)\n", TOUCH_THRESHOLD);
  #if USE_TTP223
  pinMode(BUTTON_PIN, INPUT);
  #endif
}

bool ButtonManager::readTouch()
{
#if USE_TTP223
    return digitalRead(BUTTON_PIN) == HIGH;
#else
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 3; i++)
        sum += touchRead(TOUCH_PIN);
    return (sum / 3) < TOUCH_THRESHOLD;
#endif
}

void ButtonManager::tick() {
  uint32_t now = millis();
  bool touched = readTouch();

  // ----- Начало касания -----
  if (touched && !_pressed) {
    _pressed    = true;
    _pressStart = now;
    _holdFired  = false;
    _longFired  = false;
  }

  // ----- Удержание (пока палец на сенсоре) -----
  if (touched && _pressed) {
    uint32_t held = now - _pressStart;

    // Долгое удержание -> запуск AP / сопряжение
    if (held >= LONG_HOLD && !_longFired) {
      _longFired = true;
      _holdFired = true;   // подавляем прочие жесты
      if (onLongHold) onLongHold();
    }
    // Короткое удержание -> повтор действия (яркость) каждые 150мс
    else if (held >= HOLD_MIN && held < LONG_HOLD) {
      if (!_holdFired || (now - _lastHoldStep >= 150)) {
        _holdFired    = true;
        _lastHoldStep = now;
        if (onHold) onHold();
      }
    }
  }

  // ----- Отпускание -----
  if (!touched && _pressed) {
    _pressed = false;  // исправлен баг: убрано лишнее _pressed = true
    uint32_t held = now - _pressStart;

    if (_holdFired || _longFired) {
      // жест удержания уже обработан
      // уведомляем о завершении удержания для сохранения яркости и т.п.
      if (_holdFired && !_longFired && onRelease) onRelease();
      _waitDouble = false;
    } else if (held < TAP_MAX) {
      // это касание-«тап»
      if (_waitDouble && (now - _releaseTime <= DOUBLE_GAP)) {
        // второе касание в окне -> двойное
        _waitDouble = false;
        if (onDouble) onDouble();
      } else {
        // первое касание — ждём возможного второго
        _waitDouble  = true;
        _releaseTime = now;
      }
    }
  }

  // ----- Истёк тайм-аут двойного касания -> считаем одиночным -----
  if (_waitDouble && (now - _releaseTime > DOUBLE_GAP)) {
    _waitDouble = false;
    if (onSingle) onSingle();
  }
}
