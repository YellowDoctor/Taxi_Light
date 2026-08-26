// =====================================================================
//  WiFiManager.cpp — реализация Wi-Fi логики и captive portal
// =====================================================================
#include "WiFiManager.h"
#include "SettingsManager.h"

WiFiManager Wifi;   // глобальный экземпляр

static const byte DNS_PORT = 53;

void WiFiManager::begin() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (Config.data.wifiSSID.length() > 0) {
    // Пытаемся подключиться к сохранённой сети
    startSTA(Config.data.wifiSSID, Config.data.wifiPassword);
  } else {
    // Сети нет — сразу поднимаем точку доступа
    startAP();
  }
}

void WiFiManager::startSTA(const String& ssid, const String& pass) {
  Serial.printf("[WiFi] Подключение к сети \"%s\"...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  _state = WifiState::CONNECTING;
  _connectStart = millis();
}

void WiFiManager::startAP() {
  Serial.println(F("[WiFi] Запуск точки доступа TaxiLight"));
  WiFi.mode(WIFI_AP);
  IPAddress apIP(AP_IP_ADDR);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  // Captive portal: перенаправляем все домены на IP точки доступа
  _dns.setErrorReplyCode(DNSReplyCode::NoError);
  _dns.start(DNS_PORT, "*", apIP);
  _dnsActive = true;
  _state = WifiState::AP_MODE;

  Serial.printf("[WiFi] AP поднята, IP: %s\n", apIP.toString().c_str());
}

bool WiFiManager::connectToWifi(const String& ssid, const String& pass) {
  // Сохраняем и пытаемся подключиться
  Config.data.wifiSSID = ssid;
  Config.data.wifiPassword = pass;
  Config.save();

  if (_dnsActive) { _dns.stop(); _dnsActive = false; }
  startSTA(ssid, pass);

  // Блокирующее ожидание результата (вызывается из обработчика запроса)
  uint32_t start = millis();
  while (millis() - start < WIFI_CONNECT_TIMEOUT) {
    if (WiFi.status() == WL_CONNECTED) {
      _state = WifiState::CONNECTED;
      Serial.printf("[WiFi] Подключено, IP: %s\n", WiFi.localIP().toString().c_str());
      return true;
    }
    delay(100);
  }
  Serial.println(F("[WiFi] Не удалось подключиться, возврат в режим AP"));
  startAP();
  return false;
}

void WiFiManager::forget() {
  Config.data.wifiSSID = "";
  Config.data.wifiPassword = "";
  Config.save();
  WiFi.disconnect(true, true);
  startAP();
}

void WiFiManager::loop() {
  // Обслуживание DNS в режиме captive portal
  if (_dnsActive) _dns.processNextRequest();

  switch (_state) {
    case WifiState::CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        _state = WifiState::CONNECTED;
        Serial.printf("[WiFi] Подключено, IP: %s\n",
                      WiFi.localIP().toString().c_str());
      } else if (millis() - _connectStart > WIFI_CONNECT_TIMEOUT) {
        Serial.println(F("[WiFi] Таймаут подключения -> AP"));
        startAP();
      }
      break;

    case WifiState::CONNECTED:
      // Потеряли связь — пробуем переподключиться, иначе AP
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[WiFi] Соединение потеряно"));
        _state = WifiState::DISCONNECTED;
        _lastRetry = millis();
      }
      break;

    case WifiState::DISCONNECTED:
      if (millis() - _lastRetry > WIFI_RETRY_INTERVAL) {
        _lastRetry = millis();
        if (Config.data.wifiSSID.length() > 0) {
          startSTA(Config.data.wifiSSID, Config.data.wifiPassword);
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
    case WifiState::CONNECTED:  return "Подключено";
    case WifiState::CONNECTING: return "Подключение...";
    case WifiState::AP_MODE:    return "Точка доступа";
    default:                    return "Не подключено";
  }
}

String WiFiManager::getIP() {
  if (_state == WifiState::AP_MODE) return WiFi.softAPIP().toString();
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
