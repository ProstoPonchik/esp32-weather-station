# Architecture Overview

This project uses a component-oriented layout with a central orchestrator (`app`) and shared runtime state (`AppState`).

## Module Map

```text
main.cpp
  -> app/app.cpp (setup/loop orchestration)
      -> components/display/*          (SPI LCD, LVGL init/flush/tick, sleep)
      -> components/touch/*            (FT6336 init, polling, LVGL pointer callback)
      -> components/network/*          (WiFi connect, NTP, OpenWeather API)
      -> components/sensors/sensor_manager/*
          -> drivers/sht41/*           (SHT41 init/read)
          -> drivers/bme680/*          (BME680 via BSEC2 + state persistence)
      -> ui/presenters/*               (tile creation + UI updates)
      -> ui/assets/*                   (fonts, weather icons)

Shared config/types:
  include/core/app_config.h
  include/core/app_types.h
```

## State Ownership

- `AppState` is the single source of runtime truth (`wifi_connected`, sensor availability flags, timers, snapshots).
- `SensorAggregate` stores:
  - `sht41`
  - `bme680` (raw + BSEC + derived)
  - `primary` climate snapshot (`temp/humidity + source`)
- `sensor_manager` updates sensor snapshots and promotes latest valid reading to `primary`.
- UI only renders snapshots and does not access hardware directly.

## Sensor Flow

### Setup

1. Init shared sensor I2C bus (`Wire`, GPIO1/GPIO2).
2. Init SHT41 driver.
3. Init BME680 BSEC2 driver with address fallback (`0x76` -> `0x77`).
4. Load BSEC state from NVS (if available).
5. Perform initial reads and render placeholders until first valid samples arrive.

### Loop

- `sensor_manager_tick()` is called every loop iteration.
- SHT41 uses its own interval (`APP_SENSOR_UPDATE_INTERVAL_MS`).
- BME680/BSEC2 runs by BSEC internal `next_call` timing (LP profile ~3s).
- BME680 publish gate is `APP_BME680_PUBLISH_INTERVAL_MS` (default 3000ms).
- BSEC state is periodically persisted to NVS and also saved when IAQ reaches stable accuracy.

## BME680 Metrics

BME680 snapshot contains:
- Raw: `temperature`, `humidity`, `pressure`, `gas_resistance`
- BSEC: `static_iaq`, `iaq_accuracy`, `iaq_valid`
- Derived: `altitude`

`altitude` is computed from pressure:
- reference pressure = weather pressure (if available), otherwise `APP_BME680_SEA_LEVEL_HPA_DEFAULT`.

## UI Tiles

- `HOME`: primary temperature/humidity + source marker.
- `TIME`: local time/date.
- `WEATHER`: current weather card.
- `FORECAST`: hourly/daily forecast rows.
- `SENSORS+`: SHT41-only panel.
- `BME680`: dedicated page with raw metrics, altitude, IAQ, and IAQ accuracy status.

## Design Rules

- Keep hardware IO inside component drivers/services.
- Keep scheduling decisions in `app/app.cpp` and `sensor_manager`.
- Keep rendering logic in UI presenter module.
- Prefer additive, explicit state fields over hidden globals.
