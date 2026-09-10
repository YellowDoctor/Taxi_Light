// =====================================================================
//  SettingsManager.cpp — реализация хранения настроек в NVS
// =====================================================================
#include "SettingsManager.h"

SettingsManager Config;   // глобальный экземпляр

static const char* NVS_NAMESPACE     = "taxilight";
static const char* NVS_FAV_NAMESPACE = "taxilight_fav";
static const char* NVS_SCH_NAMESPACE = "taxilight_sch";

// -----------------------------------------------------------------------
void SettingsManager::applyDefaults() {
  data.deviceName    = DEVICE_DEFAULT_NAME;
  data.brightness    = 150;
  data.isOn          = true;
  data.currentEffect = EFFECT_STATIC;
  data.color         = 0xFFB000;   // тёплый янтарный по умолчанию
  data.effectSpeed   = 128;
  data.wifiSSID      = "";
  data.wifiPassword  = "";
  data.timezone      = 3;          // UTC+3
  data.powerOnMode   = 0;          // last state
  data.touchAction1  = 0;          // вкл/выкл
  data.touchAction2  = 1;          // следующий эффект
  data.touchAction3  = 2;          // регулировка яркости

  // Избранное — все слоты пусты
  for (uint8_t i = 0; i < FAVORITES_COUNT; i++) {
    data.favorites[i].used       = false;
    data.favorites[i].name[0]    = '\0';
    data.favorites[i].color      = 0xFFB000;
    data.favorites[i].effect     = EFFECT_STATIC;
    data.favorites[i].brightness = 150;
    data.favorites[i].speed      = 128;
  }

  // Расписание — все записи отключены
  for (uint8_t i = 0; i < SCHEDULE_COUNT; i++) {
    data.schedules[i].used    = false;
    data.schedules[i].enabled = false;
    data.schedules[i].hour    = 0;
    data.schedules[i].minute  = 0;
    data.schedules[i].action  = false;
    data.schedules[i].days    = 0x7F;  // все дни
  }
}

// -----------------------------------------------------------------------
void SettingsManager::begin() {
  load();
}

// -----------------------------------------------------------------------
void SettingsManager::load() {
  applyDefaults();
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    Serial.println(F("[Settings] NVS пуст, применены значения по умолчанию"));
    return;
  }

  data.deviceName    = prefs.getString("name",   data.deviceName);
  data.brightness    = prefs.getUChar("bright",  data.brightness);
  data.isOn          = prefs.getBool("on",       data.isOn);
  data.currentEffect = prefs.getUChar("effect",  data.currentEffect);
  data.color         = prefs.getUInt("color",    data.color);
  data.effectSpeed   = prefs.getUChar("speed",   data.effectSpeed);
  data.wifiSSID      = prefs.getString("ssid",   data.wifiSSID);
  data.wifiPassword  = prefs.getString("pass",   data.wifiPassword);
  data.timezone      = prefs.getChar("tz",       data.timezone);
  data.powerOnMode   = prefs.getUChar("pom",     data.powerOnMode);
  data.touchAction1  = prefs.getUChar("ta1",     data.touchAction1);
  data.touchAction2  = prefs.getUChar("ta2",     data.touchAction2);
  data.touchAction3  = prefs.getUChar("ta3",     data.touchAction3);
  prefs.end();

  // Применяем режим включения
  if (data.powerOnMode == 1)      data.isOn = false;
  else if (data.powerOnMode == 2) data.isOn = true;

  // Валидация
  if (data.currentEffect >= EFFECT_COUNT) data.currentEffect = EFFECT_STATIC;

  // Загружаем избранное и расписание из отдельных namespace'ов
  loadFavorites();
  loadSchedules();

  Serial.println(F("[Settings] Настройки загружены из NVS"));
}

// -----------------------------------------------------------------------
void SettingsManager::save() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println(F("[Settings] ОШИБКА: не удалось открыть NVS для записи"));
    return;
  }
  prefs.putString("name",   data.deviceName);
  prefs.putUChar("bright",  data.brightness);
  prefs.putBool("on",       data.isOn);
  prefs.putUChar("effect",  data.currentEffect);
  prefs.putUInt("color",    data.color);
  prefs.putUChar("speed",   data.effectSpeed);
  prefs.putString("ssid",   data.wifiSSID);
  prefs.putString("pass",   data.wifiPassword);
  prefs.putChar("tz",       data.timezone);
  prefs.putUChar("pom",     data.powerOnMode);
  prefs.putUChar("ta1",     data.touchAction1);
  prefs.putUChar("ta2",     data.touchAction2);
  prefs.putUChar("ta3",     data.touchAction3);
  prefs.end();
  Serial.println(F("[Settings] Настройки сохранены в NVS"));
}

