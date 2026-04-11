# ESP32-S3 Weather Station

Touch-screen weather station on ESP32-S3 with LVGL UI, WiFi/NTP, OpenWeatherMap integration, SHT41 and BME680 (BSEC2).

## Features

- Component-oriented architecture (`app/core/components/ui`)
- Swipeable tiles: `Home`, `Time`, `Weather`, `Forecast`, `Sensors+`, `BME680`
- Dual local sensors:
  - SHT41: temperature + humidity
  - BME680 via BSEC2: raw `T/H/P/Gas` + `Static IAQ` + `IAQ Accuracy`
- Altitude calculation from pressure (weather-based reference pressure with fallback)
- BSEC2 state persistence in NVS for faster IAQ stabilization after reboot
- WiFi + NTP + OpenWeather OneCall 3.0
- Display auto sleep/wake on touch

## Hardware

### Display Pins (ST7796 480x320)

| Function | ESP32-S3 GPIO |
|----------|---------------|
| MOSI     | 11            |
| MISO     | 13            |
| SCLK     | 12            |
| CS       | 10            |
| DC       | 9             |
| RST      | 46            |
| BL/LED   | 42            |

### I2C

**Bus 0 (`Wire`)**
- SDA: GPIO 1
- SCL: GPIO 2
- SHT41: `0x44`
- BME680: fallback `0x76` -> `0x77`

**Bus 1 (`Wire1`) Touch FT6336U**
- SDA: GPIO 40
- SCL: GPIO 41
- RST: GPIO 39
- INT: GPIO 38

## Project Structure

```text
include/
  app/app.h
  core/{app_config.h, app_types.h}
  components/{display, touch, network, sensors}
  ui/{ui_main.h, assets/weather_icons.h}

src/
  main.cpp
  app/app.cpp
  components/
  ui/{presenters,assets,screens}

lib/
  Bosch-BME68x-Library/
  Bosch-BSEC2-Library/
```

## Configuration

Copy `src/config.h.example` to `src/config.h` and set values:

```cpp
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"

#define WEATHER_LAT     48.3064
#define WEATHER_LON     14.2858
#define WEATHER_API_KEY "your_api_key_here"

// Optional BME680/BSEC2
// #define BME680_BSEC_ENABLED                1
// #define BME680_PUBLISH_INTERVAL_MS         3000
// #define BME680_SEA_LEVEL_HPA_DEFAULT       1013.25f
// #define BME680_BSEC_STATE_SAVE_INTERVAL_MS 600000
// #define BME680_BSEC_CONFIG_28D             0
```

## Build

```bash
pio run
pio run -t upload
pio run -t monitor
```

## BME680 Page

Dedicated `BME680` tile displays:
- Temperature
- Humidity
- Pressure
- Gas resistance
- Altitude
- Static IAQ
- IAQ Accuracy / status

## Notes

- BSEC2 IAQ accuracy improves after warm-up and runtime stabilization.
- BSEC state is persisted in NVS to reduce recalibration time after reboot.

## Documentation

Detailed architecture and flow: [ARCHITECTURE.md](ARCHITECTURE.md)
