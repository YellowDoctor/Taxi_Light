// =====================================================================
//  EffectsManager.cpp — реализация световых эффектов (millis-based)
// =====================================================================
#include "EffectsManager.h"

EffectsManager Effects;   // глобальный экземпляр

void EffectsManager::begin() {
  memset(_heat, 0, sizeof(_heat));
  _lastTick = millis();
}

void EffectsManager::setEffect(uint8_t id) {
  if (id >= EFFECT_COUNT) id = EFFECT_STATIC;
  _effect = id;
  // Сброс тепловой карты при переходе на «огонь»
  if (id == EFFECT_FIRE) memset(_heat, 0, sizeof(_heat));
}

void EffectsManager::setSpeed(uint8_t v) {
  _speed = v;
}

void EffectsManager::setColor(uint8_t r, uint8_t g, uint8_t b) {
  _color = CRGB(r, g, b);
}

// Чем выше скорость, тем меньше интервал между кадрами (5..80 мс)
uint16_t EffectsManager::frameInterval() {
  return map(_speed, 0, 255, 80, 5);
}

void EffectsManager::tick() {
  if (!Led.isOn()) return;

  uint32_t now = millis();
  // static обновляется реже (не требует анимации)
  uint16_t interval = (_effect == EFFECT_STATIC) ? 200 : frameInterval();
  if (now - _lastTick < interval) return;
  _lastTick = now;

  switch (_effect) {
    case EFFECT_STATIC:    effStatic();    break;
    case EFFECT_BREATHE:   effBreathe();   break;
    case EFFECT_RAINBOW:   effRainbow();   break;
    case EFFECT_FIRE:      effFire();      break;
    case EFFECT_CANDLE:    effCandle();    break;
    case EFFECT_COLORFLOW: effColorflow(); break;
    default:               effStatic();    break;
  }
  Led.show();
}

// --- 0. Статика: просто заданный цвет ---
void EffectsManager::effStatic() {
  fill_solid(Led.leds, LED_COUNT, _color);
}

// --- 1. Дыхание: синусоидальное изменение яркости заданного цвета ---
void EffectsManager::effBreathe() {
  // beatsin8 даёт плавную синусоиду; частота зависит от скорости
  uint8_t bpm = map(_speed, 0, 255, 4, 30);
  uint8_t b = beatsin8(bpm, 20, 255);
  CRGB c = _color;
  c.nscale8_video(b);
  fill_solid(Led.leds, LED_COUNT, c);
}

// --- 2. Радуга: бегущая по всей ленте ---
void EffectsManager::effRainbow() {
  _hue++;
  fill_rainbow(Led.leds, LED_COUNT, _hue, 255 / LED_COUNT);
}

// --- 3. Огонь: классический алгоритм с тепловой картой ---
void EffectsManager::effFire() {
  const uint8_t cooling  = 55;
  const uint8_t sparking = 120;

  // 1. Охлаждение каждой ячейки
  for (int i = 0; i < LED_COUNT; i++) {
    uint8_t cooldown = random8(0, ((cooling * 10) / LED_COUNT) + 2);
    _heat[i] = (cooldown >= _heat[i]) ? 0 : _heat[i] - cooldown;
  }
  // 2. Тепло поднимается вверх
  for (int i = LED_COUNT - 1; i >= 2; i--) {
    _heat[i] = (_heat[i - 1] + _heat[i - 2] + _heat[i - 2]) / 3;
  }
  // 3. Случайные искры у основания
  if (random8() < sparking) {
    int y = random8(min(7, LED_COUNT));
    _heat[y] = qadd8(_heat[y], random8(160, 255));
  }
  // 4. Перевод тепла в цвет
  for (int i = 0; i < LED_COUNT; i++) {
    Led.leds[i] = HeatColor(_heat[i]);
  }
}

// --- 4. Свеча: тёплые случайные вспышки ---
void EffectsManager::effCandle() {
  for (int i = 0; i < LED_COUNT; i++) {
    // базовый тёплый жёлто-оранжевый + случайное мерцание яркости
    uint8_t flicker = random8(100, 255);
    CRGB c = CRGB(255, 100, 10);
    c.nscale8_video(flicker);
    // плавное смешивание для мягкого перехода
    Led.leds[i] = blend(Led.leds[i], c, 90);
  }
}

// --- 5. Цветовой поток: плавная смена цвета через HSV ---
void EffectsManager::effColorflow() {
  _hue++;
  fill_solid(Led.leds, LED_COUNT, CHSV(_hue, 255, 255));
}
