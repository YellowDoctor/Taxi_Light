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
  bool updateFromUrl(const String& url); // OTA по HTTP-URL

private:
  bool _started = false;
};

extern OtaManager Ota;   // глобальный экземпляр
