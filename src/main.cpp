/*
 * ST7796 480x320 Display with LVGL using ESP LCD Panel API
 * ESP32-S3 DevKit + SHT41 Temperature/Humidity Sensor
 * 
 * Display Pins:
 * MOSI = 11, MISO = 13, SCK = 12
 * CS = 10, DC = 9, RST = 46
 * 
 * I2C Pins (two separate buses):
 * SHT41: SDA = 1, SCL = 2 (Wire)
 * FT6336U Touch: SDA = 40, SCL = 41 (Wire1)
 * SHT41 Address = 0x44
 * FT6336U Address = 0x38
 */

#include <Arduino.h>
#include <lvgl.h>
#include <hal/lv_hal_tick.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <driver/spi_master.h>
#include <esp_timer.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <FT6X36.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Try to include local config file (ignored by git)
#if __has_include("config.h")
    #include "config.h"
#endif

// External fonts
LV_FONT_DECLARE(lv_font_jb_24);
LV_FONT_DECLARE(lv_font_jb_32);
LV_FONT_DECLARE(lv_font_jb_48);

// Pin Configuration
#define LCD_MOSI        11
#define LCD_MISO        13
#define LCD_SCLK        12
#define LCD_CS          10
#define LCD_DC          9
#define LCD_RST         46
#define LCD_BL          -1

// I2C Configuration
// SHT41 on I2C Bus 0 (Wire)
#define SHT41_SDA       1
#define SHT41_SCL       2

// Touch on I2C Bus 1 (second bus)
#define TOUCH_SDA       40
#define TOUCH_SCL       41
#define TOUCH_RST       39  // Hardware reset - required!
#define TOUCH_INT       38  // Interrupt pin

// WiFi Configuration (override in config.h)
#ifndef WIFI_SSID
    #define WIFI_SSID       "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
    #define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
#endif

// Weather Configuration (override in config.h)
#ifndef WEATHER_LAT
    #define WEATHER_LAT     48.3064
#endif
#ifndef WEATHER_LON
    #define WEATHER_LON     14.2858
#endif
#ifndef WEATHER_API_KEY
    #define WEATHER_API_KEY "YOUR_API_KEY_HERE"  // Get free key from openweathermap.org
#endif
#define WEATHER_UPDATE_INTERVAL 600000  // 10 minutes

// FT6336U Touch Screen I2C
#define FT6336_ADDR     0x38
#define FT6336_REG_NUM_TOUCHES  0x02
#define FT6336_REG_P1_XH        0x03

// Display Configuration
#define LCD_H_RES       480
#define LCD_V_RES       320
#define LCD_SPI_NUM     SPI2_HOST
#define LCD_PIXEL_CLK   40000000

// LVGL Configuration
#define LVGL_TICK_PERIOD_MS 1
#define LVGL_BUFFER_SIZE    (LCD_H_RES * 80)

// Global objects
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

// SHT41 sensor
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
bool sht41_available = false;

// Second I2C bus for touch
TwoWire I2C_Touch = TwoWire(1);

// Touch screen with FT6X36
FT6X36 ts(&I2C_Touch, TOUCH_INT);
bool touch_available = false;

// Touch data from callback
static volatile uint16_t touch_x = 0;
static volatile uint16_t touch_y = 0;
static volatile bool touch_pressed = false;

// UI objects
lv_obj_t *temp_label = NULL;
lv_obj_t *humi_label = NULL;
lv_obj_t *status_label = NULL;
lv_obj_t *tileview = NULL;

// Screen objects
lv_obj_t *time_label = NULL;
lv_obj_t *date_label = NULL;
lv_obj_t *weather_temp_label = NULL;
lv_obj_t *weather_desc_label = NULL;
lv_obj_t *weather_city_label = NULL;

// WiFi and Time
bool wifi_connected = false;
struct tm timeinfo;

// Weather data
String weather_description = "";
float weather_temp = 0.0;
unsigned long lastWeatherUpdate = 0;

// Forward declarations
void createTestUI();
void updateSensorData();

