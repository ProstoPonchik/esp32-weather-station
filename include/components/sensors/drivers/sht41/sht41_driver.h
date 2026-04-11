#pragma once

#include <Wire.h>

#include "core/app_types.h"

bool sht41_driver_init(TwoWire &wire);
bool sht41_driver_read(Sht41Snapshot &snapshot);
