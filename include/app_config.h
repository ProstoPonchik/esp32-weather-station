#pragma once

#include <Arduino.h>
#include <driver/spi_master.h>

// Try to include local config file (ignored by git)
#if __has_include("config.h")
    #include "config.h"
#endif

// WiFi Configuration (override in config.h)
#ifndef WIFI_SSID
    #define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
    #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

// Weather Configuration (override in config.h)
#ifndef WEATHER_LAT
    #define WEATHER_LAT 48.3064
#endif
#ifndef WEATHER_LON
    #define WEATHER_LON 14.2858
#endif
#ifndef WEATHER_API_KEY
    #define WEATHER_API_KEY "YOUR_API_KEY_HERE"
#endif

constexpr const char *APP_WIFI_SSID = WIFI_SSID;
constexpr const char *APP_WIFI_PASSWORD = WIFI_PASSWORD;
constexpr float APP_WEATHER_LAT = WEATHER_LAT;
constexpr float APP_WEATHER_LON = WEATHER_LON;
constexpr const char *APP_WEATHER_API_KEY = WEATHER_API_KEY;

// Display Pins
constexpr int APP_LCD_MOSI = 11;
constexpr int APP_LCD_MISO = 13;
constexpr int APP_LCD_SCLK = 12;
constexpr int APP_LCD_CS = 10;
constexpr int APP_LCD_DC = 9;
constexpr int APP_LCD_RST = 46;
constexpr int APP_LCD_BL = 42;
constexpr int APP_LCD_BL_ON_LEVEL = HIGH;

// SHT41 I2C (Wire)
constexpr int APP_SHT41_SDA = 1;
constexpr int APP_SHT41_SCL = 2;

// FT6336U touch I2C (Wire1)
constexpr int APP_TOUCH_SDA = 40;
constexpr int APP_TOUCH_SCL = 41;
constexpr int APP_TOUCH_RST = 39;
constexpr int APP_TOUCH_INT = 38;

// FT6336U registers
constexpr uint8_t APP_FT6336_ADDR = 0x38;
constexpr uint8_t APP_FT6336_REG_NUM_TOUCHES = 0x02;
constexpr uint8_t APP_FT6336_REG_P1_XH = 0x03;

// Display Configuration
constexpr int APP_LCD_H_RES = 480;
constexpr int APP_LCD_V_RES = 320;
constexpr spi_host_device_t APP_LCD_SPI_HOST = SPI2_HOST;
constexpr uint32_t APP_LCD_PIXEL_CLK = 60000000;
constexpr bool APP_LCD_COLOR_ORDER_BGR = true;
constexpr bool APP_LCD_INVERT_COLORS = true;

// Timing
constexpr uint32_t APP_LVGL_TICK_PERIOD_MS = 1;
constexpr uint32_t APP_LVGL_BUFFER_SIZE = APP_LCD_H_RES * 80;
constexpr uint32_t APP_DISPLAY_SLEEP_TIMEOUT_MS = 60000;
constexpr uint32_t APP_SENSOR_UPDATE_INTERVAL_MS = 2000;
constexpr uint32_t APP_TIME_UPDATE_INTERVAL_MS = 1000;
constexpr uint32_t APP_WEATHER_UPDATE_INTERVAL_MS = 600000;
constexpr uint32_t APP_I2C_TIMEOUT_MS = 100;
constexpr uint32_t APP_TOUCH_I2C_FREQ_HZ = 400000;
constexpr uint32_t APP_TOUCH_POLL_INTERVAL_MS = 2;
constexpr uint8_t APP_TOUCH_SMOOTHING_SHIFT = 0;
constexpr uint8_t APP_TOUCH_SCROLL_LIMIT_PX = 2;
constexpr uint8_t APP_TOUCH_SCROLL_THROW = 4;
constexpr uint8_t APP_TOUCH_GESTURE_LIMIT_PX = 8;
constexpr uint8_t APP_TOUCH_GESTURE_MIN_VELOCITY = 1;

// ESP32-S3 MSPI pins are reserved for flash/PSRAM and must not be reused.
#if defined(CONFIG_SPIRAM_MODE_OCT)
    #define APP_IS_MSPI_RESERVED_GPIO(gpio) ((gpio) >= 26 && (gpio) <= 37)
#else
    #define APP_IS_MSPI_RESERVED_GPIO(gpio) ((gpio) >= 26 && (gpio) <= 32)
#endif

#define APP_ASSERT_GPIO_NOT_MSPI(pin_name) \
    static_assert(!(APP_IS_MSPI_RESERVED_GPIO(pin_name)), #pin_name " uses reserved MSPI GPIO.")

APP_ASSERT_GPIO_NOT_MSPI(APP_LCD_MOSI);
APP_ASSERT_GPIO_NOT_MSPI(APP_LCD_MISO);
APP_ASSERT_GPIO_NOT_MSPI(APP_LCD_SCLK);
APP_ASSERT_GPIO_NOT_MSPI(APP_LCD_CS);
APP_ASSERT_GPIO_NOT_MSPI(APP_LCD_DC);
APP_ASSERT_GPIO_NOT_MSPI(APP_LCD_RST);
APP_ASSERT_GPIO_NOT_MSPI(APP_LCD_BL);
APP_ASSERT_GPIO_NOT_MSPI(APP_SHT41_SDA);
APP_ASSERT_GPIO_NOT_MSPI(APP_SHT41_SCL);
APP_ASSERT_GPIO_NOT_MSPI(APP_TOUCH_SDA);
APP_ASSERT_GPIO_NOT_MSPI(APP_TOUCH_SCL);
APP_ASSERT_GPIO_NOT_MSPI(APP_TOUCH_RST);
APP_ASSERT_GPIO_NOT_MSPI(APP_TOUCH_INT);

#undef APP_ASSERT_GPIO_NOT_MSPI
#undef APP_IS_MSPI_RESERVED_GPIO
