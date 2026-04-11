#include "ui/ui_main.h"

#include <Arduino.h>
#include <lvgl.h>
#include <math.h>

#include "core/app_config.h"
#include "ui/assets/weather_icons.h"

LV_FONT_DECLARE(lv_font_jb_24);
LV_FONT_DECLARE(lv_font_jb_32);
LV_FONT_DECLARE(lv_font_jb_48);
LV_FONT_DECLARE(lv_font_jb_16);

struct UiRefs {
    lv_obj_t *temp_label = nullptr;
    lv_obj_t *humi_label = nullptr;
    lv_obj_t *home_source_label = nullptr;

    lv_obj_t *time_label = nullptr;
    lv_obj_t *date_label = nullptr;

    lv_obj_t *weather_temp_label = nullptr;
    lv_obj_t *weather_icon_img = nullptr;
    lv_obj_t *weather_sun_label = nullptr;
    lv_obj_t *weather_hourly_icon[3] = {nullptr, nullptr, nullptr};
    lv_obj_t *weather_hourly_label[3] = {nullptr, nullptr, nullptr};
    lv_obj_t *weather_daily_icon[3] = {nullptr, nullptr, nullptr};
    lv_obj_t *weather_daily_label[3] = {nullptr, nullptr, nullptr};

    lv_obj_t *sht_status_label = nullptr;
    lv_obj_t *sht_temp_label = nullptr;
    lv_obj_t *sht_humi_label = nullptr;

    lv_obj_t *bme_status_label = nullptr;
    lv_obj_t *bme_temp_label = nullptr;
    lv_obj_t *bme_humi_label = nullptr;
    lv_obj_t *bme_pressure_label = nullptr;
    lv_obj_t *bme_gas_label = nullptr;

    lv_obj_t *tileview = nullptr;
    lv_obj_t *tile_home = nullptr;
    lv_obj_t *tile_time = nullptr;
    lv_obj_t *tile_weather = nullptr;
    lv_obj_t *tile_forecast = nullptr;
    lv_obj_t *tile_sensors_plus = nullptr;
};

static UiRefs s_ui;

static const lv_img_dsc_t *weather_icon_from_code(const String &icon_code)
{
    if (icon_code == "01d") return &weather_icon_01d;
    if (icon_code == "01n") return &weather_icon_01n;
    if (icon_code == "02d") return &weather_icon_02d;
    if (icon_code == "02n") return &weather_icon_02n;
    if (icon_code == "03d") return &weather_icon_03d;
    if (icon_code == "04d") return &weather_icon_04d;
    if (icon_code == "09d") return &weather_icon_09d;
    if (icon_code == "10d") return &weather_icon_10d;
    if (icon_code == "10n") return &weather_icon_10n;
    if (icon_code == "11d") return &weather_icon_11d;
    if (icon_code == "13d") return &weather_icon_13d;
    if (icon_code == "50d") return &weather_icon_50d;
    if (icon_code == "03n") return &weather_icon_03d;
    if (icon_code == "04n") return &weather_icon_04d;
    if (icon_code == "09n") return &weather_icon_09d;
    if (icon_code == "11n") return &weather_icon_11d;
    if (icon_code == "13n") return &weather_icon_13d;
    if (icon_code == "50n") return &weather_icon_50d;
    return &weather_icon_03d;
}

static void ui_set_weather_icon(lv_obj_t *img_obj, const String &icon_code)
{
    if (!img_obj) return;
    lv_img_set_src(img_obj, weather_icon_from_code(icon_code));
}

static const char *climate_source_text(ClimateSource source)
{
    switch (source) {
        case ClimateSource::Sht41:
            return "SRC: SHT41";
        case ClimateSource::Bme680:
            return "SRC: BME680";
        case ClimateSource::None:
        default:
            return "SRC: --";
    }
}

