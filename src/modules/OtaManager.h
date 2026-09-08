#pragma once
// =====================================================================
//  OtaManager — обновление прошивки (ArduinoOTA + OTA по URL)
// =====================================================================
#include <Arduino.h>
#include "../Config.h"

class OtaManager {
public:
  void begin();
  void handle();                        // вызывать из loop()
  bool updateFromUrl(const String& url); // OTA по HTTP/HTTPS URL
  void checkGitHubUpdate();             // асинхронная проверка версии на GitHub
  void _doCheckGitHub();                // внутренняя реализация проверки

  bool   hasUpdate()        const { return _hasUpdate; }
  String getLatestVersion() const { return _latestVersion; }
  String getLatestUrl()     const { return _latestUrl; }
  String getUpdateNotes()   const { return _updateNotes; }
  bool   isUpdating()       const { return _updating; }

  // Колбэки индикации и безопасности
  void onUpdateStart();
  void onUpdateSuccess();
  void onUpdateError();

private:
  bool   _started       = false;
  bool   _updating      = false;
  bool   _hasUpdate     = false;
  String _latestVersion = "";
  String _latestUrl     = "";
  String _updateNotes   = "";
};

extern OtaManager Ota;   // глобальный экземпляр
