#pragma once

#include <Wire.h>

#include "core/app_types.h"

bool bme680_driver_init(TwoWire &wire);
void bme680_driver_run(uint32_t now_ms);
bool bme680_driver_read(Bme680Snapshot &snapshot, float reference_pressure_hpa, uint32_t now_ms);
uint8_t bme680_driver_address();
