// =====================================================================
//  OtaManager.cpp — реализация OTA-обновлений (URL, GitHub, ArduinoOTA)
// =====================================================================
#include "OtaManager.h"
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "LedManager.h"
#include "EffectsManager.h"
#include "SettingsManager.h"

OtaManager Ota;   // глобальный экземпляр

extern void applyCurrentState();

static bool isNewerVersion(const String& remote, const String& current) {
  int rMajor = 0, rMinor = 0, rPatch = 0;
  int cMajor = 0, cMinor = 0, cPatch = 0;
  sscanf(remote.c_str(), "%d.%d.%d", &rMajor, &rMinor, &rPatch);
  sscanf(current.c_str(), "%d.%d.%d", &cMajor, &cMinor, &cPatch);
  if (rMajor > cMajor) return true;
  if (rMajor < cMajor) return false;
  if (rMinor > cMinor) return true;
  if (rMinor < cMinor) return false;
  return rPatch > cPatch;
}

void OtaManager::begin() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.println(F("[OTA] Начало обновления через ArduinoOTA"));
    Ota.onUpdateStart();
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\n[OTA] Обновление завершено"));
    Ota.onUpdateSuccess();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    if (total > 0) {
      int pct = (progress * 100) / total;
      Ota.setProgress(pct);
      Serial.printf("[OTA] Прогресс: %u%%\r", pct);
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Ошибка [%u]\n", error);
    Ota.onUpdateError();
  });

  ArduinoOTA.begin();
  _started = true;
  Serial.println(F("[OTA] ArduinoOTA готов"));
}

void OtaManager::handle() {
  if (_started && !_updating) ArduinoOTA.handle();
}

void OtaManager::onUpdateStart() {
  _updating = true;
  _progress = 0;
  Effects.pause();
  Led.showOtaProgress();
  Serial.println(F("[OTA] Индикация: зелёный цвет 50% яркости"));
}

void OtaManager::onUpdateSuccess() {
  _progress = 100;
  Serial.println(F("[OTA] Успешно! 3 мигания зелёным и перезагрузка"));
  Led.showOtaSuccess();
  delay(300);
  ESP.restart();
}

void OtaManager::onUpdateError() {
  Serial.println(F("[OTA] Ошибка! 3 мигания красным"));
  Led.showOtaError();
  _updating = false;
  _progress = 0;
  Effects.resume();
  applyCurrentState();
}

bool OtaManager::updateFromUrl(const String& url) {
  if (WiFi.status() != WL_CONNECTED || _updating) {
    Serial.println(F("[OTA] Нет подключения к сети или обновление уже идёт"));
    return false;
  }

  // Запуск загрузки прошивки в отдельной FreeRTOS-задаче со стеком 16КБ,
  // чтобы TLS handshake и долгая загрузка через Интернет не блокировали async_tcp
  // и не сбивали сторожевой таймер Task Watchdog (5 сек)!
  String* pUrl = new String(url);
  BaseType_t res = xTaskCreate([](void* param) {
    String* urlPtr = (String*)param;
    Ota._doUpdateFromUrl(*urlPtr);
    delete urlPtr;
    vTaskDelete(NULL);
  }, "otaUpdateTask", 16384, pUrl, 2, NULL);

  if (res != pdPASS) {
    delete pUrl;
    Serial.println(F("[OTA] Ошибка создания задачи обновления"));
    return false;
  }
  return true;
}

void OtaManager::_doUpdateFromUrl(const String& url) {
  Serial.printf("[OTA] Загрузка прошивки в фоновой задаче: %s\n", url.c_str());

  onUpdateStart();
  httpUpdate.rebootOnUpdate(false);
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  httpUpdate.onProgress([](int cur, int total) {
    if (total > 0) {
      int pct = (cur * 100) / total;
      if (pct > 100) pct = 100;
      Ota.setProgress(pct);
      static int lastLogged = -1;
      if (pct != lastLogged && pct % 10 == 0) {
        lastLogged = pct;
        Serial.printf("[OTA] HTTP прогресс: %d%%\n", pct);
      }
    }
  });

  t_httpUpdate_return ret;

  if (url.startsWith("https://")) {
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    secureClient.setTimeout(20000);
    ret = httpUpdate.update(secureClient, url);
  } else {
    WiFiClient client;
    client.setTimeout(20000);
    ret = httpUpdate.update(client, url);
  }

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA] Ошибка: %s\n", httpUpdate.getLastErrorString().c_str());
      onUpdateError();
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("[OTA] Нет обновлений"));
      onUpdateError();
      break;
    case HTTP_UPDATE_OK:
      onUpdateSuccess();
      break;
  }
}

void OtaManager::checkGitHubUpdate() {
  if (WiFi.status() != WL_CONNECTED || _updating) return;
  static bool isChecking = false;
  if (isChecking) return;
  isChecking = true;

  // Запуск проверки в отдельной задаче FreeRTOS (стек 12КБ для TLS/mbedtls)
  xTaskCreate([](void* param) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    OtaManager* mgr = (OtaManager*)param;
    mgr->_doCheckGitHub();
    isChecking = false;
    vTaskDelete(NULL);
  }, "otaCheckTask", 12288, this, 1, NULL);
}

void OtaManager::_doCheckGitHub() {
  Serial.println(F("[OTA] Проверка обновлений на GitHub..."));
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, GITHUB_VERSION_URL);
  http.setTimeout(8000);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      String ver   = doc["version"] | "";
      String url   = doc["url"]     | "";
      String notes = doc["notes"]   | "";

      if (ver.length() > 0 && isNewerVersion(ver, FIRMWARE_VERSION)) {
        _hasUpdate     = true;
        _latestVersion = ver;
        _latestUrl     = url;
        _updateNotes   = notes;
        Serial.printf("[OTA] Доступна новая версия: %s (текущая %s)\n",
                      ver.c_str(), FIRMWARE_VERSION);
      } else {
        _hasUpdate     = false;
        _latestVersion = ver;
        Serial.printf("[OTA] Прошивка актуальна: %s\n", FIRMWARE_VERSION);
      }
    } else {
      Serial.println(F("[OTA] Ошибка разбора version.json с GitHub"));
    }
  } else {
    Serial.printf("[OTA] GitHub запрос завершился с кодом: %d\n", httpCode);
  }
  http.end();
}

