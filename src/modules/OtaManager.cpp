// =====================================================================
//  OtaManager.cpp — реализация OTA-обновлений
// =====================================================================
#include "OtaManager.h"
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFi.h>

OtaManager Ota;   // глобальный экземпляр

void OtaManager::begin() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([]() {
    Serial.println(F("[OTA] Начало обновления"));
  });
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\n[OTA] Обновление завершено"));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Прогресс: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Ошибка [%u]\n", error);
  });

  ArduinoOTA.begin();
  _started = true;
  Serial.println(F("[OTA] ArduinoOTA готов"));
}

void OtaManager::handle() {
  if (_started) ArduinoOTA.handle();
}

bool OtaManager::updateFromUrl(const String& url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[OTA] Нет подключения к сети для OTA по URL"));
    return false;
  }
  Serial.printf("[OTA] Загрузка прошивки: %s\n", url.c_str());

  WiFiClient client;
  httpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret = httpUpdate.update(client, url);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA] Ошибка: %s\n", httpUpdate.getLastErrorString().c_str());
      return false;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("[OTA] Нет обновлений"));
      return false;
    case HTTP_UPDATE_OK:
      Serial.println(F("[OTA] Успешно (перезагрузка)"));
      return true;
  }
  return false;
}
