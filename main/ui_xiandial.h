#pragma once

#include "lvgl.h"

/* 启动「拾声」界面（开机 splash → 首页） */
void ui_init(void);

/* 刷新「城市声音集 · 本地音频」页（重新读一遍 SD 卡目录）。
 * 从首页卡片进入该页时会自动调用一次；外部改了卡内容也可手动调。*/
void ui_sd_page_refresh(void);

/* ★ 10-06：台单热重载后重建网格（由 app_stlist.c 的 st_reload 命令调用）*/
void ui_reload_stations(void);
