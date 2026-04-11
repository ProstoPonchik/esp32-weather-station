# ESP32-S3 Weather Station

Touch-screen weather station on ESP32-S3 with LVGL UI, WiFi/NTP, OpenWeatherMap integration, and dual local sensors (SHT41 + BME680).

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz-blue)
![LVGL](https://img.shields.io/badge/LVGL-8.4.0-green)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ready-orange)

## Features

- Component-oriented architecture (`app/core/components/ui`)
- 5 swipeable tiles: `Home`, `Time`, `Weather`, `Forecast`, `Sensors+`
- Dual sensor stack on shared I2C bus:
  - SHT41: temperature + humidity
  - BME680: temperature + humidity + pressure + gas resistance (raw)
- Sensor manager with alternating polling (`SHT41` / `BME680`)
- Home screen shows latest valid primary climate reading source (`SHT41` or `BME680`)
- WiFi + NTP time sync + OpenWeatherMap OneCall 3.0
- Auto display sleep/wake by touch activity

## Hardware

### Display Pins (ST7796 - 480x320)

| Function | ESP32-S3 GPIO |
|----------|---------------|
| MOSI     | 11            |
| MISO     | 13            |
| SCLK     | 12            |
| CS       | 10            |
| DC       | 9             |
| RST      | 46            |
| BL/LED   | 42            |

### I2C Sensors

**Bus 0 (`Wire`)**
- SDA: GPIO 1
- SCL: GPIO 2
- SHT41 address: `0x44`
- BME680 address: auto fallback `0x76` -> `0x77`

**Bus 1 (`Wire1`) Touch FT6336U**
- SDA: GPIO 40
- SCL: GPIO 41
- RST: GPIO 39
- INT: GPIO 38
- Address: `0x38`

## Project Structure

```text
include/
  app/app.h
  core/{app_config.h, app_types.h}
  components/
    display/
    touch/
    network/
    sensors/
      sensor_manager/
      drivers/{sht41,bme680}/
  ui/
    ui_main.h
    assets/weather_icons.h

src/
  main.cpp
  app/app.cpp
  components/
  ui/{presenters,assets,screens}

lib/
  Bosch-BME68x-Library/
```

## Configuration

Copy `src/config.h.example` to `src/config.h` and set your values:

```cpp
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"

#define WEATHER_LAT     48.3064
#define WEATHER_LON     14.2858
#define WEATHER_API_KEY "your_api_key_here"

// Optional BME680 overrides
// #define BME680_ENABLED            1
// #define BME680_I2C_ADDR_PRIMARY   0x76
// #define BME680_I2C_ADDR_SECONDARY 0x77
```

Main hardware/timing constants are in `include/core/app_config.h`.

## Build & Upload

```bash
pio run
pio run -t upload
pio run -t monitor
```

## Sensors+ Tile

The new `Sensors+` page contains two separate panels:

- `SHT41`: temperature/humidity + availability status
- `BME680`: temperature/humidity/pressure/gas raw + availability status

If a sensor is missing or invalid, UI shows placeholders (`--`) and availability state.

## Dependencies

Managed via PlatformIO:

- `lvgl/lvgl`
- `Adafruit SHT4x`
- `FT6X36`
- `ArduinoJson`
- Local vendor lib: `lib/Bosch-BME68x-Library`

## Architecture Notes

Detailed flow and ownership docs: [ARCHITECTURE.md](ARCHITECTURE.md)

## License

MIT License.
