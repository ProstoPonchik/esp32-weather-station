#pragma once

#include "core/app_types.h"

void sensor_manager_init(AppState &state);
void sensor_manager_read_initial(AppState &state);
void sensor_manager_tick(AppState &state);
const SensorAggregate &sensor_manager_get(const AppState &state);
