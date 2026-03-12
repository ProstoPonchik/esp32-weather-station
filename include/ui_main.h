#pragma once

#include "app_types.h"

void ui_init(AppState &state);
void ui_update_sensor(const SensorSnapshot &snapshot);
void ui_update_time(const TimeSnapshot &snapshot);
void ui_update_weather(const WeatherSnapshot &snapshot);

