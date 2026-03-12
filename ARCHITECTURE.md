# Architecture Overview

This project is split into small C-style modules around a shared `AppState`.

## Module Map

```text
main.cpp
  -> app.cpp (orchestrator)
      -> display_manager.cpp   (SPI LCD, LVGL init/flush/tick, backlight, sleep)
      -> touch_manager.cpp     (FT6336 init, polling, LVGL input callback)
      -> sensor_service.cpp    (SHT41 init/read)
      -> network_service.cpp   (WiFi connect, NTP sync/read, weather fetch)
      -> ui_main.cpp           (LVGL screen creation + UI label updates)

Shared types/config:
  include/app_types.h          (AppState + snapshots)
  include/app_config.h         (pins, intervals, MSPI safety checks)
```

## State Ownership

- `AppState` is the single shared runtime state (`wifi_connected`, `touch_available`, `display_sleeping`, timers, snapshots).
- Service modules update snapshots in `AppState`.
- UI module only renders snapshot data; it does not read sensors or network directly.

## Data Flow

### Setup Flow (`app_setup`)

1. Init display + LVGL.
2. Init sensor and touch.
3. Build UI.
4. Read initial sensor snapshot and render.
5. Connect WiFi; if connected:
   - sync time
   - read time snapshot and render
   - fetch weather snapshot and render
6. Save `last_activity_ms`.

### Loop Flow (`app_loop`)

1. Poll touch (`touch_poll`).
2. If touch is pressed:
   - update `last_activity_ms`
   - wake display if sleeping.
3. Run LVGL handler (`display_lvgl_handle`).
4. Sleep policy:
   - if inactivity exceeds `APP_DISPLAY_SLEEP_TIMEOUT_MS`, put display to sleep.
5. Periodic jobs:
   - sensor update every `APP_SENSOR_UPDATE_INTERVAL_MS`
   - time update every `APP_TIME_UPDATE_INTERVAL_MS` (when WiFi is connected)
   - weather update every `APP_WEATHER_UPDATE_INTERVAL_MS` (when WiFi is connected)
6. `delay(1)`.

## Design Rules

- Keep behavior stable first; prefer safe cleanup over logic rewrites.
- Keep module boundaries strict:
  - hardware/service modules do IO
  - `app.cpp` schedules and coordinates
  - `ui_main.cpp` only draws data it receives
- Keep pin and timing constants in `app_config.h`.

