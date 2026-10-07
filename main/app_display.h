#pragma once

#include <stdbool.h>
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "lvgl.h"

/* 屏 + 触摸 + LVGL 全套初始化（失败返回错误码） */
esp_err_t app_display_init(void);

/* 背光亮度 0~100 */
void app_backlight_set(int percent);

/* LVGL 显示句柄 */
lv_display_t *app_display_get(void);

/* I2C0 主总线句柄（触摸与 ES8311 共用）—— 供音频模块挂 codec 用 */
i2c_master_bus_handle_t app_i2c_bus_get(void);

/* 最近一次触摸读数（原始物理坐标 + 换算后的逻辑坐标） */
void app_touch_get_last(int *rx, int *ry, int *lx, int *ly, bool *pressed);
