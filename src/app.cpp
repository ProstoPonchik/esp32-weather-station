#include "app.h"

#include <Arduino.h>

#include "app_config.h"
#include "app_types.h"
#include "display_manager.h"
#include "network_service.h"
#include "sensor_service.h"
#include "touch_manager.h"
#include "ui_main.h"

static AppState s_state;

void app_setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1000);

    Serial.println("\n=== ESP32-S3 Weather Station ===\n");

    display_init(s_state, touch_lvgl_read_cb);
    sensor_init(s_state);
    touch_init(s_state);
    ui_init(s_state);

    sensor_read(s_state, s_state.sensor);
    ui_update_sensor(s_state.sensor);

    network_connect_wifi(s_state);
    if (network_is_connected(s_state)) {
        network_sync_time();
        if (network_read_time(s_state.time)) {
            ui_update_time(s_state.time);
        }

        weather_update(s_state);
        ui_update_weather(s_state.weather);
    }

    s_state.last_activity_ms = millis();

    Serial.println("\n=== Setup Complete! ===\n");
}

void app_loop()
{
    uint32_t now = millis();

    touch_poll(s_state, s_state.touch);
    if (s_state.touch.pressed) {
        s_state.last_activity_ms = now;
        if (s_state.display_sleeping) {
            display_set_sleep(s_state, false);
        }
    }

    display_lvgl_handle();

    if (!s_state.display_sleeping && (now - s_state.last_activity_ms >= APP_DISPLAY_SLEEP_TIMEOUT_MS)) {
        display_set_sleep(s_state, true);
    }

    if (now - s_state.last_sensor_update_ms > APP_SENSOR_UPDATE_INTERVAL_MS) {
        s_state.last_sensor_update_ms = now;
        sensor_read(s_state, s_state.sensor);
        ui_update_sensor(s_state.sensor);
    }

    if (s_state.wifi_connected && now - s_state.last_time_update_ms > APP_TIME_UPDATE_INTERVAL_MS) {
        s_state.last_time_update_ms = now;
        if (network_read_time(s_state.time)) {
            ui_update_time(s_state.time);
        }
    }

    if (s_state.wifi_connected && now - s_state.weather.updated_ms > APP_WEATHER_UPDATE_INTERVAL_MS) {
        weather_update(s_state);
        ui_update_weather(s_state.weather);
    }

    if (now - s_state.last_debug_log_ms >= 5000) {
        s_state.last_debug_log_ms = now;
        Serial.printf("[App] alive=%lus wifi=%d weather_valid=%d weather_age=%lus\n",
                      static_cast<unsigned long>(now / 1000UL),
                      s_state.wifi_connected ? 1 : 0,
                      s_state.weather.valid ? 1 : 0,
                      static_cast<unsigned long>((now - s_state.weather.updated_ms) / 1000UL));
    }

    delay(0);
}
