#pragma once
// =====================================================================
//  Config.h — все константы проекта «Такси Шашка»
//  Умный ночник на ESP32 + WS2812B
// =====================================================================

// ---------- Версия прошивки ----------
#define FIRMWARE_VERSION      "1.5.1"


// ---------- Светодиодная лента ----------
#define LED_PIN               5          // GPIO05 — вывод данных WS2812B
#define LED_COUNT             34         // количество светодиодов в шашке
#define LED_TYPE              WS2812B
#define LED_COLOR_ORDER       GRB
#define LED_MAX_MILLIAMPS     2000       // ограничение по току блока питания

// ---------- Сенсорная кнопка ----------
#define USE_TTP223            true       // (true) если использовать TTP223 
#define BUTTON_PIN            4          // Пин для TTP223
#define TOUCH_PIN             T0         // GPIO4 (touch-канал T0)
#define TOUCH_THRESHOLD       40         // порог срабатывания (меньше = касание)

// ---------- Измерение аккумулятора ----------
#define BATTERY_PIN           34         // GPIO34 (только ADC)
#define BATTERY_DIVIDER_RATIO 2.0f       // делитель 100к / 100к
#define BATTERY_SAMPLES       16         // усреднение замеров
#define BATTERY_MIN_VOLTAGE   3.2f       // 0% (безопасный минимум Li-Ion)
#define BATTERY_MAX_VOLTAGE   4.2f       // 100% (полный заряд Li-Ion)
#define BATTERY_UPDATE_MS     10000UL    // период обновления, мс

// ---------- Точка доступа (AP) ----------
#define AP_SSID               "TaxiLight"
#define AP_PASSWORD           "12345678"
#define AP_IP_ADDR            192,168,4,1

// ---------- Wi-Fi ----------
#define WIFI_CONNECT_TIMEOUT  10000UL    // таймаут подключения к сети, мс
#define WIFI_RETRY_INTERVAL   30000UL    // период попыток переподключения
#define WIFI_MAX_RETRIES      3          // попыток перед переходом в AP

// ---------- OTA ----------
#define OTA_HOSTNAME          "taxilight"
#define OTA_PASSWORD          "taxilight_ota"
#define GITHUB_VERSION_URL    "https://raw.githubusercontent.com/YellowDoctor/Taxi_Light/main/version.json"

// ---------- Устройство по умолчанию ----------
#define DEVICE_DEFAULT_NAME   "Такси Шашка"

// ---------- Эффекты ----------
#define EFFECT_STATIC         0   // Статический цвет
#define EFFECT_BREATHE        1   // Дыхание
#define EFFECT_RAINBOW        2   // Радуга
#define EFFECT_FIRE           3   // Огонь
#define EFFECT_CANDLE         4   // Свеча
#define EFFECT_COLORFLOW      5   // Цветовой поток
#define EFFECT_AURORA         6   // Северное сияние
#define EFFECT_OCEAN          7   // Океан
#define EFFECT_POLICE         8   // Полиция: красный/синий
#define EFFECT_POLICE_FLASH   9   // Полицейская мигалка
#define EFFECT_METEOR         10  // Метеор (бегущий хвост)
#define EFFECT_STROBE         11  // Стробоскоп
#define EFFECT_RUNNING        12  // Бегущие огни (гирлянда)
#define EFFECT_NIGHTSKY       13  // Ночное небо (мерцающие звёзды)
#define EFFECT_LASER          14  // Лазер (сканирующая точка)
#define EFFECT_FLASH          15  // Случайные вспышки

#define EFFECT_COUNT          16

// ---------- Избранные сцены ----------
#define FAVORITES_COUNT       5
#define FAVORITES_NAME_LEN    16

// ---------- Расписание ----------
#define SCHEDULE_COUNT        8

// ---------- NTP ----------

#define NTP_SERVER1           "pool.ntp.org"
#define NTP_SERVER2           "time.cloudflare.com"

// ---------- Прочее ----------
#define SERIAL_BAUD           115200
#define STATUS_LOG_SIZE       20         // размер кольцевого лога событий