static void ui_tileview_gesture_cb(lv_event_t *e)
{
    lv_obj_t *tv = lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!tv || !indev) return;

    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    lv_obj_t *active = lv_tileview_get_tile_act(tv);

    if (active == s_ui.tile_home) {
        if (dir == LV_DIR_LEFT) lv_obj_set_tile_id(tv, 1, 0, LV_ANIM_ON);
    } else if (active == s_ui.tile_time) {
        if (dir == LV_DIR_RIGHT) lv_obj_set_tile_id(tv, 0, 0, LV_ANIM_ON);
        if (dir == LV_DIR_LEFT) lv_obj_set_tile_id(tv, 2, 0, LV_ANIM_ON);
    } else if (active == s_ui.tile_weather) {
        if (dir == LV_DIR_RIGHT) lv_obj_set_tile_id(tv, 1, 0, LV_ANIM_ON);
        if (dir == LV_DIR_LEFT) lv_obj_set_tile_id(tv, 3, 0, LV_ANIM_ON);
        if (dir == LV_DIR_BOTTOM) lv_obj_set_tile_id(tv, 2, 1, LV_ANIM_ON);
    } else if (active == s_ui.tile_forecast) {
        if (dir == LV_DIR_TOP) lv_obj_set_tile_id(tv, 2, 0, LV_ANIM_ON);
    } else if (active == s_ui.tile_sensors_plus) {
        if (dir == LV_DIR_RIGHT) lv_obj_set_tile_id(tv, 2, 0, LV_ANIM_ON);
    }

    lv_indev_wait_release(indev);
}