// WiFi connection
void connectWiFi() {
    Serial.println("\n[WiFi] Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        Serial.println("\n[WiFi] Connected!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
    } else {
        wifi_connected = false;
        Serial.println("\n[WiFi] Connection failed!");
    }
}

// Time synchronization
void syncTime() {
    Serial.println("[NTP] Syncing time...");
    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");  // GMT+1 + DST for Linz
    
    int attempts = 0;
    while (!getLocalTime(&timeinfo) && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (attempts < 10) {
        Serial.println("\n[NTP] Time synced!");
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[NTP] Current time: %s\n", timeStr);
    } else {
        Serial.println("\n[NTP] Time sync failed!");
    }
}

// Weather update
void updateWeather() {
    if (!wifi_connected) return;
    
    HTTPClient http;
    // OneCall API 3.0 endpoint
    String url = "https://api.openweathermap.org/data/3.0/onecall?lat=" + String(WEATHER_LAT, 4) + 
                 "&lon=" + String(WEATHER_LON, 4) + 
                 "&units=metric&exclude=minutely,hourly,daily,alerts&appid=" + String(WEATHER_API_KEY);
    
    Serial.println("[Weather] Fetching weather...");
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            weather_temp = doc["current"]["temp"];
            weather_description = doc["current"]["weather"][0]["description"].as<String>();
            
            Serial.printf("[Weather] Temp: %.1fC, %s\n", weather_temp, weather_description.c_str());
            lastWeatherUpdate = millis();
        } else {
            Serial.printf("[Weather] JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.printf("[Weather] HTTP error: %d\n", httpCode);
        if (httpCode == 401) {
            Serial.println("[Weather] API key might be invalid or inactive");
        }
    }
    
    http.end();
}

// Touch read callback for LVGL
static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    if (!touch_available) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    
    if (touch_pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        // Map touch coordinates to landscape mode (480x320)
        data->point.x = touch_y;
        data->point.y = LCD_V_RES - touch_x;
        
        // Serial.printf("LVGL Touch: Raw(%d,%d) -> Screen(%d,%d)\n", 
        //              touch_x, touch_y, data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, 
                                    esp_lcd_panel_io_event_data_t *edata, 
                                    void *user_ctx);
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);

// Notify LVGL that flush is complete
static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, 
                                    esp_lcd_panel_io_event_data_t *edata, 
                                    void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

// LVGL flush callback
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, 
                             offsetx2 + 1, offsety2 + 1, color_map);
}

