#pragma once

#include <lvgl.h>

#include "core/app_types.h"

typedef void (*display_touch_read_cb_t)(lv_indev_drv_t *drv, lv_indev_data_t *data);

void display_init(AppState &state, display_touch_read_cb_t touch_read_cb);
void display_lvgl_handle();
void display_set_backlight(bool on);
void display_set_sleep(AppState &state, bool sleep);
