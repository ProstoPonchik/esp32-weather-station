# Architecture Overview

This project now uses a component-oriented layout with a central orchestrator (`app`) and a shared runtime state (`AppState`).

## Module Map

```text
main.cpp
  -> app/app.cpp (setup/loop orchestration)
      -> components/display/*          (SPI LCD, LVGL init/flush/tick, sleep)
      -> components/touch/*            (FT6336 init, polling, LVGL pointer callback)
      -> components/network/*          (WiFi connect, NTP, OpenWeather API)
      -> components/sensors/sensor_manager/*
          -> drivers/sht41/*           (SHT41 init/read)
          -> drivers/bme680/*          (BME680 init/read, forced mode)
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
  - `bme680`
  - `primary` climate snapshot (`temp/humidity + source`)
- `sensor_manager` updates sensor snapshots and promotes the last valid reading to `primary`.
- UI only renders snapshots and does not access hardware directly.

## Sensor Flow

### Setup

1. Init shared sensor I2C bus (`Wire`, GPIO1/GPIO2).
2. Init SHT41 driver.
3. Init BME680 driver with auto-address fallback (`0x76` -> `0x77`).
4. Perform initial reads of both sensors.
5. Promote latest valid reading to `primary`.

### Loop

- Every `APP_SENSOR_UPDATE_INTERVAL_MS`, `sensor_manager_tick` runs.
- If both sensors are available, reads are alternated:
  - tick N: SHT41
  - tick N+1: BME680
- If only one sensor is available, manager reads only that sensor.
- Home screen is fed from `primary`; Sensors+ screen shows each sensor separately.

## UI Tiles

- `HOME`: primary temperature/humidity + source marker.
- `TIME`: local time/date.
- `WEATHER`: current weather card.
- `FORECAST`: hourly/daily forecast rows.
- `SENSORS+`: dedicated SHT41 panel and BME680 panel (T/H/P/Gas raw).

## Design Rules

- Keep hardware IO inside component drivers/services.
- Keep scheduling decisions in `app/app.cpp`.
- Keep rendering logic in UI presenter module.
- Prefer additive changes to snapshots and config constants over hidden globals.
