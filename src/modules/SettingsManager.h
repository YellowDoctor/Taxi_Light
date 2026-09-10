#pragma once
// =====================================================================
//  SettingsManager — хранение настроек в NVS (Preferences)
// =====================================================================
#include <Arduino.h>
#include <Preferences.h>
#include "../Config.h"

// -----------------------------------------------------------------------
//  Одна избранная сцена
// -----------------------------------------------------------------------
struct FavoriteScene {
  bool     used;                        // слот занят
  char     name[FAVORITES_NAME_LEN];    // имя сцены
  uint32_t color;                       // цвет HEX RGB
  uint8_t  effect;                      // ID эффекта
  uint8_t  brightness;                  // яркость
  uint8_t  speed;                       // скорость эффекта
};

// -----------------------------------------------------------------------
//  Одна запись расписания
// -----------------------------------------------------------------------
struct Schedule {
  bool    used;        // слот активен (создан пользователем)
  bool    enabled;     // включена ли запись
  uint8_t hour;        // час 0–23
  uint8_t minute;      // минута 0–59
  bool    action;      // true = вкл, false = выкл
  uint8_t days;        // bitmask: bit0=Пн .. bit6=Вс (0x7F = каждый день)
};

// -----------------------------------------------------------------------
//  Структура всех пользовательских настроек устройства
// -----------------------------------------------------------------------
struct Settings {
  // Основные
  String   deviceName;      // имя устройства
  uint8_t  brightness;      // яркость 0–255
  bool     isOn;            // включено / выключено
  uint8_t  currentEffect;   // текущий эффект 0..EFFECT_COUNT-1
  uint32_t color;           // цвет HEX RGB (0xRRGGBB)
  uint8_t  effectSpeed;     // скорость эффекта 0–255
  // Wi-Fi
  String   wifiSSID;        // сохранённая сеть
  String   wifiPassword;    // пароль сети
  // Системные
  int8_t   timezone;        // часовой пояс UTC-12..UTC+14
  uint8_t  powerOnMode;     // 0=last, 1=off, 2=fixed
  // Кнопка
  uint8_t  touchAction1;    // действие: одно касание
  uint8_t  touchAction2;    // действие: двойное касание
  uint8_t  touchAction3;    // действие: удержание
  // Избранное
  FavoriteScene favorites[FAVORITES_COUNT];
  // Расписание
  Schedule schedules[SCHEDULE_COUNT];
};

class SettingsManager {
public:
  Settings data;

  void begin();     // загрузка при старте
  void load();      // чтение из NVS
  void save();      // запись в NVS
  void reset();     // сброс к заводским значениям

  // Избранные сцены (хранятся в отдельном NVS namespace "taxilight_fav")
  void saveFavoriteSlot(uint8_t slot);
  void loadFavoriteSlot(uint8_t slot);
  void deleteFavoriteSlot(uint8_t slot);

  // Расписание (хранится в "taxilight_sch")
  void saveScheduleSlot(uint8_t slot);
  void deleteScheduleSlot(uint8_t slot);

private:
  Preferences prefs;
  void applyDefaults();
  void loadFavorites();
  void loadSchedules();
};

extern SettingsManager Config;   // глобальный экземпляр
