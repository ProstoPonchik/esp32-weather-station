#pragma once

#include <Arduino.h>
#include <time.h>

struct SensorSnapshot {
    float temp_c = 0.0f;
    float humidity_pct = 0.0f;
    bool valid = false;
};

struct TimeSnapshot {
    tm local = {};
    bool valid = false;
};

struct WeatherSnapshot {
    struct HourlyPoint {
        String time_label = "";
        float temp_c = 0.0f;
        String icon_code = "";
        bool valid = false;
    };

    struct DailyPoint {
        String day_label = "";
        float min_c = 0.0f;
        float max_c = 0.0f;
        String icon_code = "";
        bool valid = false;
    };

    float temp_c = 0.0f;
    String description = "";
    String icon_code = "";
    String sunrise = "";
    String sunset = "";
    HourlyPoint hourly[3] = {};
    DailyPoint daily[3] = {};
    bool valid = false;
    uint32_t updated_ms = 0;
};

struct TouchPoint {
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = false;
    bool just_pressed = false;
};

struct AppState {
    bool wifi_connected = false;
    bool sht41_available = false;
    bool touch_available = false;
    bool display_sleeping = false;

    uint32_t last_activity_ms = 0;
    uint32_t last_sensor_update_ms = 0;
    uint32_t last_time_update_ms = 0;
    uint32_t last_debug_log_ms = 0;

    SensorSnapshot sensor = {};
    TimeSnapshot time = {};
    WeatherSnapshot weather = {};
    TouchPoint touch = {};
};
