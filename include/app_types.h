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
    float temp_c = 0.0f;
    String description = "";
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

    SensorSnapshot sensor = {};
    TimeSnapshot time = {};
    WeatherSnapshot weather = {};
    TouchPoint touch = {};
};

