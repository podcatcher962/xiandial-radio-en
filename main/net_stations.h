#pragma once

/* 拾声英文版 · 转发头（由 tools/_gen_fw_en.py 生成，勿手改）
 *
 *  源码里 5 处写死 include net_stations.h：
 *    app_stlist.h / app_fav.c / app_hist.c /
 *    app_radio.c / ui_xiandial.c
 *  而英文版只有 net_stations_pub.h。
 *  两个头声明等价（NET_CAT_N / NET_PROV_N / net_station_t /
 *  四个 extern 全同），所以转发即可。
 */
#include "net_stations_pub.h"
