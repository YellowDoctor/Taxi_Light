// =====================================================================
//  WiFiManager.cpp — реализация Wi-Fi логики, captive portal, NTP
// =====================================================================
#include "WiFiManager.h"
#include "SettingsManager.h"
#include <time.h>
#include <ESPmDNS.h>
#include <NetBIOS.h>

WiFiManager Wifi;   // глобальный экземпляр

static const byte DNS_PORT = 53;

void WiFiManager::begin() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (Config.data.wifiSSID.length() > 0) {
    startSTA(Config.data.wifiSSID, Config.data.wifiPassword);
  } else {
    startAP();
  }
}

void WiFiManager::startSTA(const String& ssid, const String& pass) {
  Serial.printf("[WiFi] Подключение к сети \"%s\"...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  _state        = WifiState::CONNECTING;
  _connectStart = millis();
}

void WiFiManager::startAP() {
  Serial.println(F("[WiFi] Запуск точки доступа TaxiLight"));
  WiFi.mode(WIFI_AP);
  IPAddress apIP(AP_IP_ADDR);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  _dns.setErrorReplyCode(DNSReplyCode::NoError);
  _dns.start(DNS_PORT, "*", apIP);
  _dnsActive   = true;
  _state       = WifiState::AP_MODE;
  _retryCount  = 0;

  Serial.printf("[WiFi] AP поднята, IP: %s\n", apIP.toString().c_str());
}

// Асинхронная смена Wi-Fi — никакого delay() внутри обработчика
bool WiFiManager::connectToWifi(const String& ssid, const String& pass) {
  Config.data.wifiSSID     = ssid;
  Config.data.wifiPassword = pass;
  Config.save();

  if (_dnsActive) { _dns.stop(); _dnsActive = false; }
  startSTA(ssid, pass);
  return true;   // фактический результат узнаём через getStatus() / /api/status
}

void WiFiManager::forget() {
  Config.data.wifiSSID     = "";
  Config.data.wifiPassword = "";
  Config.save();
  MDNS.end();
  NBNS.end();
  WiFi.disconnect(true, true);
  startAP();
}


void WiFiManager::syncNTP() {
  if (_state != WifiState::CONNECTED) return;
  configTime(Config.data.timezone * 3600L, 0, NTP_SERVER1, NTP_SERVER2);
  // Ждём синхронизации не более 5с (неблокирующий вариант через флаг)
  uint32_t start = millis();
  struct tm t;
  while (!getLocalTime(&t, 100) && millis() - start < 5000) {}
  _timeSynced = getLocalTime(&t, 0);
  if (_timeSynced)
    Serial.printf("[WiFi] NTP синхронизирован: %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);
  else
    Serial.println(F("[WiFi] NTP: не удалось синхронизировать время"));
}

void WiFiManager::loop() {
  if (_dnsActive) _dns.processNextRequest();

  switch (_state) {
    case WifiState::CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        _state      = WifiState::CONNECTED;
        _retryCount = 0;
        Serial.printf("[WiFi] Подключено, IP: %s\n", WiFi.localIP().toString().c_str());
        syncNTP();
        if (MDNS.begin("taxilight")) {
          MDNS.addService("http", "tcp", 80);
          Serial.println(F("[WiFi] mDNS запущен: http://taxilight.local"));
        }
        NBNS.begin("taxilight");
        Serial.println(F("[WiFi] NetBIOS запущен: http://taxilight"));
      } else if (millis() - _connectStart > WIFI_CONNECT_TIMEOUT) {
        Serial.println(F("[WiFi] Таймаут подключения"));
        if (++_retryCount >= WIFI_MAX_RETRIES) {
          Serial.println(F("[WiFi] Превышен лимит попыток -> AP"));
          _retryCount = 0;
          startAP();
        } else {
          // Ещё одна попытка через WIFI_RETRY_INTERVAL
          _state     = WifiState::DISCONNECTED;
          _lastRetry = millis();
        }
      }
      break;

    case WifiState::CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[WiFi] Соединение потеряно"));
        MDNS.end();
        NBNS.end();
        _state     = WifiState::DISCONNECTED;
        _lastRetry = millis();
        _timeSynced = false;
      }
      break;


    case WifiState::DISCONNECTED:
      if (millis() - _lastRetry > WIFI_RETRY_INTERVAL) {
        _lastRetry = millis();
        if (Config.data.wifiSSID.length() > 0) {
          if (++_retryCount >= WIFI_MAX_RETRIES) {
            Serial.println(F("[WiFi] Нет связи, переходим в AP"));
            _retryCount = 0;
            startAP();
          } else {
            startSTA(Config.data.wifiSSID, Config.data.wifiPassword);
          }
        } else {
          startAP();
        }
      }
      break;

    case WifiState::AP_MODE:
    default:
      break;
  }
}

String WiFiManager::getStatus() {
  switch (_state) {
    case WifiState::CONNECTED:    return "connected";
    case WifiState::CONNECTING:   return "connecting";
    case WifiState::AP_MODE:      return "ap_mode";
    default:                      return "disconnected";
  }
}

String WiFiManager::getIP() {
  if (_state == WifiState::AP_MODE)   return WiFi.softAPIP().toString();
  if (_state == WifiState::CONNECTED) return WiFi.localIP().toString();
  return "0.0.0.0";
}

int WiFiManager::getRSSI() {
  if (_state == WifiState::CONNECTED) return WiFi.RSSI();
  return 0;
}

bool WiFiManager::isConnected() {
  return _state == WifiState::CONNECTED;
}
