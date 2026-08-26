#pragma once
// =====================================================================
//  WiFiManager — Wi-Fi, AP, captive portal, NTP
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "../Config.h"

enum class WifiState { IDLE, CONNECTING, CONNECTED, DISCONNECTED, AP_MODE };

class WiFiManager {
public:
  void begin();
  void loop();

  // Подключиться к точке доступа (асинхронно — без блокирующего delay)
  bool connectToWifi(const String& ssid, const String& pass);
  void startAP();
  void forget();

  String getStatus();       // machine-readable: "connected"/"connecting"/"ap_mode"/"disconnected"
  String getIP();
  int    getRSSI();
  bool   isConnected();
  bool   isAP() { return _state == WifiState::AP_MODE; }

  // NTP
  bool   isTimeSynced() { return _timeSynced; }
  void   syncNTP();

private:
  WifiState  _state      = WifiState::IDLE;
  DNSServer  _dns;
  bool       _dnsActive  = false;
  uint32_t   _connectStart = 0;
  uint32_t   _lastRetry    = 0;
  uint8_t    _retryCount   = 0;
  bool       _timeSynced   = false;

  void startSTA(const String& ssid, const String& pass);
};

extern WiFiManager Wifi;   // глобальный экземпляр
