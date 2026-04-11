#pragma once

#include <Arduino.h>
#include <time.h>

enum class ClimateSource : uint8_t {
    None = 0,
    Sht41,
    Bme680,
};

struct Sht41Snapshot {
    float temp_c = 0.0f;
    float humidity_pct = 0.0f;
    bool valid = false;
    uint32_t updated_ms = 0;
};

struct Bme680Snapshot {
    float temp_c = 0.0f;
    float humidity_pct = 0.0f;
    float pressure_hpa = 0.0f;
    float gas_resistance_ohm = 0.0f;
    bool valid = false;
    uint32_t updated_ms = 0;
};

struct PrimaryClimateSnapshot {
    float temp_c = 0.0f;
    float humidity_pct = 0.0f;
    ClimateSource source = ClimateSource::None;
    bool valid = false;
    uint32_t updated_ms = 0;
};

struct SensorAggregate {
    Sht41Snapshot sht41 = {};
    Bme680Snapshot bme680 = {};
    PrimaryClimateSnapshot primary = {};
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
    bool bme680_available = false;
    bool touch_available = false;
    bool display_sleeping = false;

    uint32_t last_activity_ms = 0;
    uint32_t last_sensor_update_ms = 0;
    uint32_t last_time_update_ms = 0;
    uint32_t last_debug_log_ms = 0;

    SensorAggregate sensors = {};
    TimeSnapshot time = {};
    WeatherSnapshot weather = {};
    TouchPoint touch = {};
};