// LVGL tick timer
static void lv_tick_task(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n=== ESP32-S3 Weather Station ===\n");
    
    // Initialize SPI Bus
    Serial.println("Initializing SPI bus...");
    
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_MOSI,
        .miso_io_num = LCD_MISO,
        .sclk_io_num = LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LVGL_BUFFER_SIZE * sizeof(uint16_t)
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO));
    
    // Initialize LCD Panel IO
    Serial.println("Initializing LCD panel IO...");
    
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.dc_gpio_num = LCD_DC;
    io_config.cs_gpio_num = LCD_CS;
    io_config.pclk_hz = LCD_PIXEL_CLK;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.spi_mode = 0;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = notify_lvgl_flush_ready;
    io_config.user_ctx = &disp_drv;
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_NUM, 
                                             &io_config, &io_handle));
    
    // Initialize ST7796 Panel
    Serial.println("Initializing ST7789/ST7796 panel...");
    
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    Serial.printf("ST7796 initialized! Resolution: %dx%d\n", LCD_H_RES, LCD_V_RES);
    
    // Initialize LVGL
    Serial.println("Initializing LVGL...");
    lv_init();
    
    buf1 = (lv_color_t *)heap_caps_malloc(LVGL_BUFFER_SIZE * sizeof(lv_color_t), 
                                          MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    buf2 = (lv_color_t *)heap_caps_malloc(LVGL_BUFFER_SIZE * sizeof(lv_color_t), 
                                          MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    
    if (!buf1 || !buf2) {
        Serial.println("ERROR: Failed to allocate LVGL buffers!");
        while (1) delay(1000);
    }
    
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LVGL_BUFFER_SIZE);
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Setup LVGL Tick Timer
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };
    
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LVGL_TICK_PERIOD_MS * 1000));
    
    // Initialize I2C Bus 0 (SHT41)
    Serial.println("\n=== Initializing I2C Bus 0 (SHT41) ===");
    Serial.printf("Pins: SDA=%d, SCL=%d\n", SHT41_SDA, SHT41_SCL);
    Wire.begin(SHT41_SDA, SHT41_SCL);
    Wire.setTimeout(100);
    
    // Initialize I2C Bus 1 (Touch)
    Serial.println("=== Initializing I2C Bus 1 (Touch) ===");
    Serial.printf("Pins: SDA=%d, SCL=%d\n", TOUCH_SDA, TOUCH_SCL);
    I2C_Touch.begin(TOUCH_SDA, TOUCH_SCL, 100000);
    I2C_Touch.setTimeout(100);
    
    // Initialize SHT41 Sensor
    Serial.println("Initializing SHT41 sensor...");
    Serial.flush();  // Force output
    if (!sht4.begin()) {
        Serial.println(" not found!");
        sht41_available = false;
    } else {
        Serial.println(" OK!");
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
        sht41_available = true;
    }
    
    // Initialize Touch Screen
    Serial.println("=== Touch Screen Initialization ===");
    
    // Hardware reset
    pinMode(TOUCH_RST, OUTPUT);
    digitalWrite(TOUCH_RST, LOW);
    delay(10);
    digitalWrite(TOUCH_RST, HIGH);
    delay(200);
    
    // Check device
    I2C_Touch.beginTransmission(0x38);
    uint8_t error = I2C_Touch.endTransmission();
    Serial.printf("Result: error=%d (0=OK, 2=NACK)\n", error);
    Serial.flush();
    
    if (error == 0 && ts.begin()) {
        touch_available = true;
        Serial.println(" OK!");
    } else {
        touch_available = false;
        Serial.println(" failed!");
    }
    
    // Register touch input device with LVGL
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);
    
    // Create UI
    createTestUI();
    
    // ========== Connect WiFi ==========
    connectWiFi();
    
    // ========== Sync Time ==========
    if (wifi_connected) {
        syncTime();
        updateWeather();
        
        if (weather_temp_label && weather_desc_label) {
            char tempStr[16];
            snprintf(tempStr, sizeof(tempStr), "%.1f°", weather_temp);
            lv_label_set_text(weather_temp_label, tempStr);
            lv_label_set_text(weather_desc_label, weather_description.c_str());
        }
    }
    
    Serial.println("\n=== Setup Complete! ===\n");
}

void loop()
{
    static unsigned long lastUpdate = 0;
    static unsigned long lastTimeUpdate = 0;
    
    // Process touch events
    if (touch_available) {
        // Direct polling - check number of touches
        uint8_t touches = ts.touched();
        
        if (touches > 0) {
            // Read touch data directly from I2C registers (bypass callback)
            uint8_t data[16];
            I2C_Touch.beginTransmission(0x38);
            I2C_Touch.write(0);  // Start from register 0
            I2C_Touch.endTransmission();
            
            I2C_Touch.requestFrom(0x38, 16);
            for (int i = 0; i < 16 && I2C_Touch.available(); i++) {
                data[i] = I2C_Touch.read();
            }
            
            // Parse touch coordinates (FT6336U format)
            uint16_t raw_x = ((data[0x03] & 0x0F) << 8) | data[0x04];
            uint16_t raw_y = ((data[0x05] & 0x0F) << 8) | data[0x06];
            
            if (raw_x > 0 || raw_y > 0) {
                touch_x = raw_x;
                touch_y = raw_y;
                touch_pressed = true;
                // Serial.printf("[MANUAL READ] touches=%d, raw_x=%d, raw_y=%d\n", touches, raw_x, raw_y);
            }
            
            ts.processTouch();
        } else {
            touch_pressed = false;
        }
        
        ts.loop();
    }
    
    lv_timer_handler();
    
    // Update sensor data every 2 seconds
    if (millis() - lastUpdate > 2000) {
        lastUpdate = millis();
        updateSensorData();
    }
    
    // Update time display every second
    if (wifi_connected && time_label && millis() - lastTimeUpdate > 1000) {
        lastTimeUpdate = millis();
        if (getLocalTime(&timeinfo)) {
            char timeStr[16];
            char dateStr[48];
            strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
            strftime(dateStr, sizeof(dateStr), "%A, %B %e", &timeinfo);
            lv_label_set_text(time_label, timeStr);
            lv_label_set_text(date_label, dateStr);
        }
    }
    
    // Update weather every 10 minutes
    if (wifi_connected && millis() - lastWeatherUpdate > WEATHER_UPDATE_INTERVAL) {
        updateWeather();
        
        if (weather_temp_label && weather_desc_label) {
            char tempStr[16];
            snprintf(tempStr, sizeof(tempStr), "%.1f°", weather_temp);
            lv_label_set_text(weather_temp_label, tempStr);
            lv_label_set_text(weather_desc_label, weather_description.c_str());
        }
    }
    
    delay(1);
}

