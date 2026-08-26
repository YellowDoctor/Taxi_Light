#pragma once
// =====================================================================
//  WebServer — REST API + WebSocket + встроенный веб-интерфейс
// =====================================================================
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include "../Config.h"

class WebServerManager {
public:
  void begin();
  void addLog(const String& line);      // добавить строку в кольцевой лог
  void notifyClients();                  // push-обновление всем WS-клиентам

  // Флаг асинхронной перезагрузки (проверяется в main::loop)
  bool pendingReboot() { return _pendingReboot; }

  // Управление таймером сна (вызывается из API)
  void  setSleepTimer(uint32_t minutes);
  void  cancelSleepTimer();
  int32_t sleepTimerLeft() const;      // секунды до выключения, -1 если нет
  void  tickSleepTimer();              // вызывается из main::loop()

private:
  AsyncWebServer _server{80};
  AsyncWebSocket _ws{"/ws"};

  bool     _pendingReboot = false;

  // Таймер сна
  bool     _sleepActive  = false;
  uint32_t _sleepEnd     = 0;          // millis() момента выключения

  void setupRoutes();
  String buildStatusJson();
  String buildSettingsJson();
  String buildLogJson();
  String buildFavoritesJson();
  String buildSchedulesJson();

  // Кольцевой лог событий
  String   _log[STATUS_LOG_SIZE];
  uint8_t  _logHead  = 0;
  uint8_t  _logCount = 0;
};

extern WebServerManager Web;   // глобальный экземпляр

// Реализуется в main.cpp: применить текущие настройки к железу
void applyCurrentState();
