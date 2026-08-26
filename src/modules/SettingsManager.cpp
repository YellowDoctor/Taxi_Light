// =====================================================================
//  SettingsManager.cpp — реализация хранения настроек в NVS
// =====================================================================
#include "SettingsManager.h"

SettingsManager Config;   // глобальный экземпляр

static const char* NVS_NAMESPACE = "taxilight";

void SettingsManager::applyDefaults() {
  data.deviceName    = DEVICE_DEFAULT_NAME;
  data.brightness    = 150;
  data.isOn          = true;
  data.currentEffect = EFFECT_STATIC;
  data.color         = 0xFFB000;   // тёплый янтарный по умолчанию
  data.effectSpeed   = 128;
  data.wifiSSID      = "";
  data.wifiPassword  = "";
  data.timezone      = 3;          // UTC+3 (Киев/Москва)
  data.powerOnMode   = 0;          // last state
  data.touchAction1  = 0;          // вкл/выкл
  data.touchAction2  = 1;          // следующий эффект
  data.touchAction3  = 2;          // регулировка яркости
}

void SettingsManager::begin() {
  load();
}

void SettingsManager::load() {
  applyDefaults();   // сначала значения по умолчанию
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    // раздел не существует — оставляем значения по умолчанию
    Serial.println(F("[Settings] NVS пуст, применены значения по умолчанию"));
    return;
  }

  data.deviceName    = prefs.getString("name", data.deviceName);
  data.brightness    = prefs.getUChar("bright", data.brightness);
  data.isOn          = prefs.getBool("on", data.isOn);
  data.currentEffect = prefs.getUChar("effect", data.currentEffect);
  data.color         = prefs.getUInt("color", data.color);
  data.effectSpeed   = prefs.getUChar("speed", data.effectSpeed);
  data.wifiSSID      = prefs.getString("ssid", data.wifiSSID);
  data.wifiPassword  = prefs.getString("pass", data.wifiPassword);
  data.timezone      = prefs.getChar("tz", data.timezone);
  data.powerOnMode   = prefs.getUChar("pom", data.powerOnMode);
  data.touchAction1  = prefs.getUChar("ta1", data.touchAction1);
  data.touchAction2  = prefs.getUChar("ta2", data.touchAction2);
  data.touchAction3  = prefs.getUChar("ta3", data.touchAction3);
  prefs.end();

  // Применяем режим включения
  if (data.powerOnMode == 1)      data.isOn = false;   // всегда выкл
  else if (data.powerOnMode == 2) data.isOn = true;    // всегда вкл

  // Валидация диапазонов
  if (data.currentEffect >= EFFECT_COUNT) data.currentEffect = EFFECT_STATIC;

  Serial.println(F("[Settings] Настройки загружены из NVS"));
}

void SettingsManager::save() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println(F("[Settings] ОШИБКА: не удалось открыть NVS для записи"));
    return;
  }
  prefs.putString("name", data.deviceName);
  prefs.putUChar("bright", data.brightness);
  prefs.putBool("on", data.isOn);
  prefs.putUChar("effect", data.currentEffect);
  prefs.putUInt("color", data.color);
  prefs.putUChar("speed", data.effectSpeed);
  prefs.putString("ssid", data.wifiSSID);
  prefs.putString("pass", data.wifiPassword);
  prefs.putChar("tz", data.timezone);
  prefs.putUChar("pom", data.powerOnMode);
  prefs.putUChar("ta1", data.touchAction1);
  prefs.putUChar("ta2", data.touchAction2);
  prefs.putUChar("ta3", data.touchAction3);
  prefs.end();
  Serial.println(F("[Settings] Настройки сохранены в NVS"));
}

void SettingsManager::reset() {
  if (prefs.begin(NVS_NAMESPACE, false)) {
    prefs.clear();
    prefs.end();
  }
  applyDefaults();
  save();
  Serial.println(F("[Settings] Настройки сброшены к заводским"));
}