// -----------------------------------------------------------------------
void SettingsManager::reset() {
  if (prefs.begin(NVS_NAMESPACE, false)) { prefs.clear(); prefs.end(); }
  if (prefs.begin(NVS_FAV_NAMESPACE, false)) { prefs.clear(); prefs.end(); }
  if (prefs.begin(NVS_SCH_NAMESPACE, false)) { prefs.clear(); prefs.end(); }
  applyDefaults();
  save();
  Serial.println(F("[Settings] Настройки сброшены к заводским"));
}

// =====================================================================
//  Избранные сцены
// =====================================================================

// Ключ в NVS для слота: "f0_used", "f0_name", "f0_col", ...
static String favKey(uint8_t slot, const char* field) {
  return String("f") + slot + "_" + field;
}

void SettingsManager::loadFavorites() {
  if (!prefs.begin(NVS_FAV_NAMESPACE, true)) return;
  for (uint8_t i = 0; i < FAVORITES_COUNT; i++) {
    data.favorites[i].used = prefs.getBool(favKey(i, "used").c_str(), false);
    if (data.favorites[i].used) {
      String n = prefs.getString(favKey(i, "name").c_str(), "");
      strncpy(data.favorites[i].name, n.c_str(), FAVORITES_NAME_LEN - 1);
      data.favorites[i].name[FAVORITES_NAME_LEN - 1] = '\0';
      data.favorites[i].color      = prefs.getUInt(favKey(i, "col").c_str(),  0xFFB000);
      data.favorites[i].effect     = prefs.getUChar(favKey(i, "eff").c_str(), EFFECT_STATIC);
      data.favorites[i].brightness = prefs.getUChar(favKey(i, "bri").c_str(), 150);
      data.favorites[i].speed      = prefs.getUChar(favKey(i, "spd").c_str(), 128);
    }
  }
  prefs.end();
}

void SettingsManager::saveFavoriteSlot(uint8_t slot) {
  if (slot >= FAVORITES_COUNT) return;
  if (!prefs.begin(NVS_FAV_NAMESPACE, false)) return;
  FavoriteScene& f = data.favorites[slot];
  prefs.putBool(favKey(slot, "used").c_str(),  f.used);
  prefs.putString(favKey(slot, "name").c_str(), f.name);
  prefs.putUInt(favKey(slot, "col").c_str(),   f.color);
  prefs.putUChar(favKey(slot, "eff").c_str(),  f.effect);
  prefs.putUChar(favKey(slot, "bri").c_str(),  f.brightness);
  prefs.putUChar(favKey(slot, "spd").c_str(),  f.speed);
  prefs.end();
}

void SettingsManager::loadFavoriteSlot(uint8_t slot) {
  // Данные уже в памяти (loadFavorites при старте)
  (void)slot;
}

void SettingsManager::deleteFavoriteSlot(uint8_t slot) {
  if (slot >= FAVORITES_COUNT) return;
  data.favorites[slot].used    = false;
  data.favorites[slot].name[0] = '\0';
  saveFavoriteSlot(slot);
}

// =====================================================================
//  Расписание
// =====================================================================

static String schKey(uint8_t slot, const char* field) {
  return String("s") + slot + "_" + field;
}

void SettingsManager::loadSchedules() {
  if (!prefs.begin(NVS_SCH_NAMESPACE, true)) return;
  for (uint8_t i = 0; i < SCHEDULE_COUNT; i++) {
    bool en = prefs.getBool(schKey(i, "en").c_str(), false);
    data.schedules[i].used    = prefs.getBool(schKey(i, "u").c_str(), en);
    data.schedules[i].enabled = en;
    data.schedules[i].hour    = prefs.getUChar(schKey(i, "h").c_str(),   0);
    data.schedules[i].minute  = prefs.getUChar(schKey(i, "m").c_str(),   0);
    data.schedules[i].action  = prefs.getBool(schKey(i, "act").c_str(),  false);
    data.schedules[i].days    = prefs.getUChar(schKey(i, "days").c_str(), 0x7F);
  }
  prefs.end();
}

void SettingsManager::saveScheduleSlot(uint8_t slot) {
  if (slot >= SCHEDULE_COUNT) return;
  if (!prefs.begin(NVS_SCH_NAMESPACE, false)) return;
  Schedule& s = data.schedules[slot];
  prefs.putBool(schKey(slot, "u").c_str(),     s.used);
  prefs.putBool(schKey(slot, "en").c_str(),    s.enabled);
  prefs.putUChar(schKey(slot, "h").c_str(),    s.hour);
  prefs.putUChar(schKey(slot, "m").c_str(),    s.minute);
  prefs.putBool(schKey(slot, "act").c_str(),   s.action);
  prefs.putUChar(schKey(slot, "days").c_str(), s.days);
  prefs.end();
}

void SettingsManager::deleteScheduleSlot(uint8_t slot) {
  if (slot >= SCHEDULE_COUNT) return;
  data.schedules[slot].used    = false;
  data.schedules[slot].enabled = false;
  saveScheduleSlot(slot);
}


