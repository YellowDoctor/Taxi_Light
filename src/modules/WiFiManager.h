#pragma once
// =====================================================================
//  WiFiManager — подключение к сети / точка доступа / captive portal
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "../Config.h"

enum class WifiState : uint8_t {
  DISCONNECTED,   // нет соединения
  CONNECTING,     // идёт подключение
  CONNECTED,      // подключено к домашней сети (STA)
  AP_MODE         // режим точки доступа
};

class WiFiManager {
public:
  void begin();
  void loop();

  WifiState getState() const { return _state; }
  String    getStatus();                 // человекочитаемый статус
  String    getIP();
  int       getRSSI();
  bool      isConnected();               // подключено в режиме STA
  bool      isAP() const { return _state == WifiState::AP_MODE; }

  void startAP();                        // принудительно поднять AP
  bool connectToWifi(const String& ssid, const String& pass);
  void forget();                         // забыть сеть и уйти в AP

private:
  WifiState  _state = WifiState::DISCONNECTED;
  DNSServer  _dns;
  bool       _dnsActive = false;
  uint32_t   _lastRetry = 0;
  uint32_t   _connectStart = 0;

  void startSTA(const String& ssid, const String& pass);
};

extern WiFiManager Wifi;   // глобальный экземпляр
