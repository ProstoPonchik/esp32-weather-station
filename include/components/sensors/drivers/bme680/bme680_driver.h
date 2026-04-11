#pragma once

#include <Wire.h>

#include "core/app_types.h"

bool bme680_driver_init(TwoWire &wire);
bool bme680_driver_read(Bme680Snapshot &snapshot);
uint8_t bme680_driver_address();
