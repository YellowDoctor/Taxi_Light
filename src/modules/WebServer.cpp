// =====================================================================
//  WebServer.cpp — REST API + WebSocket + UI
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
#include <Update.h>
#include "../web/ui.h"


WebServerManager Web;   // глобальный экземпляр

// ---------------------------------------------------------------------
//  Вспомогательное: накопление тела POST-запроса и разбор JSON
// ---------------------------------------------------------------------
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
//  WebSocket: push-обновление всем клиентам
// ---------------------------------------------------------------------
void WebServerManager::notifyClients() {
  if (_ws.count() > 0) {
    _ws.textAll(buildStatusJson());
  }
}

// ---------------------------------------------------------------------
//  Таймер сна
// ---------------------------------------------------------------------
void WebServerManager::setSleepTimer(uint32_t minutes) {
  if (minutes == 0) { cancelSleepTimer(); return; }
  _sleepActive = true;
  _sleepEnd    = millis() + minutes * 60UL * 1000UL;
  Serial.printf("[Web] Таймер сна: %u мин\n", minutes);
}

void WebServerManager::cancelSleepTimer() {
  _sleepActive = false;
  _sleepEnd    = 0;
  Serial.println(F("[Web] Таймер сна отменён"));
}

int32_t WebServerManager::sleepTimerLeft() const {
  if (!_sleepActive) return -1;
  uint32_t now = millis();
  if (now >= _sleepEnd) return 0;
  return (int32_t)((_sleepEnd - now) / 1000UL);
}

void WebServerManager::tickSleepTimer() {
  if (!_sleepActive) return;
  if (millis() >= _sleepEnd) {
    _sleepActive = false;
    Config.data.isOn = false;
    applyCurrentState();
    Config.save();
    addLog("Таймер сна: выключение");
    notifyClients();
    Serial.println(F("[Web] Таймер сна: устройство выключено"));
  }
}

// ---------------------------------------------------------------------
//  Формирование JSON
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
  doc["timeSynced"] = Wifi.isTimeSynced();
  if (Wifi.isTimeSynced()) {
    struct tm t;
    if (getLocalTime(&t, 0)) {
      char tbuf[8];
      snprintf(tbuf, sizeof(tbuf), "%02d:%02d", t.tm_hour, t.tm_min);
      doc["time"] = tbuf;
    }
  }
  doc["timerLeft"]     = sleepTimerLeft();
  doc["hasUpdate"]     = Ota.hasUpdate();
  doc["latestVersion"] = Ota.getLatestVersion();
  doc["latestUrl"]     = Ota.getLatestUrl();
  doc["isUpdating"]    = Ota.isUpdating();
  doc["progress"]      = Ota.getProgress();

  String out;

  serializeJson(doc, out);
  return out;
}

String WebServerManager::buildSettingsJson() {
  JsonDocument doc;
  doc["deviceName"]    = Config.data.deviceName;
  doc["brightness"]    = Config.data.brightness;
  doc["isOn"]          = Config.data.isOn;
  doc["currentEffect"] = Config.data.currentEffect;
  char hex[8];
  snprintf(hex, sizeof(hex), "#%06X", Config.data.color & 0xFFFFFF);
  doc["color"]         = hex;
  doc["effectSpeed"]   = Config.data.effectSpeed;
  doc["wifiSSID"]      = Config.data.wifiSSID;
  doc["timezone"]      = Config.data.timezone;
  doc["powerOnMode"]   = Config.data.powerOnMode;
  doc["touchAction1"]  = Config.data.touchAction1;
  doc["touchAction2"]  = Config.data.touchAction2;
  doc["touchAction3"]  = Config.data.touchAction3;
  String out;
  serializeJson(doc, out);
  return out;
}

String WebServerManager::buildFavoritesJson() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < FAVORITES_COUNT; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["slot"]   = i;
    o["used"]   = Config.data.favorites[i].used;
    if (Config.data.favorites[i].used) {
      o["name"]       = Config.data.favorites[i].name;
      char hex[8];
      snprintf(hex, sizeof(hex), "#%06X", Config.data.favorites[i].color & 0xFFFFFF);
      o["color"]      = hex;
      o["effect"]     = Config.data.favorites[i].effect;
      o["brightness"] = Config.data.favorites[i].brightness;
      o["speed"]      = Config.data.favorites[i].speed;
    }
  }
  String out;
  serializeJson(arr, out);
  return out;
}

