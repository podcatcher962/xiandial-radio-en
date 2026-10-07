/*
 * 拾声 (XianDial) 硬件版 —— 固件版本号（★ 全项目【唯一】的版本真相源）
 * ================================================================
 * 兰兰 10-04：「系统状态页固件的版本号要建立，关键改动的版本要备份好，
 *            随时可以回退」。
 *
 *  ★ 为什么单独开一个头文件，而不是把版本写在 app_sys.c 里：
 *    备份脚本 firmware/_fw_ver.py 直接读这个文件决定存档标签，
 *    状态页也直接 include 它。三个地方引用同一个宏 = 不可能对不上。
 *    （之前版本号来自 esp_app_get_description()->version，
 *      那是 ESP-IDF 的 PROJECT_VER，本项目没设过，一直显示 1.0。）
 *
 *  ★ 改版号时三件事一起做，缺一件就出问题：
 *    ① 改这里的 XS_FW_VERSION（状态页显示它，备份脚本读它）
 *    ② 跑 python _fw_ver.py snap "本版改了什么"   ← 关键改动必须存档
 *    ③ 把本轮改动写进速查卡的迭代记录
 *
 *  版本号规则：v<主>.<次> —— 主 = 台单/架构级大改，次 = 每一轮真机迭代。
 *  例：v1.0 = 第一次能出声；v1.22 = 第二十二次迭代。
 */
#pragma once
#include "xs_version_mode.h"   /* XS_FAV_PUBLISH：发布版形态开关，见 xs_version_mode.h */

/* ---- 版本标识（状态页「固件」行显示的就是它）---- */
#define XS_FW_VERSION  "v1.51"
#define XS_FW_DATE     "10-06"

/* ---- 一句话说明这一版改了什么（状态页第二行 / 备份 manifest 都用它）----
 * ★ 一定要写「人话」，别写「修了几个 bug」——
 *   将来回退时我要知道当时机器是「能出声」还是「不能出声」。
 * ⚠️ 改这句话【不用】重编固件也能生效，但它同时进 manifest；
 *   而 XS_FW_VERSION 在状态页有显示，改版本号才需要重编。
 *
 * ★★★ 10-06 发布版切分：
 *   完整排障笔记（含第三方公网 IP 与诊断过程）移到
 *     main/app_version_note_local.h —— 【已被 .gitignore 排除】。
 *   理由：① 发布版每个设备白带 1 KB 无用文本；
 *         ② 里面那个公网 IP 是排查时测过的第三方电台地址，
 *            留在发布版固件里会被读成「预置了要主动连的服务器」。
 *   ⇒ 自用版 XS_FAV_PUBLISH=0：include 本地笔记，信息一点不丢
 *     （_fw_ver.py 存档 manifest 仍读到完整版）；
 *   ⇒ 发布版 XS_FAV_PUBLISH=1：只留一句话说明，宏与功能都不变。*/
#if XS_FAV_PUBLISH
/* 发布版：一句话说明，不含任何第三方地址。 */
#  define XS_FW_NOTE     "v1.51 修 https 台播不出的根因: mbedTLS 单连接需要的内部 RAM 连续块不够. 自用版把 MBEDTLS_SSL_IN_CONTENT_LEN 调到 4096 并开启动态缓冲."
#else
#  include "app_version_note_local.h"
#  define XS_FW_NOTE     XS_FW_NOTE_LOCAL
#endif
