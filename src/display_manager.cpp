#include "display_manager.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_timer.h>
#include <driver/spi_master.h>

#include "app_config.h"

static esp_lcd_panel_handle_t s_panel_handle = nullptr;
static lv_disp_drv_t s_disp_drv;
static lv_disp_draw_buf_t s_disp_buf;
static lv_color_t *s_buf1 = nullptr;
static lv_color_t *s_buf2 = nullptr;

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                    esp_lcd_panel_io_event_data_t *edata,
                                    void *user_ctx)
{
    lv_disp_drv_t *disp_driver = static_cast<lv_disp_drv_t *>(user_ctx);
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;

    esp_lcd_panel_draw_bitmap(s_panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

static void lv_tick_task(void *arg)
{
    lv_tick_inc(APP_LVGL_TICK_PERIOD_MS);
}

void display_set_backlight(bool on)
{
    if (APP_LCD_BL < 0) return;
    digitalWrite(APP_LCD_BL, on ? APP_LCD_BL_ON_LEVEL : !APP_LCD_BL_ON_LEVEL);
}

void display_set_sleep(AppState &state, bool sleep)
{
    if (!s_panel_handle || sleep == state.display_sleeping) return;

    if (sleep) {
        esp_err_t err = esp_lcd_panel_disp_on_off(s_panel_handle, false);
        if (err != ESP_OK) {
            Serial.printf("[Display] Failed to turn off panel: 0x%x\n", err);
            return;
        }
        display_set_backlight(false);
        state.display_sleeping = true;
        Serial.println("[Display] Sleep enabled");
    } else {
        display_set_backlight(true);
        esp_err_t err = esp_lcd_panel_disp_on_off(s_panel_handle, true);
        if (err != ESP_OK) {
            Serial.printf("[Display] Failed to turn on panel: 0x%x\n", err);
            return;
        }
        state.display_sleeping = false;
        Serial.println("[Display] Wake up");
    }
}

void display_init(AppState &state, display_touch_read_cb_t touch_read_cb)
{
    Serial.println("Initializing SPI bus...");

    spi_bus_config_t buscfg = {
        .mosi_io_num = APP_LCD_MOSI,
        .miso_io_num = APP_LCD_MISO,
        .sclk_io_num = APP_LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = APP_LVGL_BUFFER_SIZE * sizeof(lv_color_t)
    };

    ESP_ERROR_CHECK(spi_bus_initialize(APP_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    Serial.println("Initializing LCD panel IO...");

    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.dc_gpio_num = APP_LCD_DC;
    io_config.cs_gpio_num = APP_LCD_CS;
    io_config.pclk_hz = APP_LCD_PIXEL_CLK;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;
    io_config.spi_mode = 0;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = notify_lvgl_flush_ready;
    io_config.user_ctx = &s_disp_drv;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)APP_LCD_SPI_HOST, &io_config, &io_handle));

    Serial.println("Initializing ST7789/ST7796 panel...");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = APP_LCD_RST,
        .color_space = APP_LCD_COLOR_ORDER_BGR ? ESP_LCD_COLOR_SPACE_BGR : ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, false, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(s_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel_handle, APP_LCD_INVERT_COLORS));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));
    Serial.printf("[Display] Color order: %s\n", APP_LCD_COLOR_ORDER_BGR ? "BGR" : "RGB");
    Serial.printf("[Display] Color inversion: %s\n", APP_LCD_INVERT_COLORS ? "ON" : "OFF");

    if (APP_LCD_BL >= 0) {
        Serial.printf("[Display] Backlight control pin: GPIO%d\n", APP_LCD_BL);
        pinMode(APP_LCD_BL, OUTPUT);
    }
    display_set_backlight(true);

    Serial.printf("ST7796 initialized! Resolution: %dx%d\n", APP_LCD_H_RES, APP_LCD_V_RES);

    Serial.println("Initializing LVGL...");
    lv_init();

    s_buf1 = static_cast<lv_color_t *>(heap_caps_malloc(APP_LVGL_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    s_buf2 = static_cast<lv_color_t *>(heap_caps_malloc(APP_LVGL_BUFFER_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));

    if (!s_buf1 || !s_buf2) {
        Serial.println("ERROR: Failed to allocate LVGL buffers!");
        while (1) delay(1000);
    }

    lv_disp_draw_buf_init(&s_disp_buf, s_buf1, s_buf2, APP_LVGL_BUFFER_SIZE);

    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = APP_LCD_H_RES;
    s_disp_drv.ver_res = APP_LCD_V_RES;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.draw_buf = &s_disp_buf;
    lv_disp_drv_register(&s_disp_drv);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };

    static esp_timer_handle_t periodic_timer = nullptr;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, APP_LVGL_TICK_PERIOD_MS * 1000));

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    indev_drv.scroll_limit = APP_TOUCH_SCROLL_LIMIT_PX;
    indev_drv.scroll_throw = APP_TOUCH_SCROLL_THROW;
    indev_drv.gesture_limit = APP_TOUCH_GESTURE_LIMIT_PX;
    indev_drv.gesture_min_velocity = APP_TOUCH_GESTURE_MIN_VELOCITY;
    lv_indev_drv_register(&indev_drv);
    Serial.printf("[Touch] scroll_limit=%u throw=%u gesture_limit=%u velocity=%u\n",
                  APP_TOUCH_SCROLL_LIMIT_PX,
                  APP_TOUCH_SCROLL_THROW,
                  APP_TOUCH_GESTURE_LIMIT_PX,
                  APP_TOUCH_GESTURE_MIN_VELOCITY);

    state.display_sleeping = false;
}

void display_lvgl_handle()
{
    lv_timer_handler();
}
