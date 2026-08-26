// =====================================================================
//  ScheduleManager.cpp — реализация расписания
// =====================================================================
#include "ScheduleManager.h"
#include "SettingsManager.h"
#include "LedManager.h"
#include "EffectsManager.h"
#include "WiFiManager.h"
#include <time.h>

ScheduleManager Scheduler;   // глобальный экземпляр

void ScheduleManager::begin() {
  _lastCheck  = 0;
  _lastMinute = -1;
  Serial.println(F("[Scheduler] Менеджер расписания запущен"));
}

void ScheduleManager::tick() {
  // Проверяем раз в 30 секунд
  if (millis() - _lastCheck < 30000) return;
  _lastCheck = millis();

  // Требуется подключение в режиме STA (время синхронизируется при коннекте)
  if (!Wifi.isConnected()) return;

  struct tm t;
  if (!getLocalTime(&t, 0)) return;

  int currentMinute = t.tm_hour * 60 + t.tm_min;

  // Чтобы не срабатывать дважды в одну минуту
  if (currentMinute == _lastMinute) return;
  _lastMinute = currentMinute;

  // Определяем текущий день недели (0=Вс, 1=Пн...6=Сб -> bitmask: bit0=Пн..bit6=Вс)
  // tm_wday: 0=Sun, 1=Mon..6=Sat
  uint8_t dayBit;
  if (t.tm_wday == 0) dayBit = (1 << 6);            // воскресенье -> bit6
  else                dayBit = (1 << (t.tm_wday - 1)); // Пн=bit0 .. Сб=bit5

  for (uint8_t i = 0; i < SCHEDULE_COUNT; i++) {
    const Schedule& s = Config.data.schedules[i];
    if (!s.enabled) continue;
    if (!(s.days & dayBit)) continue;
    if (s.hour   != (uint8_t)t.tm_hour) continue;
    if (s.minute != (uint8_t)t.tm_min)  continue;

    // Срабатывание!
    Config.data.isOn = s.action;
    Config.save();

    if (s.action) {
      Led.turnOn();
      Led.setBrightness(Config.data.brightness);
      Effects.setColor(
        (Config.data.color >> 16) & 0xFF,
        (Config.data.color >>  8) & 0xFF,
         Config.data.color        & 0xFF);
      Effects.setEffect(Config.data.currentEffect);
    } else {
      Led.turnOff();
    }

    Serial.printf("[Scheduler] Слот %d: %s в %02d:%02d\n",
      i, s.action ? "ВКЛ" : "ВЫКЛ", s.hour, s.minute);
  }
}
