#include "ui_main.h"

#include <Arduino.h>
#include <lvgl.h>

#include "app_config.h"

LV_FONT_DECLARE(lv_font_jb_24);
LV_FONT_DECLARE(lv_font_jb_32);
LV_FONT_DECLARE(lv_font_jb_48);

struct UiRefs {
    lv_obj_t *temp_label = nullptr;
    lv_obj_t *humi_label = nullptr;
    lv_obj_t *time_label = nullptr;
    lv_obj_t *date_label = nullptr;
    lv_obj_t *weather_temp_label = nullptr;
    lv_obj_t *weather_desc_label = nullptr;
    lv_obj_t *tileview = nullptr;
};

static UiRefs s_ui;

void ui_update_sensor(const SensorSnapshot &snapshot)
{
    if (!s_ui.temp_label || !s_ui.humi_label) return;

    if (!snapshot.valid) {
        lv_label_set_text(s_ui.temp_label, "--");
        lv_label_set_text(s_ui.humi_label, "--");
        return;
    }

    char temp_str[16];
    char humi_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.1f", snapshot.temp_c);
    snprintf(humi_str, sizeof(humi_str), "%.0f", snapshot.humidity_pct);

    lv_label_set_text(s_ui.temp_label, temp_str);
    lv_label_set_text(s_ui.humi_label, humi_str);
}

void ui_update_time(const TimeSnapshot &snapshot)
{
    if (!s_ui.time_label || !s_ui.date_label || !snapshot.valid) return;

    char time_str[16];
    char date_str[48];
    strftime(time_str, sizeof(time_str), "%H:%M", &snapshot.local);
    strftime(date_str, sizeof(date_str), "%A, %B %e", &snapshot.local);
    lv_label_set_text(s_ui.time_label, time_str);
    lv_label_set_text(s_ui.date_label, date_str);
}

void ui_update_weather(const WeatherSnapshot &snapshot)
{
    if (!s_ui.weather_temp_label || !s_ui.weather_desc_label) return;

    if (!snapshot.valid) {
        lv_label_set_text(s_ui.weather_temp_label, "--\xC2\xB0");
        lv_label_set_text(s_ui.weather_desc_label, "Loading...");
        return;
    }

    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.1f\xC2\xB0", snapshot.temp_c);
    lv_label_set_text(s_ui.weather_temp_label, temp_str);
    lv_label_set_text(s_ui.weather_desc_label, snapshot.description.c_str());
}

