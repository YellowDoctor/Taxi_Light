// =====================================================================
//  EffectsManager.cpp — реализация световых эффектов (millis-based)
// =====================================================================
#include "EffectsManager.h"

EffectsManager Effects; // глобальный экземпляр

void EffectsManager::begin()
{
  memset(_heat, 0, sizeof(_heat));
  _lastTick = millis();
  _policeStep = 0;
  _policeLastChange = millis();
}

void EffectsManager::setEffect(uint8_t id)
{
  if (id >= EFFECT_COUNT)
    id = EFFECT_STATIC;
  _effect = id;
  // Сброс тепловой карты при переходе на «огонь»
  if (id == EFFECT_FIRE)
    memset(_heat, 0, sizeof(_heat));

  // Сброс полицейской мигалки
  if (id == EFFECT_POLICE_FLASH)
  {
    _policeStep = 0;
    _policeLastChange = millis();
  }

  // Сброс общего оттенка
  if (id == EFFECT_AURORA ||
      id == EFFECT_OCEAN)
  {
    _hue = 0;
  }

  if (id == EFFECT_METEOR)  { _meteorPos = 0; }
  if (id == EFFECT_LASER)   { _laserPos = 0; _laserDir = 1; }
  if (id == EFFECT_STROBE)  { _strobeOn = false; }
}

void EffectsManager::setSpeed(uint8_t v)
{
  _speed = v;
}

void EffectsManager::setColor(uint8_t r, uint8_t g, uint8_t b)
{
  _color = CRGB(r, g, b);
}

// Чем выше скорость, тем меньше интервал между кадрами (5..80 мс)
uint16_t EffectsManager::frameInterval()
{
  return map(_speed, 0, 255, 80, 5);
}

void EffectsManager::tick()
{
  if (!Led.isOn())
    return;

  uint32_t now = millis();
  // static обновляется реже (не требует анимации)
  uint16_t interval = (_effect == EFFECT_STATIC) ? 200 : frameInterval();
  if (now - _lastTick < interval)
    return;
  _lastTick = now;

  switch (_effect)
  {
  case EFFECT_STATIC:
    effStatic();
    break;
  case EFFECT_BREATHE:
    effBreathe();
    break;
  case EFFECT_RAINBOW:
    effRainbow();
    break;
  case EFFECT_FIRE:
    effFire();
    break;
  case EFFECT_CANDLE:
    effCandle();
    break;
  case EFFECT_COLORFLOW:
    effColorflow();
    break;
  case EFFECT_AURORA:
    effAurora();
    break;
  case EFFECT_OCEAN:
    effOcean();
    break;
  case EFFECT_POLICE:
    effPolice();
    break;
  case EFFECT_POLICE_FLASH:
    effPoliceFlash();
    break;
  case EFFECT_METEOR:     effMeteor();    break;
  case EFFECT_STROBE:     effStrobe();    break;
  case EFFECT_RUNNING:    effRunning();   break;
  case EFFECT_NIGHTSKY:   effNightsky();  break;
  case EFFECT_LASER:      effLaser();     break;
  case EFFECT_FLASH:      effFlash();     break;
  default:
    effStatic();
    break;
  }
  Led.show();
}

// --- 0. Статика: просто заданный цвет ---
void EffectsManager::effStatic()
{
  fill_solid(Led.leds, LED_COUNT, _color);
}

// --- 1. Дыхание: синусоидальное изменение яркости заданного цвета ---
void EffectsManager::effBreathe()
{
  // beatsin8 даёт плавную синусоиду; частота зависит от скорости
  uint8_t bpm = map(_speed, 0, 255, 4, 30);
  uint8_t b = beatsin8(bpm, 20, 255);
  CRGB c = _color;
  c.nscale8_video(b);
  fill_solid(Led.leds, LED_COUNT, c);
}

