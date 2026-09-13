// =====================================================================
//  WebServer.cpp тАФ REST API + WebSocket + UI
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


WebServerManager Web;   // ╨│╨╗╨╛╨▒╨░╨╗╤М╨╜╤Л╨╣ ╤Н╨║╨╖╨╡╨╝╨┐╨╗╤П╤А

// ---------------------------------------------------------------------
//  ╨Т╤Б╨┐╨╛╨╝╨╛╨│╨░╤В╨╡╨╗╤М╨╜╨╛╨╡: ╨╜╨░╨║╨╛╨┐╨╗╨╡╨╜╨╕╨╡ ╤В╨╡╨╗╨░ POST-╨╖╨░╨┐╤А╨╛╤Б╨░ ╨╕ ╤А╨░╨╖╨▒╨╛╤А JSON
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
//  ╨Ъ╨╛╨╗╤М╤Ж╨╡╨▓╨╛╨╣ ╨╗╨╛╨│ ╤Б╨╛╨▒╤Л╤В╨╕╨╣
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
//  WebSocket: push-╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨▓╤Б╨╡╨╝ ╨║╨╗╨╕╨╡╨╜╤В╨░╨╝
// ---------------------------------------------------------------------
void WebServerManager::notifyClients() {
  if (_ws.count() > 0) {
    _ws.textAll(buildStatusJson());
  }
}

// ---------------------------------------------------------------------
//  ╨в╨░╨╣╨╝╨╡╤А ╤Б╨╜╨░
// ---------------------------------------------------------------------
void WebServerManager::setSleepTimer(uint32_t minutes) {
  if (minutes == 0) { cancelSleepTimer(); return; }
  _sleepActive = true;
  _sleepEnd    = millis() + minutes * 60UL * 1000UL;
  Serial.printf("[Web] ╨в╨░╨╣╨╝╨╡╤А ╤Б╨╜╨░: %u ╨╝╨╕╨╜\n", minutes);
}

void WebServerManager::cancelSleepTimer() {
  _sleepActive = false;
  _sleepEnd    = 0;
  Serial.println(F("[Web] ╨в╨░╨╣╨╝╨╡╤А ╤Б╨╜╨░ ╨╛╤В╨╝╨╡╨╜╤С╨╜"));
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
    addLog("╨в╨░╨╣╨╝╨╡╤А ╤Б╨╜╨░: ╨▓╤Л╨║╨╗╤О╤З╨╡╨╜╨╕╨╡");
    notifyClients();
    Serial.println(F("[Web] ╨в╨░╨╣╨╝╨╡╤А ╤Б╨╜╨░: ╤Г╤Б╤В╤А╨╛╨╣╤Б╤В╨▓╨╛ ╨▓╤Л╨║╨╗╤О╤З╨╡╨╜╨╛"));
  }
}