void ui_init(AppState &state)
{
    Serial.println("Creating modern tileview UI with 3 screens...");

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0A0E1A), 0);

    s_ui.tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(s_ui.tileview, APP_LCD_H_RES, APP_LCD_V_RES);
    lv_obj_set_style_bg_color(s_ui.tileview, lv_color_hex(0x0A0E1A), 0);
    lv_obj_set_style_border_width(s_ui.tileview, 0, 0);

    lv_obj_t *tile1 = lv_tileview_add_tile(s_ui.tileview, 0, 0, LV_DIR_RIGHT);
    lv_obj_set_style_bg_color(tile1, lv_color_hex(0x0A0E1A), 0);

    lv_obj_t *title1 = lv_label_create(tile1);
    lv_label_set_text(title1, "HOME");
    lv_obj_set_style_text_font(title1, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title1, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title1, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *temp_card = lv_obj_create(tile1);
    lv_obj_set_size(temp_card, 200, 110);
    lv_obj_set_style_bg_color(temp_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(temp_card, 15, 0);
    lv_obj_set_style_border_width(temp_card, 0, 0);
    lv_obj_set_style_pad_all(temp_card, 0, 0);
    lv_obj_align(temp_card, LV_ALIGN_CENTER, -115, -30);

    lv_obj_t *temp_icon = lv_label_create(temp_card);
    lv_label_set_text(temp_icon, "\xC2\xB0" "C");
    lv_obj_set_style_text_font(temp_icon, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(temp_icon, lv_color_hex(0xFF6B6B), 0);
    lv_obj_align(temp_icon, LV_ALIGN_TOP_LEFT, 15, 15);

    s_ui.temp_label = lv_label_create(temp_card);
    lv_label_set_text(s_ui.temp_label, "--");
    lv_obj_set_style_text_font(s_ui.temp_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(s_ui.temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_ui.temp_label, LV_ALIGN_BOTTOM_LEFT, 15, -10);

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

    s_ui.humi_label = lv_label_create(humi_card);
    lv_label_set_text(s_ui.humi_label, "--");
    lv_obj_set_style_text_font(s_ui.humi_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(s_ui.humi_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_ui.humi_label, LV_ALIGN_BOTTOM_LEFT, 15, -10);

    lv_obj_t *hint1 = lv_label_create(tile1);
    lv_label_set_text(hint1, "< Swipe");
    lv_obj_set_style_text_font(hint1, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint1, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint1, LV_ALIGN_BOTTOM_MID, 0, -15);

    lv_obj_t *tile2 = lv_tileview_add_tile(s_ui.tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    lv_obj_set_style_bg_color(tile2, lv_color_hex(0x0A0E1A), 0);

    lv_obj_t *title2 = lv_label_create(tile2);
    lv_label_set_text(title2, "TIME");
    lv_obj_set_style_text_font(title2, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title2, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title2, LV_ALIGN_TOP_MID, 0, 15);

    s_ui.time_label = lv_label_create(tile2);
    lv_label_set_text(s_ui.time_label, "00:00");
    lv_obj_set_style_text_font(s_ui.time_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(s_ui.time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(s_ui.time_label, LV_ALIGN_CENTER, 0, -35);

    s_ui.date_label = lv_label_create(tile2);
    lv_label_set_text(s_ui.date_label, "Friday, January 3");
    lv_obj_set_style_text_font(s_ui.date_label, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(s_ui.date_label, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(s_ui.date_label, LV_ALIGN_CENTER, 0, 25);

    lv_obj_t *hint2 = lv_label_create(tile2);
    lv_label_set_text(hint2, "< Swipe >");
    lv_obj_set_style_text_font(hint2, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint2, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint2, LV_ALIGN_BOTTOM_MID, 0, -15);

    lv_obj_t *tile3 = lv_tileview_add_tile(s_ui.tileview, 2, 0, LV_DIR_LEFT);
    lv_obj_set_style_bg_color(tile3, lv_color_hex(0x0A0E1A), 0);

    lv_obj_t *title3 = lv_label_create(tile3);
    lv_label_set_text(title3, "WEATHER");
    lv_obj_set_style_text_font(title3, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title3, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title3, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *location = lv_label_create(tile3);
    lv_label_set_text(location, "Linz, Griesmayrstr. 23");
    lv_obj_set_style_text_font(location, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(location, lv_color_hex(0x5A6B7D), 0);
    lv_obj_align(location, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *weather_card = lv_obj_create(tile3);
    lv_obj_set_size(weather_card, 420, 140);
    lv_obj_set_style_bg_color(weather_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(weather_card, 20, 0);
    lv_obj_set_style_border_width(weather_card, 0, 0);
    lv_obj_set_style_pad_all(weather_card, 0, 0);
    lv_obj_align(weather_card, LV_ALIGN_CENTER, 0, 10);

    s_ui.weather_temp_label = lv_label_create(weather_card);
    lv_label_set_text(s_ui.weather_temp_label, "--\xC2\xB0");
    lv_obj_set_style_text_font(s_ui.weather_temp_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(s_ui.weather_temp_label, lv_color_hex(0xFFB84D), 0);
    lv_obj_align(s_ui.weather_temp_label, LV_ALIGN_CENTER, 0, -15);

    s_ui.weather_desc_label = lv_label_create(weather_card);
    lv_label_set_text(s_ui.weather_desc_label, "Loading...");
    lv_obj_set_style_text_font(s_ui.weather_desc_label, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(s_ui.weather_desc_label, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(s_ui.weather_desc_label, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t *hint3 = lv_label_create(tile3);
    lv_label_set_text(hint3, "Swipe >");
    lv_obj_set_style_text_font(hint3, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint3, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint3, LV_ALIGN_BOTTOM_MID, 0, -15);

    ui_update_sensor(state.sensor);
    ui_update_time(state.time);
    ui_update_weather(state.weather);

    Serial.println("Modern UI created!");
}
