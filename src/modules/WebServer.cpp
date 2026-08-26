// =====================================================================
//  WebServer.cpp — реализация REST API и отдачи UI
// =====================================================================
#include "WebServer.h"
#include <ArduinoJson.h>
#include <WiFi.h>

#include "SettingsManager.h"
#include "LedManager.h"
#include "EffectsManager.h"
#include "BatteryManager.h"
#include "WiFiManager.h"
#include "OtaManager.h"
#include "../web/ui.h"

WebServerManager Web;   // глобальный экземпляр

// ---------------------------------------------------------------------
//  Вспомогательное: накопление тела POST-запроса и разбор JSON
// ---------------------------------------------------------------------
// Собирает тело запроса (возможно, по частям) в request->_tempObject (String*)
// и по завершении вызывает пользовательский обработчик с распарсенным JSON.
typedef std::function<void(AsyncWebServerRequest*, JsonDocument&)> JsonHandler;

static void handleJsonBody(AsyncWebServerRequest* request, uint8_t* data,
                           size_t len, size_t index, size_t total,
                           JsonHandler handler) {
  if (index == 0) {
    request->_tempObject = new String();
    ((String*)request->_tempObject)->reserve(total + 1);
  }
  String* body = (String*)request->_tempObject;
  if (body) {
    for (size_t i = 0; i < len; i++) *body += (char)data[i];
  }
  if (index + len == total) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, *body);
    delete body;
    request->_tempObject = nullptr;
    if (err) {
      request->send(400, "application/json",
                    "{\"ok\":false,\"error\":\"invalid json\"}");
      return;
    }
    handler(request, doc);
  }
}

// ---------------------------------------------------------------------
//  Кольцевой лог событий
// ---------------------------------------------------------------------
void WebServerManager::addLog(const String& line) {
  _log[_logHead] = line;
  _logHead = (_logHead + 1) % STATUS_LOG_SIZE;
  if (_logCount < STATUS_LOG_SIZE) _logCount++;
}

String WebServerManager::buildLogJson() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < _logCount; i++) {
    uint8_t idx = (_logHead + STATUS_LOG_SIZE - _logCount + i) % STATUS_LOG_SIZE;
    arr.add(_log[idx]);
  }
  String out;
  serializeJson(arr, out);
  return out;
}

// ---------------------------------------------------------------------
//  Формирование JSON состояния
// ---------------------------------------------------------------------
String WebServerManager::buildStatusJson() {
  JsonDocument doc;
  doc["on"]         = Config.data.isOn;
  doc["brightness"] = Config.data.brightness;

  char hex[8];
  snprintf(hex, sizeof(hex), "#%06X", Config.data.color & 0xFFFFFF);
  doc["color"]      = hex;

  doc["effect"]     = Config.data.currentEffect;
  doc["speed"]      = Config.data.effectSpeed;
  doc["battery"]    = Battery.getPercent();
  doc["voltage"]    = Battery.getVoltage();
  doc["charging"]   = Battery.isCharging();
  doc["wifiStatus"] = Wifi.getStatus();
  doc["ip"]         = Wifi.getIP();
  doc["rssi"]       = Wifi.getRSSI();
  doc["deviceName"] = Config.data.deviceName;
  doc["version"]    = FIRMWARE_VERSION;
  doc["freeHeap"]   = ESP.getFreeHeap();
  doc["mac"]        = WiFi.macAddress();
  doc["apMode"]     = Wifi.isAP();

  String out;
  serializeJson(doc, out);
  return out;
}

String WebServerManager::buildSettingsJson() {
  JsonDocument doc;
  doc["deviceName"]   = Config.data.deviceName;
  doc["brightness"]   = Config.data.brightness;
  doc["isOn"]         = Config.data.isOn;
  doc["currentEffect"]= Config.data.currentEffect;
  char hex[8];
  snprintf(hex, sizeof(hex), "#%06X", Config.data.color & 0xFFFFFF);
  doc["color"]        = hex;
  doc["effectSpeed"]  = Config.data.effectSpeed;
  doc["wifiSSID"]     = Config.data.wifiSSID;
  doc["timezone"]     = Config.data.timezone;
  doc["powerOnMode"]  = Config.data.powerOnMode;
  doc["touchAction1"] = Config.data.touchAction1;
  doc["touchAction2"] = Config.data.touchAction2;
  doc["touchAction3"] = Config.data.touchAction3;
  String out;
  serializeJson(doc, out);
  return out;
}

