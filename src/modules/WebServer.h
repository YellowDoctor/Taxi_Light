#pragma once
// =====================================================================
//  WebServer — REST API + отдача встроенного веб-интерфейса
// =====================================================================
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "../Config.h"

class WebServerManager {
public:
  void begin();
  void addLog(const String& line);      // добавить строку в кольцевой лог

private:
  AsyncWebServer _server{80};

  void setupRoutes();
  String buildStatusJson();             // JSON для /api/status
  String buildSettingsJson();           // JSON для /api/settings
  String buildLogJson();                // JSON лога событий

  // Кольцевой лог событий для страницы /dev
  String   _log[STATUS_LOG_SIZE];
  uint8_t  _logHead = 0;
  uint8_t  _logCount = 0;
};

extern WebServerManager Web;   // глобальный экземпляр

// Реализуется в main.cpp: применить текущие настройки к железу
void applyCurrentState();
