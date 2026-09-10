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
  int    getProgress()      const { return _progress; }
  void   setProgress(int p)       { _progress = p; }

  // Колбэки индикации и безопасности
  void onUpdateStart();
  void onUpdateSuccess();
  void onUpdateError();

private:
  void   _doUpdateFromUrl(const String& url);
  bool   _started       = false;
  volatile bool _updating = false;
  volatile int  _progress = 0;
  volatile bool _hasUpdate = false;
  String _latestVersion = "";
  String _latestUrl     = "";
  String _updateNotes   = "";
};

extern OtaManager Ota;   // глобальный экземпляр