// --- 2. Радуга: бегущая по всей ленте ---
void EffectsManager::effRainbow()
{
  _hue++;
  fill_rainbow(Led.leds, LED_COUNT, _hue, 255 / LED_COUNT);
}

// --- 3. Огонь: классический алгоритм с тепловой картой ---
void EffectsManager::effFire()
{
  const uint8_t cooling = 55;
  const uint8_t sparking = 120;

  // 1. Охлаждение каждой ячейки
  for (int i = 0; i < LED_COUNT; i++)
  {
    uint8_t cooldown = random8(0, ((cooling * 10) / LED_COUNT) + 2);
    _heat[i] = (cooldown >= _heat[i]) ? 0 : _heat[i] - cooldown;
  }
  // 2. Тепло поднимается вверх
  for (int i = LED_COUNT - 1; i >= 2; i--)
  {
    _heat[i] = (_heat[i - 1] + _heat[i - 2] + _heat[i - 2]) / 3;
  }
  // 3. Случайные искры у основания
  if (random8() < sparking)
  {
    int y = random8(min(7, LED_COUNT));
    _heat[y] = qadd8(_heat[y], random8(160, 255));
  }
  // 4. Перевод тепла в цвет
  for (int i = 0; i < LED_COUNT; i++)
  {
    Led.leds[i] = HeatColor(_heat[i]);
  }
}

// --- 4. Свеча: тёплые случайные вспышки ---
void EffectsManager::effCandle()
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    // базовый тёплый жёлто-оранжевый + случайное мерцание яркости
    uint8_t flicker = random8(100, 255);
    CRGB c = CRGB(255, 100, 10);
    c.nscale8_video(flicker);
    // плавное смешивание для мягкого перехода
    Led.leds[i] = blend(Led.leds[i], c, 90);
  }
}

// --- 5. Цветовой поток: плавная смена цвета через HSV ---
void EffectsManager::effColorflow()
{
  _hue++;
  fill_solid(Led.leds, LED_COUNT, CHSV(_hue, 255, 255));
}

// =====================================================================
// 6. СЕВЕРНОЕ СИЯНИЕ
// =====================================================================
//
// Медленные плавные зелёно-синие-фиолетовые переливы.
// Используется несколько волн, чтобы свечение не выглядело
// как обычная смена цветов.
//

void EffectsManager::effAurora()
{
  _hue++;

  for (int i = 0; i < LED_COUNT; i++)
  {
    // Пространственная волна
    uint8_t wave =
        sin8(
            _hue * 2 +
            i * 9);

    // Вторая, более медленная волна
    uint8_t wave2 =
        sin8(
            _hue +
            i * 5);

    uint8_t brightness =
        qadd8(
            50,
            scale8(wave, 180));

    uint8_t hue =
        90 +
        scale8(wave2, 90);

    CRGB target =
        CHSV(
            hue,
            200,
            brightness);

    // Плавное смешивание
    Led.leds[i] =
        blend(
            Led.leds[i],
            target,
            35);
  }
}

// =====================================================================
// 7. ОКЕАН
// =====================================================================
//
// Спокойные синие и бирюзовые волны.
//

void EffectsManager::effOcean()
{
  _hue++;

  for (int i = 0; i < LED_COUNT; i++)
  {
    uint8_t wave =
        sin8(
            _hue * 2 +
            i * 12);

    uint8_t wave2 =
        sin8(
            _hue +
            i * 6);

    uint8_t brightness =
        qadd8(
            40,
            scale8(wave, 190));

    uint8_t hue =
        135 +
        scale8(wave2, 35);

    CRGB target =
        CHSV(
            hue,
            220,
            brightness);

    Led.leds[i] =
        blend(
            Led.leds[i],
            target,
            45);
  }
}

// =====================================================================
// 8. ПОЛИЦИЯ — КРАСНЫЙ / СИНИЙ
// =====================================================================
//
// LED 1-7   = RED
// LED 8-26  = BLUE
// LED 27-34 = RED
//
// Эффект попеременно усиливает красную и синюю часть.
//

