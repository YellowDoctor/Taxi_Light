// =====================================================================
//  main.cpp — умный ночник «Такси Шашка» на ESP32
//  Версия 1.2. Неблокирующий loop(), модульная архитектура.
// =====================================================================
#include <Arduino.h>
#include "Config.h"
#include "modules/SettingsManager.h"
#include "modules/LedManager.h"
#include "modules/EffectsManager.h"
#include "modules/BatteryManager.h"
#include "modules/ButtonManager.h"
#include "modules/WiFiManager.h"
#include "modules/OtaManager.h"
#include "modules/WebServer.h"
#include "modules/SunriseManager.h"
#include "modules/ScheduleManager.h"

// ---------------------------------------------------------------------
//  Применение текущих настроек к железу (LED + эффекты)
//  Вызывается из WebServer при изменении параметров.
// ---------------------------------------------------------------------
void applyCurrentState() {
  if (Config.data.isOn) {
    Led.turnOn();
    Led.setBrightness(Config.data.brightness);
    Effects.setColor((Config.data.color >> 16) & 0xFF,
                     (Config.data.color >>  8) & 0xFF,
                      Config.data.color        & 0xFF);
    Effects.setSpeed(Config.data.effectSpeed);
    Effects.setEffect(Config.data.currentEffect);
  } else {
    Led.turnOff();
  }
}

// ---------------------------------------------------------------------
//  Обработчики жестов сенсорной кнопки
// ---------------------------------------------------------------------
static int8_t brightDir = 1;   // направление регулировки яркости

// Выполнить действие по коду (используется для настраиваемых действий)
void doAction(uint8_t action) {
  switch (action) {
    case 0:  // вкл/выкл
      Config.data.isOn = !Config.data.isOn;
      applyCurrentState();
      Config.save();
      Web.addLog("Кнопка: питание");
      Web.notifyClients();
      break;
    case 1:  // следующий эффект
      Config.data.currentEffect = (Config.data.currentEffect + 1) % EFFECT_COUNT;
      if (!Config.data.isOn) { Config.data.isOn = true; }
      applyCurrentState();
      Config.save();
      Web.addLog("Кнопка: смена эффекта");
      Web.notifyClients();
      break;
    default: break;
  }
}

void onSingleTap() { doAction(Config.data.touchAction1); }
void onDoubleTap() { doAction(Config.data.touchAction2); }

void onHold() {
  // Sunrise: при удержании отменяем активный рассвет
  if (Sunrise.isActive()) Sunrise.cancel();

  // Плавная регулировка яркости вверх/вниз
  int v = Config.data.brightness + brightDir * 8;
  if (v >= 255) { v = 255; brightDir = -1; }
  if (v <= 5)   { v = 5;   brightDir = 1; }
  Config.data.brightness = (uint8_t)v;
  if (!Config.data.isOn) { Config.data.isOn = true; Led.turnOn(); }
  Led.setBrightness(Config.data.brightness);
}

void onRelease() {
  // Вызывается при отпускании после удержания — сохраняем яркость
  Config.save();
  Web.notifyClients();
}

void onLongHold() {
  // Долгое удержание -> принудительный запуск точки доступа (сопряжение)
  Web.addLog("Кнопка: запуск режима сопряжения (AP)");
  Wifi.startAP();
}

// ---------------------------------------------------------------------
//  setup()
// ---------------------------------------------------------------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println();
  Serial.println(F("====================================="));
  Serial.printf ("  Такси Шашка  v%s\n", FIRMWARE_VERSION);
  Serial.println(F("====================================="));

  // 1. Настройки
  Config.begin();

  // 2. Светодиоды + эффекты
  Led.begin();
  Effects.begin();
  applyCurrentState();

  // 3. Аккумулятор
  Battery.begin();

  // 4. Кнопка + назначение жестов
  Button.begin();
  Button.onSingle   = onSingleTap;
  Button.onDouble   = onDoubleTap;
  Button.onHold     = onHold;
  Button.onRelease  = onRelease;
  Button.onLongHold = onLongHold;

  // 5. Wi-Fi
  Wifi.begin();

  // 6. OTA
  Ota.begin();

  // 7. Веб-сервер
  Web.begin();

  // 8. Sunrise + Scheduler
  Sunrise.begin();
  Scheduler.begin();

  Serial.printf("[SYS] Инициализация завершена, свободно heap: %u байт\n",
                ESP.getFreeHeap());
}

// ---------------------------------------------------------------------
//  loop() — только неблокирующие вызовы
// ---------------------------------------------------------------------
void loop() {
  Wifi.loop();        // обслуживание Wi-Fi и captive DNS
  Ota.handle();       // ArduinoOTA
  Button.tick();      // сенсорная кнопка
  Battery.tick();     // измерение аккумулятора (раз в 10с)
  Sunrise.tick();     // будильник-рассвет
  Scheduler.tick();   // расписание (раз в 30с)
  Web.tickSleepTimer(); // таймер сна

  // Эффекты (если sunrise не рисует сам)
  if (!Sunrise.isActive()) Effects.tick();

  // Асинхронная перезагрузка (из /api/reboot)
  if (Web.pendingReboot()) {
    delay(200);
    ESP.restart();
  }

  // Защита от переполнения при экстремально низкой памяти
  static uint32_t lastHeapCheck = 0;
  if (millis() - lastHeapCheck > 30000) {
    lastHeapCheck = millis();
    if (ESP.getFreeHeap() < 8000) {
      Serial.println(F("[SYS] ВНИМАНИЕ: критически мало памяти!"));
      Web.addLog("ВНИМАНИЕ: мало памяти");
    }
  }
}
