/**
 * @file lv_conf.h
 * Configuration file for LVGL v8.3
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has a 8 bit interface (e.g. SPI) */
#define LV_COLOR_16_SWAP 1

/* Enable features to draw on transparent background. */
#define LV_COLOR_SCREEN_TRANSP 0

/* Adjust color mix functions rounding. GPUs might calculate color mix differently. */
#define LV_COLOR_MIX_ROUND_OFS (LV_COLOR_DEPTH == 32 ? 0: 128)

/* Prefix performance critical functions to place them into a faster memory (e.g RAM) */
#define LV_ATTRIBUTE_FAST_MEM

/* Prefix large constant arrays or structures to place them into slower memory (e.g. external FLASH) */
#define LV_ATTRIBUTE_LARGE_CONST

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* 1: use custom malloc/free, 0: use the built-in `lv_mem_alloc()` and `lv_mem_free()` */
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    /* Size of the memory available for `lv_mem_alloc()` in bytes (>= 2kB) */
    #define LV_MEM_SIZE (64U * 1024U)          /* [bytes] */

    /* Set an address for the memory pool instead of allocating it as a normal array. */
    #define LV_MEM_ADR 0     /* 0: unused */
    
    /* Automatically defrag on free. */
    #define LV_MEM_AUTO_DEFRAG 1
#else       /* LV_MEM_CUSTOM */
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>   /* Header for the dynamic memory function */
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif     /* LV_MEM_CUSTOM */

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period. LVG will redraw changed areas with this period time */
#define LV_DISP_DEF_REFR_PERIOD 30      /* [ms] */

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30     /* [ms] */

/* Use a custom tick source that tells the elapsed time in milliseconds. */
#define LV_TICK_CUSTOM 0
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         /* Header for the system time function */
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    /* Expression evaluating to current system time in ms */
#endif   /* LV_TICK_CUSTOM */

/* Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings. */
#define LV_DPI_DEF 130     /* [px/inch] */

/*=================
   COMPILER SETTINGS
 *=================*/

/* For big endian systems set to 1 */
#define LV_BIG_ENDIAN_SYSTEM 0

/* Define a custom attribute to `lv_tick_inc` function */
#define LV_ATTRIBUTE_TICK_INC

/* Define a custom attribute to `lv_timer_handler` function */
#define LV_ATTRIBUTE_TIMER_HANDLER

/* Define a custom attribute to `lv_disp_flush_ready` function */
#define LV_ATTRIBUTE_FLUSH_READY

/* Required alignment size for buffers */
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1

/* Will be added where memories needs to be aligned (with -Os data might not be aligned to boundary by default). */
#define LV_ATTRIBUTE_MEM_ALIGN

/*==================
 *   FONT USAGE
 *==================*/

/* Montserrat fonts with ASCII range and some symbols */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /* bpp = 3 */
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /* Hebrew, Arabic, Persian letters */
#define LV_FONT_SIMSUN_16_CJK            0  /* 1000 most common CJK radicals */

/* Pixel perfect monospace fonts */
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/* Optionally declare custom fonts here. */

/* Set a default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*===================
 *  WIDGET USAGE
 *==================*/

/* Documentation: https://docs.lvgl.io/latest/en/html/widgets/index.html */
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1

/*==================
 * EXTRA COMPONENTS
 *==================*/

/* Enable widgets */
#define LV_USE_ANIMIMG    1
#define LV_USE_CALENDAR   1
#define LV_USE_CHART      1
#define LV_USE_COLORWHEEL 1
#define LV_USE_IMGBTN     1
#define LV_USE_KEYBOARD   1
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_METER      1
#define LV_USE_MSGBOX     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*==================
 * EXAMPLES
 *==================*/
#define LV_BUILD_EXAMPLES 1

/*==================
 * DEMO USAGE
 *==================*/

/* Show some widget. It might be required to increase `LV_MEM_SIZE` */
#define LV_USE_DEMO_WIDGETS 1
#if LV_USE_DEMO_WIDGETS
    #define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif

/* Benchmark your system */
#define LV_USE_DEMO_BENCHMARK 1
#if LV_USE_DEMO_BENCHMARK
    /* Use RGB565A8 images with 16 bit color depth instead of ARGB8565 */
    #define LV_DEMO_BENCHMARK_RGB565A8 0
#endif

/* Stress test for LVGL */
#define LV_USE_DEMO_STRESS 1

/* Music player demo */
#define LV_USE_DEMO_MUSIC 0
#if LV_USE_DEMO_MUSIC
    #define LV_DEMO_MUSIC_SQUARE    0
    #define LV_DEMO_MUSIC_LANDSCAPE 0
    #define LV_DEMO_MUSIC_ROUND     0
    #define LV_DEMO_MUSIC_LARGE     0
    #define LV_DEMO_MUSIC_AUTO_PLAY 0
#endif

/*==================
 *    OTHERS
 *==================*/

/* 1: Enable API to take snapshot for object */
#define LV_USE_SNAPSHOT 0

/* 1: Enable Monkey test */
#define LV_USE_MONKEY 0

/* 1: Enable grid navigation */
#define LV_USE_GRIDNAV 0

/* 1: Enable lv_obj fragment */
#define LV_USE_FRAGMENT 0

/* 1: Support using images as font in label or span widgets */
#define LV_USE_IMGFONT 0

/* 1: Enable a published subscriber based messaging system */
#define LV_USE_MSG 0

/* 1: Enable Pinyin input method */
#define LV_USE_IME_PINYIN 0

/* 1: Enable file explorer */
#define LV_USE_FILE_EXPLORER 0

/*==================
 * LOG SETTINGS
 *==================*/

/* 1: Enable the log module */
#define LV_USE_LOG 1
#if LV_USE_LOG

    /* How important log should be added:
     * LV_LOG_LEVEL_TRACE       A lot of logs to give detailed information
     * LV_LOG_LEVEL_INFO        Log important events
     * LV_LOG_LEVEL_WARN        Log if something unwanted happened but didn't cause a problem
     * LV_LOG_LEVEL_ERROR       Only critical issue, when the system may fail
     * LV_LOG_LEVEL_USER        Only logs added by the user
     * LV_LOG_LEVEL_NONE        Do not log anything */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /* 1: Print the log with 'printf'; 0: User need to register a callback with `lv_log_register_print_cb()` */
    #define LV_LOG_PRINTF 1

    /* Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs */
    #define LV_LOG_TRACE_MEM        1
    #define LV_LOG_TRACE_TIMER      1
    #define LV_LOG_TRACE_INDEV      1
    #define LV_LOG_TRACE_DISP_REFR  1
    #define LV_LOG_TRACE_EVENT      1
    #define LV_LOG_TRACE_OBJ_CREATE 1
    #define LV_LOG_TRACE_LAYOUT     1
    #define LV_LOG_TRACE_ANIM       1

#endif  /* LV_USE_LOG */

#endif /* LV_CONF_H */