void ui_update_sensors(const SensorAggregate &snapshot)
{
    if (s_ui.temp_label && s_ui.humi_label) {
        if (!snapshot.primary.valid) {
            lv_label_set_text(s_ui.temp_label, "--");
            lv_label_set_text(s_ui.humi_label, "--");
        } else {
            char temp_str[16];
            char humi_str[16];
            snprintf(temp_str, sizeof(temp_str), "%.1f", snapshot.primary.temp_c);
            snprintf(humi_str, sizeof(humi_str), "%.0f", snapshot.primary.humidity_pct);
            lv_label_set_text(s_ui.temp_label, temp_str);
            lv_label_set_text(s_ui.humi_label, humi_str);
        }
    }

    if (s_ui.home_source_label) {
        lv_label_set_text(s_ui.home_source_label, climate_source_text(snapshot.primary.source));
    }

    if (s_ui.sht_status_label && s_ui.sht_temp_label && s_ui.sht_humi_label) {
        if (!snapshot.sht41.valid) {
            lv_label_set_text(s_ui.sht_status_label, "SHT41: unavailable");
            lv_label_set_text(s_ui.sht_temp_label, "T: --");
            lv_label_set_text(s_ui.sht_humi_label, "H: --");
        } else {
            char temp_str[24];
            char humi_str[24];
            snprintf(temp_str, sizeof(temp_str), "T: %.1f C", snapshot.sht41.temp_c);
            snprintf(humi_str, sizeof(humi_str), "H: %.0f%%", snapshot.sht41.humidity_pct);
            lv_label_set_text(s_ui.sht_status_label, "SHT41: OK");
            lv_label_set_text(s_ui.sht_temp_label, temp_str);
            lv_label_set_text(s_ui.sht_humi_label, humi_str);
        }
    }

    if (s_ui.bme_status_label && s_ui.bme_temp_label && s_ui.bme_humi_label && s_ui.bme_pressure_label &&
        s_ui.bme_gas_label) {
        if (!snapshot.bme680.valid) {
            lv_label_set_text(s_ui.bme_status_label, "BME680: unavailable");
            lv_label_set_text(s_ui.bme_temp_label, "T: --");
            lv_label_set_text(s_ui.bme_humi_label, "H: --");
            lv_label_set_text(s_ui.bme_pressure_label, "P: --");
            lv_label_set_text(s_ui.bme_gas_label, "Gas: --");
        } else {
            char temp_str[24];
            char humi_str[24];
            char pressure_str[24];
            char gas_str[32];
            snprintf(temp_str, sizeof(temp_str), "T: %.1f C", snapshot.bme680.temp_c);
            snprintf(humi_str, sizeof(humi_str), "H: %.0f%%", snapshot.bme680.humidity_pct);
            snprintf(pressure_str, sizeof(pressure_str), "P: %.1f hPa", snapshot.bme680.pressure_hpa);
            snprintf(gas_str, sizeof(gas_str), "Gas: %.0f ohm", snapshot.bme680.gas_resistance_ohm);
            lv_label_set_text(s_ui.bme_status_label, "BME680: OK");
            lv_label_set_text(s_ui.bme_temp_label, temp_str);
            lv_label_set_text(s_ui.bme_humi_label, humi_str);
            lv_label_set_text(s_ui.bme_pressure_label, pressure_str);
            lv_label_set_text(s_ui.bme_gas_label, gas_str);
        }
    }
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
    if (!s_ui.weather_temp_label || !s_ui.weather_icon_img || !s_ui.weather_sun_label) return;

    if (!snapshot.valid) {
        lv_label_set_text(s_ui.weather_temp_label, "--\xC2\xB0");
        ui_set_weather_icon(s_ui.weather_icon_img, "03d");
        lv_label_set_text(s_ui.weather_sun_label, "SR --:--  SS --:--");
        for (int i = 0; i < 3; i++) {
            if (s_ui.weather_hourly_label[i]) lv_label_set_text(s_ui.weather_hourly_label[i], "--:--  --\xC2\xB0");
            if (s_ui.weather_daily_label[i]) lv_label_set_text(s_ui.weather_daily_label[i], "---  --/--\xC2\xB0");
            ui_set_weather_icon(s_ui.weather_hourly_icon[i], "03d");
            ui_set_weather_icon(s_ui.weather_daily_icon[i], "03d");
        }
        return;
    }

    char temp_str[16];
    snprintf(temp_str, sizeof(temp_str), "%.1f\xC2\xB0", snapshot.temp_c);
    lv_label_set_text(s_ui.weather_temp_label, temp_str);
    ui_set_weather_icon(s_ui.weather_icon_img, snapshot.icon_code);

    String sun_line = "SR " + snapshot.sunrise + "  SS " + snapshot.sunset;
    lv_label_set_text(s_ui.weather_sun_label, sun_line.c_str());

    for (int i = 0; i < 3; i++) {
        if (snapshot.hourly[i].valid) {
            String hourly_text = snapshot.hourly[i].time_label + "  " +
                                 String(static_cast<int>(roundf(snapshot.hourly[i].temp_c))) + "\xC2\xB0";
            if (s_ui.weather_hourly_label[i]) lv_label_set_text(s_ui.weather_hourly_label[i], hourly_text.c_str());
            ui_set_weather_icon(s_ui.weather_hourly_icon[i], snapshot.hourly[i].icon_code);
        } else {
            if (s_ui.weather_hourly_label[i]) lv_label_set_text(s_ui.weather_hourly_label[i], "--:--  --\xC2\xB0");
            ui_set_weather_icon(s_ui.weather_hourly_icon[i], "03d");
        }

        if (snapshot.daily[i].valid) {
            String daily_text = snapshot.daily[i].day_label + "  " +
                                String(static_cast<int>(roundf(snapshot.daily[i].max_c))) + "/" +
                                String(static_cast<int>(roundf(snapshot.daily[i].min_c))) + "\xC2\xB0";
            if (s_ui.weather_daily_label[i]) lv_label_set_text(s_ui.weather_daily_label[i], daily_text.c_str());
            ui_set_weather_icon(s_ui.weather_daily_icon[i], snapshot.daily[i].icon_code);
        } else {
            if (s_ui.weather_daily_label[i]) lv_label_set_text(s_ui.weather_daily_label[i], "---  --/--\xC2\xB0");
            ui_set_weather_icon(s_ui.weather_daily_icon[i], "03d");
        }
    }
}