void EffectsManager::effPolice()
{
  static bool redPhase = false;

  uint32_t now = millis();

  // Скорость переключения
  uint16_t interval =
      map(
          _speed,
          0,
          255,
          900,
          100);

  static uint32_t lastChange = 0;

  if (now - lastChange >= interval)
  {
    lastChange = now;
    redPhase = !redPhase;
  }

  uint8_t brightness =
      beatsin8(
          map(_speed, 0, 255, 8, 25),
          80,
          255);

  for (int i = 0; i < LED_COUNT; i++)
  {
    // Номер LED: 1..34
    int ledNumber = i + 1;

    bool redZone =
        (ledNumber >= 1 && ledNumber <= 7) ||
        (ledNumber >= 27 && ledNumber <= 34);

    if (redZone)
    {
      uint8_t b =
          redPhase
              ? brightness
              : brightness / 4;

      Led.leds[i] =
          CRGB(
              b,
              0,
              0);
    }
    else
    {
      uint8_t b =
          redPhase
              ? brightness / 4
              : brightness;

      Led.leds[i] =
          CRGB(
              0,
              0,
              b);
    }
  }
}

// =====================================================================
// 9. ПОЛИЦЕЙСКАЯ МИГАЛКА
// =====================================================================
//
// Последовательность:
//
// Синий:  вспышка
// Синий:  вспышка
// пауза
// Красный: вспышка
// Красный: вспышка
// пауза
//
// Красная зона:
// LED 1-7
// LED 27-34
//
// Синяя зона:
// LED 8-26
//

void EffectsManager::effPoliceFlash()
{
  uint32_t now = millis();

  // Скорость всей последовательности
  uint16_t stepTime =
      map(
          _speed,
          0,
          255,
          350,
          70);

  if (now - _policeLastChange >= stepTime)
  {
    _policeLastChange = now;

    _policeStep++;

    if (_policeStep >= 6)
      _policeStep = 0;
  }

  // -------------------------------------------------------------
  // Определяем состояние
  // -------------------------------------------------------------

  bool blueFlash = false;
  bool redFlash = false;

  switch (_policeStep)
  {
  case 0:
    blueFlash = true;
    break;

  case 1:
    blueFlash = true;
    break;

  case 2:
    // пауза
    break;

  case 3:
    redFlash = true;
    break;

  case 4:
    redFlash = true;
    break;

  case 5:
    // пауза
    break;
  }

  // -------------------------------------------------------------
  // Плавность вспышки
  // -------------------------------------------------------------

  uint8_t brightness = 255;

  if (_policeStep == 0 ||
      _policeStep == 3)
  {
    brightness = 255;
  }
  else if (_policeStep == 1 ||
           _policeStep == 4)
  {
    brightness = 180;
  }
  else
  {
    brightness = 0;
  }

  if (blueFlash)
  {
    brightness = qadd8(
        brightness,
        0);
  }
  // -------------------------------------------------------------
  // Заполняем 34 светодиода
  // -------------------------------------------------------------

  for (int i = 0; i < LED_COUNT; i++)
  {
    int ledNumber = i + 1;

    bool redZone =
        (ledNumber >= 1 && ledNumber <= 7) ||
        (ledNumber >= 27 && ledNumber <= 34);

    if (redZone)
    {
      Led.leds[i] =
          redFlash
              ? CRGB(brightness, 0, 0)
              : CRGB::Black;
    }
    else
    {
      Led.leds[i] =
          blueFlash
              ? CRGB(0, 0, brightness)
              : CRGB::Black;
    }
  }
}

