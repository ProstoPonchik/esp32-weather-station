#pragma once

#include <lvgl.h>

#include "app_types.h"

void touch_init(AppState &state);
void touch_poll(AppState &state, TouchPoint &point);
void touch_lvgl_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);