// ========== Update Sensor Data ==========
void updateSensorData()
{
    if (!temp_label || !humi_label) return;
    
    if (!sht41_available) {
        lv_label_set_text(temp_label, "--");
        lv_label_set_text(humi_label, "--");
        return;
    }
    
    sensors_event_t humidity, temp;
    if (sht4.getEvent(&humidity, &temp)) {
        char temp_str[16];
        char humi_str[16];
        
        snprintf(temp_str, sizeof(temp_str), "%.1f", temp.temperature);
        snprintf(humi_str, sizeof(humi_str), "%.0f", humidity.relative_humidity);
        
        lv_label_set_text(temp_label, temp_str);
        lv_label_set_text(humi_label, humi_str);
    } else {
        lv_label_set_text(temp_label, "--");
        lv_label_set_text(humi_label, "--");
    }
}

// ========== Create Modern UI ==========
void createTestUI()
{
    Serial.println("Creating modern tileview UI with 3 screens...");
    
    // Dark theme background
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0A0E1A), 0);
    
    // Create tileview
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(tileview, LCD_H_RES, LCD_V_RES);
    lv_obj_set_style_bg_color(tileview, lv_color_hex(0x0A0E1A), 0);
    lv_obj_set_style_border_width(tileview, 0, 0);
    
    // ========== Screen 1: Sensor Data ==========
    lv_obj_t *tile1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_RIGHT);
    lv_obj_set_style_bg_color(tile1, lv_color_hex(0x0A0E1A), 0);
    
    // Title
    lv_obj_t *title1 = lv_label_create(tile1);
    lv_label_set_text(title1, "HOME");
    lv_obj_set_style_text_font(title1, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title1, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title1, LV_ALIGN_TOP_MID, 0, 15);
    
    // Temperature card
    lv_obj_t *temp_card = lv_obj_create(tile1);
    lv_obj_set_size(temp_card, 200, 110);
    lv_obj_set_style_bg_color(temp_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(temp_card, 15, 0);
    lv_obj_set_style_border_width(temp_card, 0, 0);
    lv_obj_set_style_pad_all(temp_card, 0, 0);
    lv_obj_align(temp_card, LV_ALIGN_CENTER, -115, -30);
    
    lv_obj_t *temp_icon = lv_label_create(temp_card);
    lv_label_set_text(temp_icon, "°C");
    lv_obj_set_style_text_font(temp_icon, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(temp_icon, lv_color_hex(0xFF6B6B), 0);
    lv_obj_align(temp_icon, LV_ALIGN_TOP_LEFT, 15, 15);
    
    temp_label = lv_label_create(temp_card);
    lv_label_set_text(temp_label, "--");
    lv_obj_set_style_text_font(temp_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(temp_label, LV_ALIGN_BOTTOM_LEFT, 15, -10);
    
    // Humidity card
    lv_obj_t *humi_card = lv_obj_create(tile1);
    lv_obj_set_size(humi_card, 200, 110);
    lv_obj_set_style_bg_color(humi_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(humi_card, 15, 0);
    lv_obj_set_style_border_width(humi_card, 0, 0);
    lv_obj_set_style_pad_all(humi_card, 0, 0);
    lv_obj_align(humi_card, LV_ALIGN_CENTER, 115, -30);
    
    lv_obj_t *humi_icon = lv_label_create(humi_card);
    lv_label_set_text(humi_icon, "%");
    lv_obj_set_style_text_font(humi_icon, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(humi_icon, lv_color_hex(0x4ECDC4), 0);
    lv_obj_align(humi_icon, LV_ALIGN_TOP_LEFT, 15, 15);
    
    humi_label = lv_label_create(humi_card);
    lv_label_set_text(humi_label, "--");
    lv_obj_set_style_text_font(humi_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(humi_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(humi_label, LV_ALIGN_BOTTOM_LEFT, 15, -10);
    
    // Swipe hint
    lv_obj_t *hint1 = lv_label_create(tile1);
    lv_label_set_text(hint1, "< Swipe");
    lv_obj_set_style_text_font(hint1, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint1, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint1, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    // ========== Screen 2: Clock ==========
    lv_obj_t *tile2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_set_style_bg_color(tile2, lv_color_hex(0x0A0E1A), 0);
    
    // Title
    lv_obj_t *title2 = lv_label_create(tile2);
    lv_label_set_text(title2, "TIME");
    lv_obj_set_style_text_font(title2, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title2, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title2, LV_ALIGN_TOP_MID, 0, 15);
    
    // Time display
    time_label = lv_label_create(tile2);
    lv_label_set_text(time_label, "00:00");
    lv_obj_set_style_text_font(time_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -35);
    
    // Date display
    date_label = lv_label_create(tile2);
    lv_label_set_text(date_label, "Friday, January 3");
    lv_obj_set_style_text_font(date_label, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 25);
    
    // Swipe hint
    lv_obj_t *hint2 = lv_label_create(tile2);
    lv_label_set_text(hint2, "< Swipe >");
    lv_obj_set_style_text_font(hint2, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint2, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint2, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    // ========== Screen 3: Weather ==========
    lv_obj_t *tile3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_LEFT);
    lv_obj_set_style_bg_color(tile3, lv_color_hex(0x0A0E1A), 0);
    
    // Title
    lv_obj_t *title3 = lv_label_create(tile3);
    lv_label_set_text(title3, "WEATHER");
    lv_obj_set_style_text_font(title3, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title3, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title3, LV_ALIGN_TOP_MID, 0, 15);
    
    // Location
    lv_obj_t *location = lv_label_create(tile3);
    lv_label_set_text(location, "Linz, Griesmayrstr. 23");
    lv_obj_set_style_text_font(location, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(location, lv_color_hex(0x5A6B7D), 0);
    lv_obj_align(location, LV_ALIGN_TOP_MID, 0, 50);
    
    // Weather card
    lv_obj_t *weather_card = lv_obj_create(tile3);
    lv_obj_set_size(weather_card, 420, 140);
    lv_obj_set_style_bg_color(weather_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(weather_card, 20, 0);
    lv_obj_set_style_border_width(weather_card, 0, 0);
    lv_obj_set_style_pad_all(weather_card, 0, 0);
    lv_obj_align(weather_card, LV_ALIGN_CENTER, 0, 10);
    
    weather_temp_label = lv_label_create(weather_card);
    lv_label_set_text(weather_temp_label, "--°");
    lv_obj_set_style_text_font(weather_temp_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(weather_temp_label, lv_color_hex(0xFFB84D), 0);
    lv_obj_align(weather_temp_label, LV_ALIGN_CENTER, 0, -15);
    
    weather_desc_label = lv_label_create(weather_card);
    lv_label_set_text(weather_desc_label, "Loading...");
    lv_obj_set_style_text_font(weather_desc_label, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(weather_desc_label, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(weather_desc_label, LV_ALIGN_CENTER, 0, 40);
    
    // Swipe hint
    lv_obj_t *hint3 = lv_label_create(tile3);
    lv_label_set_text(hint3, "Swipe >");
    lv_obj_set_style_text_font(hint3, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint3, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint3, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    Serial.println("Modern UI created!");
    updateSensorData();
}

