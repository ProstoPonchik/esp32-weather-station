#pragma once

#include "core/app_types.h"

void network_connect_wifi(AppState &state);
bool network_is_connected(const AppState &state);
void network_sync_time();
bool network_read_time(TimeSnapshot &snapshot);
void weather_update(AppState &state);