String WebServerManager::buildSchedulesJson() {
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (uint8_t i = 0; i < SCHEDULE_COUNT; i++) {
    JsonObject o = arr.add<JsonObject>();
    o["slot"]    = i;
    o["used"]    = Config.data.schedules[i].used;
    o["enabled"] = Config.data.schedules[i].enabled;
    o["hour"]    = Config.data.schedules[i].hour;
    o["minute"]  = Config.data.schedules[i].minute;
    o["action"]  = Config.data.schedules[i].action;
    o["days"]    = Config.data.schedules[i].days;
  }
  String out;
  serializeJson(arr, out);
  return out;
}

// ---------------------------------------------------------------------
//  Маршруты
// ---------------------------------------------------------------------
void WebServerManager::setupRoutes() {

  // --- WebSocket ---
  _ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type,
                 void*, uint8_t*, size_t) {
    // При подключении нового клиента он сразу получит статус
    if (type == WS_EVT_CONNECT) Web.notifyClients();
  });
  _server.addHandler(&_ws);

  // --- Главная страница (SPA) ---
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "text/html", (const uint8_t*)INDEX_HTML, sizeof(INDEX_HTML) - 1);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
  });

  // --- Страница разработчика ---
  _server.on("/dev", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "text/html", (const uint8_t*)DEV_HTML, sizeof(DEV_HTML) - 1);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
  });

  // --- PWA Manifest ---
  _server.on("/manifest.json", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "application/manifest+json", (const uint8_t*)MANIFEST_JSON, sizeof(MANIFEST_JSON) - 1);
    resp->addHeader("Cache-Control", "public, max-age=86400");
    req->send(resp);
  });

  // --- PWA Icon (SVG с адаптивной темой) ---
  _server.on("/icon.svg", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "image/svg+xml", (const uint8_t*)ICON_SVG, sizeof(ICON_SVG) - 1);
    resp->addHeader("Cache-Control", "public, max-age=86400");
    req->send(resp);
  });

  // --- PNG Icon (растровый для Android Chrome PWA) ---
  _server.on("/icon.png", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "image/png", ICON_PNG, ICON_PNG_LEN);
    resp->addHeader("Cache-Control", "public, max-age=86400");
    req->send(resp);
  });

  // --- Service Worker (требуется Chrome для standalone WebAPK) ---
  _server.on("/sw.js", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "application/javascript", (const uint8_t*)SW_JS, sizeof(SW_JS) - 1);
    resp->addHeader("Cache-Control", "no-cache");
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

  // --- GET /api/log ---
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
          notifyClients();
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
          notifyClients();
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
          notifyClients();
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
          notifyClients();
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // --- GET /api/wifi/scan ---
  _server.on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED || n == -2) {
      WiFi.scanNetworks(true);
    } else if (n >= 0) {
      for (int i = 0; i < n; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"]   = WiFi.SSID(i);
        o["rssi"]   = WiFi.RSSI(i);
        o["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      }
      WiFi.scanDelete();
      WiFi.scanNetworks(true);
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
          // Отвечаем сразу, само подключение — асинхронно
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

  // --- POST /api/ota/upload (загрузка .bin файла из браузера) ---
  _server.on("/api/ota/upload", HTTP_POST,
    [this](AsyncWebServerRequest* req) {
      bool success = !Update.hasError();
      AsyncWebServerResponse* resp = req->beginResponse(
        200, "application/json",
        success ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"Ошибка прошивки\"}"
      );
      resp->addHeader("Connection", "close");
      req->send(resp);
      if (success) {
        addLog("Прошивка из файла завершена успешно");
        Ota.onUpdateSuccess();
      } else {
        addLog("Ошибка прошивки из файла");
        Ota.onUpdateError();
      }
    },
    [this](AsyncWebServerRequest* req, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
      if (!index) {
        addLog("Старт загрузки файла прошивки: " + filename);
        Serial.printf("[OTA] Загрузка файла: %s\n", filename.c_str());
        Ota.onUpdateStart();
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          Update.printError(Serial);
        }
      }
      if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }
      if (final) {
        if (Update.end(true)) {
          Serial.printf("[OTA] Файл успешно получен, размер: %u байт\n", index + len);
        } else {
          Update.printError(Serial);
        }
      }
    });

  // --- GET /api/ota/status ---
  _server.on("/api/ota/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["currentVersion"] = FIRMWARE_VERSION;
    doc["hasUpdate"]      = Ota.hasUpdate();
    doc["latestVersion"]  = Ota.getLatestVersion();
    doc["latestUrl"]      = Ota.getLatestUrl();
    doc["updateNotes"]    = Ota.getUpdateNotes();
    doc["isUpdating"]     = Ota.isUpdating();
    doc["progress"]       = Ota.getProgress();
    String out;
    serializeJson(doc, out);
    req->send(200, "application/json", out);
  });

  // --- POST /api/ota/check (принудительная проверка GitHub) ---
  _server.on("/api/ota/check", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("Запрос проверки обновлений на GitHub");
    Ota.checkGitHubUpdate();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  // --- POST /api/ota/github (обновление до версии с GitHub) ---
  _server.on("/api/ota/github", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!Ota.hasUpdate() || Ota.getLatestUrl().length() == 0) {
      req->send(400, "application/json", "{\"ok\":false,\"error\":\"Нет доступных обновлений\"}");
      return;
    }
    addLog("Запуск обновления с GitHub: " + Ota.getLatestVersion());
    req->send(200, "application/json", "{\"ok\":true}");
    Ota.updateFromUrl(Ota.getLatestUrl());
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
          if (url.length() == 0) {
            r->send(400, "application/json", "{\"ok\":false,\"error\":\"Пустой URL\"}");
            return;
          }
          addLog("OTA по URL: " + url);
          r->send(200, "application/json", "{\"ok\":true}");
          Ota.updateFromUrl(url);
        });
    });

  // --- POST /api/reboot (асинхронно, через флаг) ---
  _server.on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("Перезагрузка устройства");
    req->send(200, "application/json", "{\"ok\":true}");
    _pendingReboot = true;   // реальный restart — в loop()
  });

  // --- POST /api/test/led ---
  _server.on("/api/test/led", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("Тест LED: радуга 5с");
    Config.data.isOn = true;
    Config.data.currentEffect = EFFECT_RAINBOW;
    applyCurrentState();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  // ===================================================================
  //  ИЗБРАННЫЕ СЦЕНЫ (Favorites)
  // ===================================================================

  // --- GET /api/favorites ---
  _server.on("/api/favorites", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildFavoritesJson());
  });

  // --- POST /api/favorites/save ---
  _server.on("/api/favorites/save", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int slot = doc["slot"] | -1;
          if (slot < 0 || slot >= FAVORITES_COUNT) {
            r->send(400, "application/json", "{\"ok\":false,\"error\":\"bad slot\"}");
            return;
          }
          FavoriteScene& f = Config.data.favorites[slot];
          f.used       = true;
          f.color      = Config.data.color;
          f.effect     = Config.data.currentEffect;
          f.brightness = Config.data.brightness;
          f.speed      = Config.data.effectSpeed;
          // Имя
          const char* name = doc["name"] | "";
          strncpy(f.name, (name[0] ? name : (String("Сцена ") + (slot+1)).c_str()),
                  FAVORITES_NAME_LEN - 1);
          f.name[FAVORITES_NAME_LEN - 1] = '\0';
          Config.saveFavoriteSlot(slot);
          addLog(String("Сохранено в слот ") + slot);
          r->send(200, "application/json", buildFavoritesJson());
        });
    });

  // --- POST /api/favorites/load ---
  _server.on("/api/favorites/load", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int slot = doc["slot"] | -1;
          if (slot < 0 || slot >= FAVORITES_COUNT || !Config.data.favorites[slot].used) {
            r->send(400, "application/json", "{\"ok\":false,\"error\":\"bad slot\"}");
            return;
          }
          const FavoriteScene& f = Config.data.favorites[slot];
          Config.data.color          = f.color;
          Config.data.currentEffect  = f.effect;
          Config.data.brightness     = f.brightness;
          Config.data.effectSpeed    = f.speed;
          Config.data.isOn           = true;
          Config.save();
          applyCurrentState();
          addLog(String("Загружена сцена: ") + f.name);
          notifyClients();
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // --- POST /api/favorites/delete ---
  _server.on("/api/favorites/delete", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int slot = doc["slot"] | -1;
          if (slot < 0 || slot >= FAVORITES_COUNT) {
            r->send(400, "application/json", "{\"ok\":false,\"error\":\"bad slot\"}");
            return;
          }
          Config.deleteFavoriteSlot(slot);
          addLog(String("Слот ") + slot + " удалён");
          r->send(200, "application/json", buildFavoritesJson());
        });
    });

  // ===================================================================
  //  ТАЙМЕР СНА
  // ===================================================================

  // --- POST /api/timer ---
  _server.on("/api/timer", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int minutes = doc["minutes"] | 0;
          setSleepTimer((uint32_t)max(0, minutes));
          if (minutes > 0)
            addLog(String("Таймер сна: ") + minutes + " мин");
          else
            addLog("Таймер сна отменён");
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // ===================================================================
  //  РАСПИСАНИЕ
  // ===================================================================


  // --- GET /api/schedules ---
  _server.on("/api/schedules", HTTP_GET, [this](AsyncWebServerRequest* req) {
    req->send(200, "application/json", buildSchedulesJson());
  });

  // --- POST /api/schedules ---
  _server.on("/api/schedules", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int slot = doc["slot"] | -1;
          if (slot < 0 || slot >= SCHEDULE_COUNT) {
            r->send(400, "application/json", "{\"ok\":false,\"error\":\"bad slot\"}");
            return;
          }
          Schedule& s    = Config.data.schedules[slot];
          if (!doc["used"].isNull())    s.used    = doc["used"].as<bool>();
          if (!doc["enabled"].isNull()) s.enabled = doc["enabled"].as<bool>();
          if (!doc["hour"].isNull())    s.hour    = (uint8_t)((int)doc["hour"]);
          if (!doc["minute"].isNull())  s.minute  = (uint8_t)((int)doc["minute"]);
          if (!doc["action"].isNull())  s.action  = doc["action"].as<bool>();
          if (!doc["days"].isNull())    s.days    = (uint8_t)((int)doc["days"]);
          Config.saveScheduleSlot(slot);
          addLog(String("Расписание слот ") + slot + " обновлено");
          r->send(200, "application/json", buildSchedulesJson());
        });
    });

  // --- POST /api/schedules/delete ---
  _server.on("/api/schedules/delete", HTTP_POST,
    [](AsyncWebServerRequest* req) {},
    NULL,
    [this](AsyncWebServerRequest* req, uint8_t* data, size_t len,
           size_t index, size_t total) {
      handleJsonBody(req, data, len, index, total,
        [this](AsyncWebServerRequest* r, JsonDocument& doc) {
          int slot = doc["slot"] | -1;
          if (slot < 0 || slot >= SCHEDULE_COUNT) {
            r->send(400, "application/json", "{\"ok\":false,\"error\":\"bad slot\"}");
            return;
          }
          Config.deleteScheduleSlot(slot);
          addLog(String("Расписание слот ") + slot + " удалён");
          r->send(200, "application/json", buildSchedulesJson());
        });
    });

  // --- Captive portal: неизвестные маршруты -> главная ---
  _server.onNotFound([](AsyncWebServerRequest* req) {
    if (Wifi.isAP()) {
      AsyncWebServerResponse* resp =
        req->beginResponse(200, "text/html", (const uint8_t*)INDEX_HTML, sizeof(INDEX_HTML) - 1);
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
  Serial.println(F("[Web] HTTP/WebSocket сервер запущен на порту 80"));
}