// ---------------------------------------------------------------------
//  Маршруты
// ---------------------------------------------------------------------
void WebServerManager::setupRoutes() {

  // --- Главная страница (SPA) ---
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse_P(200, "text/html", INDEX_HTML);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
  });

  // --- Страница разработчика ---
  _server.on("/dev", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse_P(200, "text/html", DEV_HTML);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
  });

  // --- GET /api/status ---
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildStatusJson());
  });

  // --- GET /api/settings ---
  _server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildSettingsJson());
  });

  // --- GET /api/log (для страницы /dev) ---
  _server.on("/api/log", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildLogJson());
  });

  // --- POST /api/power ---
  _server.on("/api/power", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          Config.data.isOn = doc["on"] | Config.data.isOn;
          Config.save();
          applyCurrentState();
          addLog(String("Питание: ") + (Config.data.isOn ? "ВКЛ" : "ВЫКЛ"));
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // --- POST /api/brightness ---
  _server.on("/api/brightness", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int v = doc["value"] | Config.data.brightness;
          if (v < 0) v = 0; if (v > 255) v = 255;
          Config.data.brightness = (uint8_t)v;
          Config.save();
          applyCurrentState();
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // --- POST /api/color ---
  _server.on("/api/color", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          uint8_t rr = doc["r"] | 255;
          uint8_t gg = doc["g"] | 255;
          uint8_t bb = doc["b"] | 255;
          Config.data.color = ((uint32_t)rr << 16) | ((uint32_t)gg << 8) | bb;
          Config.save();
          applyCurrentState();
          addLog("Цвет изменён");
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // --- POST /api/effect ---
  _server.on("/api/effect", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int id = doc["id"] | Config.data.currentEffect;
          if (id < 0 || id >= EFFECT_COUNT) id = 0;
          Config.data.currentEffect = (uint8_t)id;
          if (doc["speed"].is<int>())
            Config.data.effectSpeed = (uint8_t)(int)doc["speed"];
          Config.save();
          applyCurrentState();
          addLog(String("Эффект: ") + id);
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // --- GET /api/wifi/scan ---
  _server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED || n == -2) {
      WiFi.scanNetworks(true);   // асинхронный запуск
    } else if (n >= 0) {
      for (int i = 0; i < n; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"]   = WiFi.SSID(i);
        o["rssi"]   = WiFi.RSSI(i);
        o["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      }
      WiFi.scanDelete();
      WiFi.scanNetworks(true);   // обновление для следующего запроса
    }
    String out;
    serializeJson(arr, out);
    req->send(200, "application/json", out);
  });

  // --- POST /api/wifi/connect ---
  _server.on("/api/wifi/connect", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          String ssid = doc["ssid"] | "";
          String pass = doc["pass"] | "";
          addLog("Подключение к Wi-Fi: " + ssid);
          // Ответ отправляем сразу, само подключение — после
          JsonDocument res;
          res["ok"] = (ssid.length() > 0);
          String out; serializeJson(res, out);
          r->send(200, "application/json", out);
          if (ssid.length() > 0) Wifi.connectToWifi(ssid, pass);
        });
    });

  // --- POST /api/wifi/forget ---
  _server.on("/api/wifi/forget", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("Сеть Wi-Fi забыта");
    req->send(200, "application/json", "{\"ok\":true}");
    Wifi.forget();
  });

  // --- POST /api/settings (частичное обновление) ---
  _server.on("/api/settings", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          if (doc["deviceName"].is<const char*>())
            Config.data.deviceName = String((const char*)doc["deviceName"]);
          if (doc["timezone"].is<int>())
            Config.data.timezone = (int8_t)(int)doc["timezone"];
          if (doc["powerOnMode"].is<int>())
            Config.data.powerOnMode = (uint8_t)(int)doc["powerOnMode"];
          if (doc["touchAction1"].is<int>())
            Config.data.touchAction1 = (uint8_t)(int)doc["touchAction1"];
          if (doc["touchAction2"].is<int>())
            Config.data.touchAction2 = (uint8_t)(int)doc["touchAction2"];
          if (doc["touchAction3"].is<int>())
            Config.data.touchAction3 = (uint8_t)(int)doc["touchAction3"];
          Config.save();
          applyCurrentState();
          addLog("Настройки обновлены");
          r->send(200, "application/json", buildSettingsJson());
        });
    });

  // --- POST /api/settings/reset ---
  _server.on("/api/settings/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("Сброс настроек");
    req->send(200, "application/json", "{\"ok\":true}");
    Config.reset();
    applyCurrentState();
  });

  // --- POST /api/ota/url ---
  _server.on("/api/ota/url", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          String url = doc["url"] | "";
          addLog("OTA по URL: " + url);
          if (url.length() == 0) {
            r->send(400, "application/json", "{\"ok\":false}");
            return;
          }
          r->send(200, "application/json", "{\"ok\":true}");
          Ota.updateFromUrl(url);   // при успехе устройство перезагрузится
        });
    });

  // --- POST /api/reboot ---
  _server.on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("Перезагрузка устройства");
    req->send(200, "application/json", "{\"ok\":true}");
    delay(200);
    ESP.restart();
  });

  // --- POST /api/test/led (тест ленты — радуга) ---
  _server.on("/api/test/led", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("Тест LED: радуга 5с");
    Config.data.isOn = true;
    Config.data.currentEffect = EFFECT_RAINBOW;
    applyCurrentState();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  // --- Captive portal: неизвестные маршруты -> главная ---
  _server.onNotFound([](AsyncWebServerRequest* req) {
    if (Wifi.isAP()) {
      // редирект на корень для captive portal
      AsyncWebServerResponse* resp =
        req->beginResponse_P(200, "text/html", INDEX_HTML);
      req->send(resp);
    } else {
      req->send(404, "text/plain", "Not found");
    }
  });
}

void WebServerManager::begin() {
  setupRoutes();
  _server.begin();
  addLog("Веб-сервер запущен");
  Serial.println(F("[Web] HTTP-сервер запущен на порту 80"));
}