// =====================================================================
// 10. МЕТЕОР — яркая точка с затухающим хвостом
// =====================================================================
void EffectsManager::effMeteor()
{
  const uint8_t TAIL = 8;      // длина хвоста
  const uint8_t FADE = 80;     // скорость затухания хвоста (0..255)

  // Плавное затухание всех пикселей
  for (int i = 0; i < LED_COUNT; i++)
    Led.leds[i].nscale8(255 - FADE);

  // Голова метеора — полный цвет
  Led.leds[_meteorPos] = _color;

  // Хвост — убывающая яркость
  for (uint8_t t = 1; t < TAIL && (int)_meteorPos - t >= 0; t++) {
    uint8_t b = 255 - (uint8_t)((255.0f / TAIL) * t);
    CRGB c = _color;
    c.nscale8(b);
    Led.leds[_meteorPos - t] = blend(Led.leds[_meteorPos - t], c, 180);
  }

  _meteorPos++;
  if (_meteorPos >= LED_COUNT) _meteorPos = 0;
}

// =====================================================================
// 11. СТРОБОСКОП — быстрые вспышки заданного цвета
// =====================================================================
void EffectsManager::effStrobe()
{
  _strobeOn = !_strobeOn;
  if (_strobeOn)
    fill_solid(Led.leds, LED_COUNT, _color);
  else
    fill_solid(Led.leds, LED_COUNT, CRGB::Black);
}

// =====================================================================
// 12. БЕГУЩИЕ ОГНИ — несколько ярких точек движутся по ленте
// =====================================================================
void EffectsManager::effRunning()
{
  _hue++;
  const uint8_t SPACING = LED_COUNT / 4;   // 4 огня

  fill_solid(Led.leds, LED_COUNT, CRGB::Black);
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t pos = (_hue / 2 + i * SPACING) % LED_COUNT;
    Led.leds[pos] = _color;
    // Мягкий ореол вокруг точки
    if (pos > 0)           Led.leds[pos - 1] = blend(CRGB::Black, _color, 80);
    if (pos < LED_COUNT-1) Led.leds[pos + 1] = blend(CRGB::Black, _color, 80);
  }
}

// =====================================================================
// 13. НОЧНОЕ НЕБО — редкие мерцающие «звёзды» на тёмном фоне
// =====================================================================
void EffectsManager::effNightsky()
{
  // Медленное угасание всех пикселей
  for (int i = 0; i < LED_COUNT; i++)
    Led.leds[i].nscale8(220);

  // Случайно зажигаем новую «звезду» — вероятность зависит от скорости
  uint8_t chance = map(_speed, 0, 255, 3, 40);
  if (random8() < chance) {
    uint8_t pos = random8(LED_COUNT);
    uint8_t bright = random8(150, 255);
    Led.leds[pos] = CRGB(bright, bright, bright);   // белая звезда
  }
}

// =====================================================================
// 14. ЛАЗЕР — одна яркая точка, сканирующая ленту туда-обратно
// =====================================================================
void EffectsManager::effLaser()
{
  // Затухание шлейфа
  for (int i = 0; i < LED_COUNT; i++)
    Led.leds[i].nscale8(180);

  // Позиция луча
  Led.leds[(uint8_t)_laserPos] = _color;

  // Движение
  _laserPos += _laserDir;
  if (_laserPos >= LED_COUNT) { _laserPos = LED_COUNT - 2; _laserDir = -1; }
  if (_laserPos < 0)           { _laserPos = 1;             _laserDir =  1; }
}

// =====================================================================
// 15. СЛУЧАЙНЫЕ ВСПЫШКИ — внезапные яркие импульсы отдельных LED
// =====================================================================
void EffectsManager::effFlash()
{
  // Угасание
  for (int i = 0; i < LED_COUNT; i++)
    Led.leds[i].nscale8(200);

  // Случайные вспышки
  uint8_t chance = map(_speed, 0, 255, 5, 60);
  for (uint8_t i = 0; i < 3; i++) {
    if (random8() < chance) {
      uint8_t pos = random8(LED_COUNT);
      Led.leds[pos] = _color;
    }
  }
}