# ESP32-S3 Weather Station

Modern touch-screen weather station with LVGL 8.4 interface, WiFi, NTP time sync, and OpenWeatherMap API integration.

![ESP32-S3](https://img.shields.io/badge/ESP32--S3-240MHz-blue)
![LVGL](https://img.shields.io/badge/LVGL-8.4.0-green)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ready-orange)

## Features

- **Modern UI**: Dark theme with card-based design, anti-aliased JetBrains Mono fonts
- **Three Screens**: Swipeable tileview (Home/Time/Weather)
- **Sensors**: SHT41 temperature & humidity sensor  
- **Touch**: FT6336U capacitive touch with swipe gestures
- **WiFi**: Automatic connection and NTP time synchronization
- **Weather**: OpenWeatherMap OneCall API 3.0 integration
- **Display**: ST7796 480x320 TFT @ 40MHz SPI

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

### I2C Sensors

**Bus 0 (SHT41 Sensor)**:
- SDA: GPIO 1
- SCL: GPIO 2
- Address: 0x44

**Bus 1 (Touch FT6336U)**:
- SDA: GPIO 40
- SCL: GPIO 41  
- RST: GPIO 39
- INT: GPIO 38
- Address: 0x38

## Configuration

### WiFi Settings

Edit in `src/main.cpp`:
```cpp
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"
```

### Weather API

Get free API key from [OpenWeatherMap](https://openweathermap.org/api/one-call-3):

```cpp
#define WEATHER_LAT     48.3064  // Your latitude
#define WEATHER_LON     14.2858  // Your longitude  
#define WEATHER_API_KEY "your_api_key_here"
```

## Build & Upload

Using PlatformIO:

```bash
# Build
pio run

# Upload
pio run -t upload

# Monitor
pio run -t monitor
```

## Dependencies

Managed automatically by PlatformIO (`platformio.ini`):

- **LVGL 8.4.0**: Graphics library
- **Adafruit SHT4x**: Temperature/humidity sensor
- **FT6X36**: Touch controller library
- **ArduinoJson**: Weather API parsing
- **WiFi & HTTPClient**: Network connectivity

## UI Screens

### Screen 1: Home (Sensors)
- Temperature card (°C) with real-time SHT41 readings
- Humidity card (%) with gradient display
- Card-based design with rounded corners

### Screen 2: Time
- Large 48px time display (HH:MM)
- Date with day of week
- NTP synchronized (GMT+1 + DST)

### Screen 3: Weather
- Current temperature from OpenWeatherMap
- Weather description (clouds, rain, etc.)
- Location display (Linz, Austria)
- Auto-update every 10 minutes

## Custom Fonts

Anti-aliased JetBrains Mono fonts included:
- **24px**: Labels, hints, descriptions
- **32px**: Reserved for future use
- **48px**: Large displays (time, temperature)

Fonts support: `0x20-0x7E` + degree symbol `°`

Generated with bpp=4 (16 grayscale levels)

## Performance

- **FPS**: 20-30 frames (optimized LVGL tick 1ms, buffer 80 lines)
- **Flash**: 89.4% (1172KB / 1310KB)
- **RAM**: 34.9% (114KB / 327KB)
- **PSRAM**: 8MB available

## Troubleshooting

### Display Issues
- Check SPI connections (MOSI, MISO, SCLK, CS, DC, RST)
- Verify 3.3V power supply
- Try reducing SPI frequency to 20MHz

### Touch Not Working
- Verify I2C Bus 1 connections (SDA=40, SCL=41)
- Check hardware reset pin (RST=39)
- Ensure FT6336U address 0x38 detected

### WiFi Connection Failed
- Check SSID and password in `main.cpp`
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check router MAC filtering

### Weather Not Updating
- Verify API key validity on OpenWeatherMap dashboard
- Check OneCall API 3.0 subscription (free tier available)
- Monitor serial output for HTTP error codes

## Serial Monitor Output

Expected output (115200 baud):
```
=== ESP32-S3 Weather Station ===

Initializing SPI bus...
ST7796 initialized! Resolution: 480x320
Initializing I2C buses...
Initializing SHT41 sensor... OK!
Initializing touch screen... OK!

[WiFi] Connected!
[WiFi] IP: 192.168.0.88
[NTP] Time synced!
[Weather] Temp: -3.9C, scattered clouds

=== Setup Complete! ===
```

## Future Enhancements

- [ ] BME680 air quality sensor integration
- [ ] Weather forecast (hourly/daily)
- [ ] Weather icons instead of text
- [ ] Customizable themes
- [ ] Settings screen with WiFi config
- [ ] Multiple location support
- [ ] Sunrise/sunset times

## License

MIT License - Free for personal and commercial use.