// ---------------------------------------------------------------------
//  ╨д╨╛╤А╨╝╨╕╤А╨╛╨▓╨░╨╜╨╕╨╡ JSON
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
  doc["charged"]    = Battery.isCharged();
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
//  ╨Ь╨░╤А╤И╤А╤Г╤В╤Л
// ---------------------------------------------------------------------
void WebServerManager::setupRoutes() {

  // --- WebSocket ---
  _ws.onEvent([](AsyncWebSocket*, AsyncWebSocketClient*, AwsEventType type,
                 void*, uint8_t*, size_t) {
    // ╨Я╤А╨╕ ╨┐╨╛╨┤╨║╨╗╤О╤З╨╡╨╜╨╕╨╕ ╨╜╨╛╨▓╨╛╨│╨╛ ╨║╨╗╨╕╨╡╨╜╤В╨░ ╨╛╨╜ ╤Б╤А╨░╨╖╤Г ╨┐╨╛╨╗╤Г╤З╨╕╤В ╤Б╤В╨░╤В╤Г╤Б
    if (type == WS_EVT_CONNECT) Web.notifyClients();
  });
  _server.addHandler(&_ws);

  // --- ╨У╨╗╨░╨▓╨╜╨░╤П ╤Б╤В╤А╨░╨╜╨╕╤Ж╨░ (SPA, gzip) ---
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "text/html", INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
    resp->addHeader("Content-Encoding", "gzip");
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
  });

  // --- ╨б╤В╤А╨░╨╜╨╕╤Ж╨░ ╤А╨░╨╖╤А╨░╨▒╨╛╤В╤З╨╕╨║╨░ (gzip) ---
  _server.on("/dev", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "text/html", DEV_HTML_GZ, DEV_HTML_GZ_LEN);
    resp->addHeader("Content-Encoding", "gzip");
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

  // --- PWA Icon (SVG ╤Б ╨░╨┤╨░╨┐╤В╨╕╨▓╨╜╨╛╨╣ ╤В╨╡╨╝╨╛╨╣) ---
  _server.on("/icon.svg", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "image/svg+xml", (const uint8_t*)ICON_SVG, sizeof(ICON_SVG) - 1);
    resp->addHeader("Cache-Control", "public, max-age=86400");
    req->send(resp);
  });

  // --- PNG Icon (╤А╨░╤Б╤В╤А╨╛╨▓╤Л╨╣ ╨┤╨╗╤П Android Chrome PWA) ---
  _server.on("/icon.png", HTTP_GET, [](AsyncWebServerRequest* req) {
    AsyncWebServerResponse* resp =
      req->beginResponse(200, "image/png", ICON_PNG, ICON_PNG_LEN);
    resp->addHeader("Cache-Control", "public, max-age=86400");
    req->send(resp);
  });

  // --- Service Worker (╤В╤А╨╡╨▒╤Г╨╡╤В╤Б╤П Chrome ╨┤╨╗╤П standalone WebAPK) ---
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
          addLog(String("╨Я╨╕╤В╨░╨╜╨╕╨╡: ") + (Config.data.isOn ? "╨Т╨Ъ╨Ы" : "╨Т╨л╨Ъ╨Ы"));
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
          addLog("╨ж╨▓╨╡╤В ╨╕╨╖╨╝╨╡╨╜╤С╨╜");
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
          addLog(String("╨н╤Д╤Д╨╡╨║╤В: ") + id);
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
          addLog("╨Я╨╛╨┤╨║╨╗╤О╤З╨╡╨╜╨╕╨╡ ╨║ Wi-Fi: " + ssid);
          // ╨Ю╤В╨▓╨╡╤З╨░╨╡╨╝ ╤Б╤А╨░╨╖╤Г, ╤Б╨░╨╝╨╛ ╨┐╨╛╨┤╨║╨╗╤О╤З╨╡╨╜╨╕╨╡ тАФ ╨░╤Б╨╕╨╜╤Е╤А╨╛╨╜╨╜╨╛
          JsonDocument res;
          res["ok"] = (ssid.length() > 0);
          String out; serializeJson(res, out);
          r->send(200, "application/json", out);
          if (ssid.length() > 0) Wifi.connectToWifi(ssid, pass);
        });
    });

  // --- POST /api/wifi/forget ---
  _server.on("/api/wifi/forget", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("╨б╨╡╤В╤М Wi-Fi ╨╖╨░╨▒╤Л╤В╨░");
    req->send(200, "application/json", "{\"ok\":true}");
    Wifi.forget();
  });

  // --- POST /api/settings (╤З╨░╤Б╤В╨╕╤З╨╜╨╛╨╡ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡) ---
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
          addLog("╨Э╨░╤Б╤В╤А╨╛╨╣╨║╨╕ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╤Л");
          r->send(200, "application/json", buildSettingsJson());
        });
    });

  // --- POST /api/settings/reset ---
  _server.on("/api/settings/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("╨б╨▒╤А╨╛╤Б ╨╜╨░╤Б╤В╤А╨╛╨╡╨║");
    req->send(200, "application/json", "{\"ok\":true}");
    Config.reset();
    applyCurrentState();
  });

  // --- POST /api/ota/upload (╨╖╨░╨│╤А╤Г╨╖╨║╨░ .bin ╤Д╨░╨╣╨╗╨░ ╨╕╨╖ ╨▒╤А╨░╤Г╨╖╨╡╤А╨░) ---
  _server.on("/api/ota/upload", HTTP_POST,
    [this](AsyncWebServerRequest* req) {
      bool success = !Update.hasError();
      AsyncWebServerResponse* resp = req->beginResponse(
        200, "application/json",
        success ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"╨Ю╤И╨╕╨▒╨║╨░ ╨┐╤А╨╛╤И╨╕╨▓╨║╨╕\"}"
      );
      resp->addHeader("Connection", "close");
      req->send(resp);
      if (success) {
        addLog("╨Я╤А╨╛╤И╨╕╨▓╨║╨░ ╨╕╨╖ ╤Д╨░╨╣╨╗╨░ ╨╖╨░╨▓╨╡╤А╤И╨╡╨╜╨░ ╤Г╤Б╨┐╨╡╤И╨╜╨╛");
        Ota.onUpdateSuccess();
      } else {
        addLog("╨Ю╤И╨╕╨▒╨║╨░ ╨┐╤А╨╛╤И╨╕╨▓╨║╨╕ ╨╕╨╖ ╤Д╨░╨╣╨╗╨░");
        Ota.onUpdateError();
      }
    },
    [this](AsyncWebServerRequest* req, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
      if (!index) {
        addLog("╨б╤В╨░╤А╤В ╨╖╨░╨│╤А╤Г╨╖╨║╨╕ ╤Д╨░╨╣╨╗╨░ ╨┐╤А╨╛╤И╨╕╨▓╨║╨╕: " + filename);
        Serial.printf("[OTA] ╨Ч╨░╨│╤А╤Г╨╖╨║╨░ ╤Д╨░╨╣╨╗╨░: %s\n", filename.c_str());
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
          Serial.printf("[OTA] ╨д╨░╨╣╨╗ ╤Г╤Б╨┐╨╡╤И╨╜╨╛ ╨┐╨╛╨╗╤Г╤З╨╡╨╜, ╤А╨░╨╖╨╝╨╡╤А: %u ╨▒╨░╨╣╤В\n", index + len);
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

  // --- POST /api/ota/check (╨┐╤А╨╕╨╜╤Г╨┤╨╕╤В╨╡╨╗╤М╨╜╨░╤П ╨┐╤А╨╛╨▓╨╡╤А╨║╨░ GitHub) ---
  _server.on("/api/ota/check", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("╨Ч╨░╨┐╤А╨╛╤Б ╨┐╤А╨╛╨▓╨╡╤А╨║╨╕ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╣ ╨╜╨░ GitHub");
    Ota.checkGitHubUpdate();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  // --- POST /api/ota/github (╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╡ ╨┤╨╛ ╨▓╨╡╤А╤Б╨╕╨╕ ╤Б GitHub) ---
  _server.on("/api/ota/github", HTTP_POST, [this](AsyncWebServerRequest* req) {
    if (!Ota.hasUpdate() || Ota.getLatestUrl().length() == 0) {
      req->send(400, "application/json", "{\"ok\":false,\"error\":\"╨Э╨╡╤В ╨┤╨╛╤Б╤В╤Г╨┐╨╜╤Л╤Е ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╨╣\"}");
      return;
    }
    addLog("╨Ч╨░╨┐╤Г╤Б╨║ ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╕╤П ╤Б GitHub: " + Ota.getLatestVersion());
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
            r->send(400, "application/json", "{\"ok\":false,\"error\":\"╨Я╤Г╤Б╤В╨╛╨╣ URL\"}");
            return;
          }
          addLog("OTA ╨┐╨╛ URL: " + url);
          r->send(200, "application/json", "{\"ok\":true}");
          Ota.updateFromUrl(url);
        });
    });

  // --- POST /api/reboot (╨░╤Б╨╕╨╜╤Е╤А╨╛╨╜╨╜╨╛, ╤З╨╡╤А╨╡╨╖ ╤Д╨╗╨░╨│) ---
  _server.on("/api/reboot", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("╨Я╨╡╤А╨╡╨╖╨░╨│╤А╤Г╨╖╨║╨░ ╤Г╤Б╤В╤А╨╛╨╣╤Б╤В╨▓╨░");
    req->send(200, "application/json", "{\"ok\":true}");
    _pendingReboot = true;   // ╤А╨╡╨░╨╗╤М╨╜╤Л╨╣ restart тАФ ╨▓ loop()
  });

  // --- POST /api/test/led ---
  _server.on("/api/test/led", HTTP_POST, [this](AsyncWebServerRequest* req) {
    addLog("╨в╨╡╤Б╤В LED: ╤А╨░╨┤╤Г╨│╨░ 5╤Б");
    Config.data.isOn = true;
    Config.data.currentEffect = EFFECT_RAINBOW;
    applyCurrentState();
    req->send(200, "application/json", "{\"ok\":true}");
  });

  // ===================================================================
  //  ╨Ш╨Ч╨С╨а╨Р╨Э╨Э╨л╨Х ╨б╨ж╨Х╨Э╨л (Favorites)
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
          // ╨Ш╨╝╤П
          const char* name = doc["name"] | "";
          strncpy(f.name, (name[0] ? name : (String("╨б╤Ж╨╡╨╜╨░ ") + (slot+1)).c_str()),
                  FAVORITES_NAME_LEN - 1);
          f.name[FAVORITES_NAME_LEN - 1] = '\0';
          Config.saveFavoriteSlot(slot);
          addLog(String("╨б╨╛╤Е╤А╨░╨╜╨╡╨╜╨╛ ╨▓ ╤Б╨╗╨╛╤В ") + slot);
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
          addLog(String("╨Ч╨░╨│╤А╤Г╨╢╨╡╨╜╨░ ╤Б╤Ж╨╡╨╜╨░: ") + f.name);
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
          addLog(String("╨б╨╗╨╛╤В ") + slot + " ╤Г╨┤╨░╨╗╤С╨╜");
          r->send(200, "application/json", buildFavoritesJson());
        });
    });

  // ===================================================================
  //  ╨в╨Р╨Щ╨Ь╨Х╨а ╨б╨Э╨Р
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
            addLog(String("╨в╨░╨╣╨╝╨╡╤А ╤Б╨╜╨░: ") + minutes + " ╨╝╨╕╨╜");
          else
            addLog("╨в╨░╨╣╨╝╨╡╤А ╤Б╨╜╨░ ╨╛╤В╨╝╨╡╨╜╤С╨╜");
          r->send(200, "application/json", buildStatusJson());
        });
    });

  // ===================================================================
  //  ╨а╨Р╨б╨Я╨Ш╨б╨Р╨Э╨Ш╨Х
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
          addLog(String("╨а╨░╤Б╨┐╨╕╤Б╨░╨╜╨╕╨╡ ╤Б╨╗╨╛╤В ") + slot + " ╨╛╨▒╨╜╨╛╨▓╨╗╨╡╨╜╨╛");
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
          addLog(String("╨а╨░╤Б╨┐╨╕╤Б╨░╨╜╨╕╨╡ ╤Б╨╗╨╛╤В ") + slot + " ╤Г╨┤╨░╨╗╤С╨╜");
          r->send(200, "application/json", buildSchedulesJson());
        });
    });

  // --- Captive portal: ╨╜╨╡╨╕╨╖╨▓╨╡╤Б╤В╨╜╤Л╨╡ ╨╝╨░╤А╤И╤А╤Г╤В╤Л -> ╨│╨╗╨░╨▓╨╜╨░╤П ---
  _server.onNotFound([](AsyncWebServerRequest* req) {
    if (Wifi.isAP()) {
      AsyncWebServerResponse* resp =
        req->beginResponse(200, "text/html", INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
      resp->addHeader("Content-Encoding", "gzip");
      req->send(resp);
    } else {
      req->send(404, "text/plain", "Not found");
    }
  });
}

void WebServerManager::begin() {
  setupRoutes();
  _server.begin();
  addLog("╨Т╨╡╨▒-╤Б╨╡╤А╨▓╨╡╤А ╨╖╨░╨┐╤Г╤Й╨╡╨╜");
  Serial.println(F("[Web] HTTP/WebSocket ╤Б╨╡╤А╨▓╨╡╤А ╨╖╨░╨┐╤Г╤Й╨╡╨╜ ╨╜╨░ ╨┐╨╛╤А╤В╤Г 80"));
}

