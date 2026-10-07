/*
 * 拾声 (XianDial) 硬件版 —— 引脚定义
 *
 * 依据：官方《01_IO资源分配表.xlsx》＋ xiaozhi-esp32/boards/lcdwiki-es3c35p/config.h
 * 板型：LCD Wiki ES3C35P  (ESP32-S3 N16R8 / 3.5" ST77922 QSPI / FT6336G / ES8311+FM8002E)
 */
#pragma once

/* ---- QSPI 屏 ---- */
#define PIN_LCD_CS      10
#define PIN_LCD_CLK     12
#define PIN_LCD_D0      11
#define PIN_LCD_D1      13
#define PIN_LCD_D2      14
#define PIN_LCD_D3      9
/* 屏无 RST 引脚（由 0x11 软复位序列唤醒）*/
#define PIN_LCD_BL      41      /* 背光，可 PWM */

/* ---- 触摸 FT6336G（与 ES8311 共用 I2C）---- */
#define PIN_TOUCH_SDA   38
#define PIN_TOUCH_SCL   39
#define PIN_TOUCH_RST   48
#define PIN_TOUCH_INT   47
#define TOUCH_I2C_ADDR  0x55

/* ---- 音频（ES8311 + FM8002E）---- */
#define PIN_PA_EN       1       /* 功放使能脚 */
/* ★★★ 功放极性：低电平使能 ★★★
 * 证据（10-03 查证，别再改反）：官方板级 xz_lcdwiki-es3c35p.cc 里是
 *   Es8311AudioCodec(..., AUDIO_CODEC_PA_PIN, ES8311_CODEC_DEFAULT_ADDR, true, true)
 * 最后那个 true = pa_reverted，而驱动头文件写明语义：
 *   device/include/es8311_codec.h:  false: enable PA when pin set to 1,
 *                                   true : enable PA when pin set to 0
 * 即官方板 GPIO1 输出【0】时功放才工作。
 * ⚠️ 10-03 曾按「高电平使能」写，结果功放全程关死 -> 整机完全无声（连底噪都没有），
 *    而日志一切正常、极易误判成软件问题。 */
#define PA_EN_ACTIVE    0       /* 功放开：输出低 */
#define PA_EN_IDLE      1       /* 功放关：输出高（静音） */
#define PIN_I2S_MCLK    17
#define PIN_I2S_BCLK    18
#define PIN_I2S_WS      21
#define PIN_I2S_DOUT    15      /* ESP32 -> codec */
#define PIN_I2S_DIN     16      /* codec -> ESP32 */
#define I2C_NUM_CODEC   0

/* ---- 其他 ---- */
#define PIN_BAT_ADC     8
#define PIN_LED_RGB     40
#define PIN_BOOT_BTN    0

/* ---- 面板物理分辨率（厂家 config.h：DISPLAY_WIDTH=320 / DISPLAY_HEIGHT=480）---- */
#define LCD_PANEL_W     320
#define LCD_PANEL_H     480

/* ---- LVGL 逻辑分辨率（横屏 480x320）----
 * 面板本身保持竖屏地址空间；横屏由 app_display.c 的 xs_flush_cb()
 * 在写屏时旋转 90°（display 的 rotation 字段保持 0，LVGL 不参与旋转）。 */
#define LCD_H_RES       480
#define LCD_V_RES       320

/*
 * 面板 MADCTL —— ★ 必须保持厂家原值 0x00，不要加 MV 位 ★
 *
 * 10-02 定案：ST77922 这块屏【不支持 XY 轴交换】。厂家板级源码里写着
 *     static_assert(!DISPLAY_SWAP_XY, "ST77922 does not support swapping the X and Y axes");
 * 驱动 esp_lcd_st77922 的 swap_xy() 也是空实现（直接返回 NOT_SUPPORTED）。
 * 之前设 0x60(MX|MV) 想硬件横屏 ⇒ MV 位被忽略，面板仍是 320 列地址空间，
 * 480 宽的帧只写进左边 320 列，右边 160 列保留旧画面（这就是"右边还是
 * 厂家 demo 的 22fps/15%CPU"的原因）。
 * 正解：面板保持竖屏 0x00，横屏由 app_display.c 的 xs_flush_cb() 在写屏时旋转 90°。
 */
#define LCD_MADCTL_PANEL   0x00
#define LCD_MADCTL_LANDSCAPE   0x00     /* 兼容旧引用；实际值同 LCD_MADCTL_PANEL */

/*
 * 触摸换算 —— 10-02 用真机点击数据定案
 *
 *   面板物理坐标（竖屏 320x480）  (rx, ry)：触摸控制器直接给出，与面板同坐标系
 *   逻辑横屏坐标（LVGL 480x320）  (lx, ly)
 *
 *       lx = (LCD_H_RES - 1) - ry      即 479 - ry
 *       ly = rx
 *
 * 推导依据：
 *   显示侧 xs_flush_cb() 把逻辑 (lx,ly) 画到面板 (ly, 479-lx)；
 *   触摸侧就是它的逆变换，反解得上面两式。
 *   （LVGL 自带的 lv_display_rotate_point(ROTATION_90) 也是同一个式子，
 *     但我们自建 display 且 rotation 恒为 0，所以 LVGL 不会再动这些点，
 *     必须由 touch_read_cb 自己换算 —— 早前两边各转一次，点击就全飞了。）
 *
 * 注意：这个换算必须与「显示旋转方向」一致。两种旋转方向相差 180 度，
 *   写反了的现象就是「点屏幕全落到对面」。实测四角若对不上，
 *   改下面两个镜像开关就能翻，不用动代码逻辑。
 */
#define TOUCH_MIRROR_LX  0      /* 置 1：lx 再镜像一次（左右翻） */
#define TOUCH_MIRROR_LY  0      /* 置 1：ly 再镜像一次（上下翻） */
