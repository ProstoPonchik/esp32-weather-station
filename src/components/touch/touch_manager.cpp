#include "components/touch/touch_manager.h"

#include <Arduino.h>
#include <FT6X36.h>
#include <Wire.h>

#include "core/app_config.h"

static TwoWire s_i2c_touch(1);
static FT6X36 s_touch(&s_i2c_touch, APP_TOUCH_INT);

static bool s_touch_available = false;
static uint16_t s_touch_x = 0;
static uint16_t s_touch_y = 0;
static bool s_touch_pressed = false;
static bool s_prev_pressed = false;
static uint32_t s_last_poll_ms = 0;

static uint16_t clamp_coord(int32_t value, int32_t max_value)
{
    if (value < 0) return 0;
    if (value > max_value) return static_cast<uint16_t>(max_value);
    return static_cast<uint16_t>(value);
}

static void map_touch_to_screen(uint16_t raw_x, uint16_t raw_y, uint16_t &screen_x, uint16_t &screen_y)
{
    // Panel is rotated (swap XY). Clamp final coordinates so LVGL never gets out-of-range values.
    int32_t mapped_x = static_cast<int32_t>(raw_y);
    int32_t mapped_y = static_cast<int32_t>(APP_LCD_V_RES - 1) - static_cast<int32_t>(raw_x);
    screen_x = clamp_coord(mapped_x, APP_LCD_H_RES - 1);
    screen_y = clamp_coord(mapped_y, APP_LCD_V_RES - 1);
}

void touch_lvgl_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;

    if (!s_touch_available) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    if (s_touch_pressed) {
        uint16_t x = 0;
        uint16_t y = 0;
        map_touch_to_screen(s_touch_x, s_touch_y, x, y);
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void touch_init(AppState &state)
{
    Serial.println("=== Initializing I2C Bus 1 (Touch) ===");
    Serial.printf("Pins: SDA=%d, SCL=%d\n", APP_TOUCH_SDA, APP_TOUCH_SCL);
    s_i2c_touch.begin(APP_TOUCH_SDA, APP_TOUCH_SCL, APP_TOUCH_I2C_FREQ_HZ);
    s_i2c_touch.setTimeout(APP_I2C_TIMEOUT_MS);

    Serial.println("=== Touch Screen Initialization ===");

    pinMode(APP_TOUCH_RST, OUTPUT);
    digitalWrite(APP_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(APP_TOUCH_RST, HIGH);
    delay(200);

    s_i2c_touch.beginTransmission(APP_FT6336_ADDR);
    uint8_t error = s_i2c_touch.endTransmission();
    Serial.printf("Result: error=%d (0=OK, 2=NACK)\n", error);
    Serial.flush();

    if (error == 0 && s_touch.begin()) {
        s_touch_available = true;
        Serial.println(" OK!");
    } else {
        s_touch_available = false;
        Serial.println(" failed!");
    }

    state.touch_available = s_touch_available;
}

void touch_poll(AppState &state, TouchPoint &point)
{
    point = {};

    if (!s_touch_available) {
        state.touch = point;
        return;
    }

    uint32_t now = millis();
    if (now - s_last_poll_ms < APP_TOUCH_POLL_INTERVAL_MS) {
        if (s_touch_pressed) {
            map_touch_to_screen(s_touch_x, s_touch_y, point.x, point.y);
            point.pressed = true;
        }
        point.just_pressed = point.pressed && !s_prev_pressed;
        s_prev_pressed = point.pressed;
        state.touch = point;
        return;
    }
    s_last_poll_ms = now;

    uint8_t touches = s_touch.touched();

    if (touches > 0) {
        uint8_t data[16] = {};
        s_i2c_touch.beginTransmission(APP_FT6336_ADDR);
        s_i2c_touch.write(0);
        s_i2c_touch.endTransmission();

        s_i2c_touch.requestFrom(APP_FT6336_ADDR, static_cast<uint8_t>(16));
        for (int i = 0; i < 16 && s_i2c_touch.available(); i++) {
            data[i] = s_i2c_touch.read();
        }

        uint16_t raw_x = ((data[0x03] & 0x0F) << 8) | data[0x04];
        uint16_t raw_y = ((data[0x05] & 0x0F) << 8) | data[0x06];

        if (raw_x > 0 || raw_y > 0) {
            if (s_touch_pressed) {
                uint32_t alpha_div = 1U << APP_TOUCH_SMOOTHING_SHIFT;
                uint32_t alpha_keep = alpha_div - 1U;
                s_touch_x = static_cast<uint16_t>((s_touch_x * alpha_keep + raw_x) / alpha_div);
                s_touch_y = static_cast<uint16_t>((s_touch_y * alpha_keep + raw_y) / alpha_div);
            } else {
                s_touch_x = raw_x;
                s_touch_y = raw_y;
            }
            s_touch_pressed = true;

            map_touch_to_screen(s_touch_x, s_touch_y, point.x, point.y);
            point.pressed = true;
        } else {
            s_touch_pressed = false;
        }
    } else {
        s_touch_pressed = false;
    }

    point.just_pressed = point.pressed && !s_prev_pressed;
    s_prev_pressed = point.pressed;
    state.touch = point;
}
