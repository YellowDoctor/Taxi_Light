#pragma once
// =====================================================================
//  ScheduleManager — расписание включения/выключения
//  Работает с NTP-временем. Проверяет расписание раз в минуту.
// =====================================================================
#include <Arduino.h>
#include "../Config.h"

class ScheduleManager {
public:
  void begin();
  void tick();   // вызывается из loop()

private:
  uint32_t _lastCheck  = 0;
  int      _lastMinute = -1;   // чтобы не срабатывать дважды в одну минуту
};

extern ScheduleManager Scheduler;   // глобальный экземпляр
