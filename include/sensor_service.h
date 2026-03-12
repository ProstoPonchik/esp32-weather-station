#pragma once

#include "app_types.h"

void sensor_init(AppState &state);
void sensor_read(const AppState &state, SensorSnapshot &snapshot);