void ui_init(AppState &state)
{
    Serial.println("Creating modern tileview UI with weather and sensors pages...");

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x0A0E1A), 0);

    s_ui.tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_size(s_ui.tileview, APP_LCD_H_RES, APP_LCD_V_RES);
    lv_obj_set_style_bg_color(s_ui.tileview, lv_color_hex(0x0A0E1A), 0);
    lv_obj_set_style_border_width(s_ui.tileview, 0, 0);

    lv_obj_t *tile1 = lv_tileview_add_tile(s_ui.tileview, 0, 0, LV_DIR_RIGHT);
    s_ui.tile_home = tile1;
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

    s_ui.home_source_label = lv_label_create(tile1);
    lv_label_set_text(s_ui.home_source_label, "SRC: --");
    lv_obj_set_style_text_font(s_ui.home_source_label, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(s_ui.home_source_label, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(s_ui.home_source_label, LV_ALIGN_CENTER, 0, 55);

    lv_obj_t *hint1 = lv_label_create(tile1);
    lv_label_set_text(hint1, "< Swipe");
    lv_obj_set_style_text_font(hint1, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint1, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint1, LV_ALIGN_BOTTOM_MID, 0, -15);

    lv_obj_t *tile2 = lv_tileview_add_tile(s_ui.tileview, 1, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    s_ui.tile_time = tile2;
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

    lv_obj_t *tile3 = lv_tileview_add_tile(s_ui.tileview, 2, 0, LV_DIR_LEFT | LV_DIR_RIGHT | LV_DIR_BOTTOM);
    s_ui.tile_weather = tile3;
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
    lv_obj_set_size(weather_card, 440, 150);
    lv_obj_set_style_bg_color(weather_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(weather_card, 20, 0);
    lv_obj_set_style_border_width(weather_card, 0, 0);
    lv_obj_set_style_pad_all(weather_card, 0, 0);
    lv_obj_align(weather_card, LV_ALIGN_CENTER, 0, 25);

    s_ui.weather_icon_img = lv_img_create(weather_card);
    ui_set_weather_icon(s_ui.weather_icon_img, "03d");
    lv_obj_align(s_ui.weather_icon_img, LV_ALIGN_CENTER, -140, -45);

    s_ui.weather_temp_label = lv_label_create(weather_card);
    lv_label_set_text(s_ui.weather_temp_label, "--\xC2\xB0");
    lv_obj_set_style_text_font(s_ui.weather_temp_label, &lv_font_jb_48, 0);
    lv_obj_set_style_text_color(s_ui.weather_temp_label, lv_color_hex(0xFFB84D), 0);
    lv_obj_align(s_ui.weather_temp_label, LV_ALIGN_CENTER, 45, -45);

    s_ui.weather_sun_label = lv_label_create(weather_card);
    lv_label_set_text(s_ui.weather_sun_label, "SR --:--  SS --:--");
    lv_obj_set_style_text_font(s_ui.weather_sun_label, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(s_ui.weather_sun_label, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(s_ui.weather_sun_label, LV_ALIGN_CENTER, 0, 35);

    lv_obj_t *hint3 = lv_label_create(tile3);
    lv_label_set_text(hint3, "Swipe >  |  Swipe left | Swipe up");
    lv_obj_set_style_text_font(hint3, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(hint3, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint3, LV_ALIGN_BOTTOM_MID, 0, -15);

    lv_obj_t *tile4 = lv_tileview_add_tile(s_ui.tileview, 2, 1, LV_DIR_TOP);
    s_ui.tile_forecast = tile4;
    lv_obj_set_style_bg_color(tile4, lv_color_hex(0x0A0E1A), 0);

    lv_obj_t *title4 = lv_label_create(tile4);
    lv_label_set_text(title4, "FORECAST");
    lv_obj_set_style_text_font(title4, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title4, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title4, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *forecast_card = lv_obj_create(tile4);
    lv_obj_set_size(forecast_card, 440, 220);
    lv_obj_set_style_bg_color(forecast_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(forecast_card, 20, 0);
    lv_obj_set_style_border_width(forecast_card, 0, 0);
    lv_obj_set_style_pad_all(forecast_card, 0, 0);
    lv_obj_align(forecast_card, LV_ALIGN_CENTER, 0, 10);

    lv_obj_t *hourly_title = lv_label_create(forecast_card);
    lv_label_set_text(hourly_title, "HOURLY");
    lv_obj_set_style_text_font(hourly_title, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hourly_title, lv_color_hex(0xAFC0D3), 0);
    lv_obj_align(hourly_title, LV_ALIGN_TOP_LEFT, 20, 12);

    lv_obj_t *daily_title = lv_label_create(forecast_card);
    lv_label_set_text(daily_title, "DAILY");
    lv_obj_set_style_text_font(daily_title, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(daily_title, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(daily_title, LV_ALIGN_TOP_LEFT, 240, 12);

    for (int i = 0; i < 3; i++) {
        int y = 50 + (i * 52);

        s_ui.weather_hourly_icon[i] = lv_img_create(forecast_card);
        ui_set_weather_icon(s_ui.weather_hourly_icon[i], "03d");
        lv_obj_align(s_ui.weather_hourly_icon[i], LV_ALIGN_TOP_LEFT, 20, y);

        s_ui.weather_hourly_label[i] = lv_label_create(forecast_card);
        lv_label_set_text(s_ui.weather_hourly_label[i], "--:--  --\xC2\xB0");
        lv_obj_set_style_text_font(s_ui.weather_hourly_label[i], &lv_font_jb_16, 0);
        lv_obj_set_style_text_color(s_ui.weather_hourly_label[i], lv_color_hex(0xAFC0D3), 0);
        lv_obj_align(s_ui.weather_hourly_label[i], LV_ALIGN_TOP_LEFT, 72, y + 13);

        s_ui.weather_daily_icon[i] = lv_img_create(forecast_card);
        ui_set_weather_icon(s_ui.weather_daily_icon[i], "03d");
        lv_obj_align(s_ui.weather_daily_icon[i], LV_ALIGN_TOP_LEFT, 240, y);

        s_ui.weather_daily_label[i] = lv_label_create(forecast_card);
        lv_label_set_text(s_ui.weather_daily_label[i], "---  --/--\xC2\xB0");
        lv_obj_set_style_text_font(s_ui.weather_daily_label[i], &lv_font_jb_16, 0);
        lv_obj_set_style_text_color(s_ui.weather_daily_label[i], lv_color_hex(0x7B8FA1), 0);
        lv_obj_align(s_ui.weather_daily_label[i], LV_ALIGN_TOP_LEFT, 292, y + 13);
    }

    lv_obj_t *hint4 = lv_label_create(tile4);
    lv_label_set_text(hint4, "Swipe down");
    lv_obj_set_style_text_font(hint4, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint4, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint4, LV_ALIGN_BOTTOM_MID, 0, -15);

    lv_obj_t *tile5 = lv_tileview_add_tile(s_ui.tileview, 3, 0, LV_DIR_RIGHT);
    s_ui.tile_sensors_plus = tile5;
    lv_obj_set_style_bg_color(tile5, lv_color_hex(0x0A0E1A), 0);

    lv_obj_t *title5 = lv_label_create(tile5);
    lv_label_set_text(title5, "SENSORS+");
    lv_obj_set_style_text_font(title5, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(title5, lv_color_hex(0x7B8FA1), 0);
    lv_obj_align(title5, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *sht_card = lv_obj_create(tile5);
    lv_obj_set_size(sht_card, 440, 100);
    lv_obj_set_style_bg_color(sht_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(sht_card, 18, 0);
    lv_obj_set_style_border_width(sht_card, 0, 0);
    lv_obj_set_style_pad_all(sht_card, 0, 0);
    lv_obj_align(sht_card, LV_ALIGN_TOP_MID, 0, 60);

    s_ui.sht_status_label = lv_label_create(sht_card);
    lv_label_set_text(s_ui.sht_status_label, "SHT41: --");
    lv_obj_set_style_text_font(s_ui.sht_status_label, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(s_ui.sht_status_label, lv_color_hex(0xAFC0D3), 0);
    lv_obj_align(s_ui.sht_status_label, LV_ALIGN_TOP_LEFT, 15, 10);

    s_ui.sht_temp_label = lv_label_create(sht_card);
    lv_label_set_text(s_ui.sht_temp_label, "T: --");
    lv_obj_set_style_text_font(s_ui.sht_temp_label, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(s_ui.sht_temp_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_align(s_ui.sht_temp_label, LV_ALIGN_TOP_LEFT, 15, 52);

    s_ui.sht_humi_label = lv_label_create(sht_card);
    lv_label_set_text(s_ui.sht_humi_label, "H: --");
    lv_obj_set_style_text_font(s_ui.sht_humi_label, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(s_ui.sht_humi_label, lv_color_hex(0x4ECDC4), 0);
    lv_obj_align(s_ui.sht_humi_label, LV_ALIGN_TOP_LEFT, 180, 52);

    lv_obj_t *bme_card = lv_obj_create(tile5);
    lv_obj_set_size(bme_card, 440, 120);
    lv_obj_set_style_bg_color(bme_card, lv_color_hex(0x1A1F2E), 0);
    lv_obj_set_style_radius(bme_card, 18, 0);
    lv_obj_set_style_border_width(bme_card, 0, 0);
    lv_obj_set_style_pad_all(bme_card, 0, 0);
    lv_obj_align(bme_card, LV_ALIGN_TOP_MID, 0, 175);

    s_ui.bme_status_label = lv_label_create(bme_card);
    lv_label_set_text(s_ui.bme_status_label, "BME680: --");
    lv_obj_set_style_text_font(s_ui.bme_status_label, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(s_ui.bme_status_label, lv_color_hex(0xFFB84D), 0);
    lv_obj_align(s_ui.bme_status_label, LV_ALIGN_TOP_LEFT, 15, 10);

    s_ui.bme_temp_label = lv_label_create(bme_card);
    lv_label_set_text(s_ui.bme_temp_label, "T: --");
    lv_obj_set_style_text_font(s_ui.bme_temp_label, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(s_ui.bme_temp_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_align(s_ui.bme_temp_label, LV_ALIGN_TOP_LEFT, 15, 52);

    s_ui.bme_humi_label = lv_label_create(bme_card);
    lv_label_set_text(s_ui.bme_humi_label, "H: --");
    lv_obj_set_style_text_font(s_ui.bme_humi_label, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(s_ui.bme_humi_label, lv_color_hex(0x4ECDC4), 0);
    lv_obj_align(s_ui.bme_humi_label, LV_ALIGN_TOP_LEFT, 180, 52);

    s_ui.bme_pressure_label = lv_label_create(bme_card);
    lv_label_set_text(s_ui.bme_pressure_label, "P: --");
    lv_obj_set_style_text_font(s_ui.bme_pressure_label, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(s_ui.bme_pressure_label, lv_color_hex(0xAFC0D3), 0);
    lv_obj_align(s_ui.bme_pressure_label, LV_ALIGN_TOP_LEFT, 15, 82);

    s_ui.bme_gas_label = lv_label_create(bme_card);
    lv_label_set_text(s_ui.bme_gas_label, "Gas: --");
    lv_obj_set_style_text_font(s_ui.bme_gas_label, &lv_font_jb_16, 0);
    lv_obj_set_style_text_color(s_ui.bme_gas_label, lv_color_hex(0xAFC0D3), 0);
    lv_obj_align(s_ui.bme_gas_label, LV_ALIGN_TOP_LEFT, 220, 82);

    lv_obj_t *hint5 = lv_label_create(tile5);
    lv_label_set_text(hint5, "Swipe right");
    lv_obj_set_style_text_font(hint5, &lv_font_jb_24, 0);
    lv_obj_set_style_text_color(hint5, lv_color_hex(0x3D4A5C), 0);
    lv_obj_align(hint5, LV_ALIGN_BOTTOM_MID, 0, -15);

    lv_obj_add_event_cb(s_ui.tileview, ui_tileview_gesture_cb, LV_EVENT_GESTURE, nullptr);

    ui_update_sensors(state.sensors);
    ui_update_time(state.time);
    ui_update_weather(state.weather);

    Serial.println("Modern UI created!");
}
