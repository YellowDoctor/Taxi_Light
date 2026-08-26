#pragma once
// =====================================================================
//  SettingsManager — хранение настроек в NVS (Preferences)
// =====================================================================
#include <Arduino.h>
#include <Preferences.h>
#include "../Config.h"

// Структура всех пользовательских настроек устройства
struct Settings {
  String   deviceName;      // имя устройства
  uint8_t  brightness;      // яркость 0–255
  bool     isOn;            // включено / выключено
  uint8_t  currentEffect;   // текущий эффект 0..EFFECT_COUNT-1
  uint32_t color;           // цвет HEX RGB (0xRRGGBB)
  uint8_t  effectSpeed;     // скорость эффекта 0–255
  String   wifiSSID;        // сохранённая сеть
  String   wifiPassword;    // пароль сети
  int8_t   timezone;        // часовой пояс UTC-12..UTC+14
  uint8_t  powerOnMode;     // 0=last, 1=off, 2=fixed
  uint8_t  touchAction1;    // действие: одно касание
  uint8_t  touchAction2;    // действие: двойное касание
  uint8_t  touchAction3;    // действие: удержание
};

class SettingsManager {
public:
  Settings data;

  void begin();     // загрузка при старте
  void load();      // чтение из NVS
  void save();      // запись в NVS
  void reset();     // сброс к заводским значениям

private:
  Preferences prefs;
  void applyDefaults();
};

extern SettingsManager Config;   // глобальный экземпляр
