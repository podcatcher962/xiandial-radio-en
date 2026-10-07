# ============================================================
#  拾声 (XianDial) 英文版 —— main 组件
#============================================================
# ★★★ 这个文件由 tools/_gen_fw_en_cmake.py 【生成】，勿手改 ★★★
#   改法：改 xiandial-radio/main/CMakeLists.txt 里带 【EN】 标记的那几行，
#   然后重跑生成器。理由：两份 CMakeLists 靠人手同步，
#   迟早会漂移 —— 而漂移的后果是【英文版编到中文版的字库/台单】，
#   界面全是方块，而且是静默的（编译通过、烧录成功）。
#
#=========================== 与中文版的差异（共 3 处）===========================
#  ① 字库文件名：xs_font_12.c → xs_font_en12.c
#     ★ 符号名【不变】（都叫 xs_font_12）⇒ ui_xiandial.c 一字不改。
#  ② 台单：英文版只允许发布版那 3 个文件。
#     ⇒ 这里【不做】if(EXISTS net_stations_pub.c) 二选一 —— 英文版
#       本来就只有发布版，加那个判断反而给了"哪天多了 net_stations.c
#       就自动编自用版"的后门。所以【显式列出】并断言它不存在。
#  ③ XS_FAV_PUBLISH 固定为 1（英文版不播种任何收藏）。
#
#  ★★ 为什么英文版【必须有 net_stations_pub.c】
#   net_stations_pub.h 里声明了 g_cat_name / g_prov_name / g_stations /
#   g_station_count，ui_xiandial.c 与 app_stlist.c 都在 extern。
#   少这个文件 → 链接期报 undefined reference。
#   而它的 g_station_count = 0，正是"空台单"发布要求的形态。
# ============================================================
list(APPEND XS_SRCS
    "xiandial_main.c"
    "app_display.c"
    "app_audio.c"
    "app_sd.c"
    "app_stlist.c"
    "app_sys.c"
    "app_prov.c"
    "app_radio.c"
    "app_hls.c"
    "app_fav.c"
    "app_hist.c"
    "app_sleep.c"
    "app_pwr.c"
    "app_led.c"
    "ui_xiandial.c"
    # ---- 英文版界面字库（gen_fonts_en.py 产；符号名与中文版相同）----
    "fonts/xs_font_en12.c"
    "fonts/xs_font_en16.c"
    "fonts/xs_font_en24.c"
    "fonts/xs_font_cjk12.c"
    # ---- 发布版台单（0 条）+ 通用汉字台名字库（GB2312 一级 3755 字）----
    "net_stations_pub.c"
    "fonts/xs_font_st12.c"
    "fonts/xs_font_st16.c"
)

#★★ 发布禁区：英文版目录里【绝不允许】出现自用版台单 ★★
#   一旦有 net_stations.c（1254 条真实台源 URL）与 net_stations_seed.h
#   （10 个真实台名），打包时会把兰兰的私人台单一起发出去。
#   ⇒ 在配置阶段就硬失败，不给"忘了删"任何机会。
#   （同一个事实tools/_gen_fw_en.py 生成时也查一遍，
#     这里是第二道：生成时查"刚生成干净吗"，这里查"打包前还干净吗"。）
#
# ★★ net_stations.h 【不在】禁入名单里，尽管名字和自用版一样 ★★
#   源码 5 处写死 #include "net_stations.h"（app_stlist.h / app_fav.c /
#   app_hist.c / app_radio.c / ui_xiandial.c），而英文版只有
#   net_stations_pub.h ⇒ 缺了它直接编不过。
#   生成器造的是一行转发头（#include "net_stations_pub.h"），
#   内容级检查在 _build_en.py 与 _release_check_en.py。
#   ⇒ 同一个名字下可以是完全不同的两种东西，判据必须看【内容】。
foreach(_forbidden net_stations.c net_stations_seed.h)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_forbidden}")
        message(FATAL_ERROR
            "英文版目录里出现了自用版文件 ${_forbidden} —— "
            "它含 1254 条真实台源/私人收藏，绝不能进国际版发布包。"
            "请删掉它（由 tools/_gen_fw_en.py 重新生成）。")
    endif()
endforeach()

# ★★ net_stations.h 必须是转发头，不能是自用版内容的副本 ★★
#   （那份注释里有兰兰的台单来源与 1254/689/201 等数字。）
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/net_stations.h")
    message(FATAL_ERROR
        "缺 net_stations.h 转发头 —— 源码 5 处写死 include 它。"
        "请重跑 python tools/_gen_fw_en.py")
endif()
file(READ "${CMAKE_CURRENT_LIST_DIR}/net_stations.h" _xs_fwd)
if(NOT _xs_fwd MATCHES "net_stations_pub\\.h")
    message(FATAL_ERROR
        "net_stations.h 里没有转发到 net_stations_pub.h —— "
        "它可能被误填成自用版内容（会带出私人批注）。"
        "请重跑 python tools/_gen_fw_en.py")
endif()

message(STATUS "拾声：英文版 · 0 条台单（net_stations_pub.c）+ 英文字库 "
               "+ GB2312 通用台名字库")
set(XS_FAV_PUBLISH 1)

idf_component_register(
    SRCS
        ${XS_SRCS}
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_ledc
        esp_driver_spi
        esp_driver_i2s
        esp_driver_sdmmc
        sdmmc
        esp_driver_usb_serial_jtag
        esp_adc
        esp_wifi
        esp_netif
        esp_event
        esp_http_server
        lwip
        esp_http_client
        mbedtls
        nvs_flash
        fatfs
        esp_timer
        esp_lcd
        esp_app_format
        lvgl
        espressif__esp_lvgl_port
        espressif__esp_lcd_st77922
        espressif__esp_lcd_touch
        espressif__esp_lcd_touch_ft5x06
        espressif__esp_codec_dev
        espressif__esp_audio_codec
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE XS_FAV_PUBLISH=${XS_FAV_PUBLISH})
message(STATUS "拾声：英文版 XS_FAV_PUBLISH=1（不播种收藏）")