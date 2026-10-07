/*
 * 拾声 (XianDial) 硬件版 —— 界面
 * 设计语言「声场 Soundfield」· 横屏 480x320 · 触摸为主力
 *   底 #0B0E12 / 面板 #151A21 / 高亮 #1E252E / 主色拾声绿 #34D399
 *   次色电波青 #38BDF8 / 状态琥珀 #FBBF24 / 文本 #F2F5F8 / 次要 #7C8894
 *   夜间暖白 #C8BFA8 / 分隔 #2A323C
 *
 * 迭代记录
 *   10-02 第五次：把所有「该能点」的控件真的接上交互（此前只有导航栏接了）。
 *   10-03 第七次：
 *     ① 第 3 页「播放」从「假 FM 调谐盘」改成真正的【本地播放界面】：
 *        刻度环变成进度环，环心显示曲名 + 已播/总时长，右侧显示格式与状态，
 *        三个键变成 上一个/暂停/下一个（真的换曲），底部加「本地列表」回 SD 页。
 *        —— 兰兰反馈「按了本地音频文件没有对应的播放界面」，就是缺这一页。
 *     ② 点 SD 页里的音频文件后【自动跳到第 3 页】，不用手动点导航。
 *     ③ 状态页网络行显示 IP / SSID，并加「连 wifi.txt」按钮（随时重连）。
 *   10-03 第八次 —— ★ 让网络电台真的出声：
 *     ① 首页/发现页的【假台单换成真台单】（net_stations.c，119 台国内台，
 *        全部无 TLS 的 http，裸请求就 200）。点一行 = 真的开播，不是弹提示音。
 *     ② 播放页支持【直播态】：直播没有总时长，环变成「电平表」，
 *        时长行显示 LIVE，格式行显示「网络电台 · 64 kbps」。
 *     ③ 上一个/下一个键：在放本地文件就换曲，在放电台就换台。
 *     ④ 台名用【台名专用字库】xs_font_st14/18 —— 台名里有「圳 涿 碚 禺」
 *        这类常用字库没有的字，不单独做一份就是满屏方块（见 gen_fonts_st.py）。
 *
 *   10-03 第九次（本次）—— 兰兰上手后的第一批反馈：
 *     ① 顶栏右上不再是「WiFi 已连」几个字，改成【图标】：
 *        绿色 WiFi 图标 = 已连，暗的 = 没连上；旁边一个【内存卡图标】+「TF 29G」。
 *        （图标直接用 LVGL 内置 montserrat_14 里的 LV_SYMBOL_WIFI / _SD_CARD，
 *          不自绘矢量 —— 省事而且清晰。）
 *     ② 电台库从 119 台扩到 687 台，【栏目 10 个 / 省份 31 个】：
 *        栏目与省份不再靠台名猜，直接照抄网页版台单每条台的 g（栏目）/ c（省份）。
 *     ③ 首页「栏目」条、【发现页「地区」条】都改成【横向可滑】的：
 *        芯片放不下就往右滑，不再只有新闻/音乐/交通三个。
 *     ④ 首页/发现页/分类页的电台列表全部改成【双列 + 上下滑动】，
 *        并撤掉「换一批」——上下滑比翻批更顺手（兰兰原话）。
 *     ⑤ 点首页的栏目芯片 → 【全屏分类页】（只有名称、双列、右上角返回键）。
 *        屏幕小，全屏页一屏能放 14 个台，比挤在首页里强。
 *     ⑥ 收藏电台：首页那张卡原来点下去只弹提示音（兰兰说「点击无效」），
 *        现在进【全屏收藏页】；并且收藏【机器上能自己设】——
 *        播放页有「☆收藏」键、网格里长按也能收藏，存 NVS 掉电不丢（app_fav.c）。
 *
 *   10-03 第十次（本次）—— 兰兰上手后的第二批反馈：
 *     ① 顶栏去掉重复的容量显示：原来「● 就绪 · TF 29G」那格和 SD 卡图标右边
 *        的「29 G」是同一件事，屏幕上出现两次。SD 图标右边保留唯一一份，
 *        「● 就绪」那格换成【当前时间】（兰兰：「如果不重要可以替换为当前时间」）。
 *     ② 播放页加【返回键】，做到「从哪里来回到哪里」：
 *        每层网格记住自己来自哪一页（首页 / 发现 / 全屏分类页），
 *        播放页的返回键就回那一页；全屏分类页再点它自己的返回，回发现/首页。
 *        ★ 不做成「统一的播放页固定返回首页」—— 兰兰原话：
 *          「要返回刚刚在发现页选择的地区栏」。
 *     ③ 播放页加【动态频谱】：真频谱（app_radio 里 Hann 窗 + 12 点 Goertzel），
 *        不是静态示意。首页那张卡上的假柱子也换成真的。
 *     ④ 台单换成【自用版】：国内 686 台 + 港澳台/海外 201 台（_gen_stations_all.py）。
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "app_audio.h"
#include "app_display.h"
#include "app_fav.h"
#include "app_hist.h"
#include "app_pins.h"
#include "app_prov.h"
#include "app_pwr.h"         /* ★ 10-04：电量图标 + 关机（app_pwr.c 驱动）*/
#include "app_radio.h"
#include "app_sd.h"
#include "app_sleep.h"      /* 睡眠定时（10-04 新增） */
#include "app_sys.h"
#include "app_version.h"
#include "net_stations.h"
#include "ui_xiandial.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_stlist.h"

static const char *TAG = "XSUI";

/* ---------- 颜色 ---------- */
#define C_BG      lv_color_hex(0x0B0E12)
#define C_PANEL   lv_color_hex(0x151A21)
#define C_HILITE  lv_color_hex(0x1E252E)
#define C_GREEN   lv_color_hex(0x34D399)
#define C_CYAN    lv_color_hex(0x38BDF8)
#define C_AMBER   lv_color_hex(0xFBBF24)
#define C_TEXT    lv_color_hex(0xF2F5F8)
#define C_MUTED   lv_color_hex(0x7C8894)
#define C_WARM    lv_color_hex(0xC8BFA8)
#define C_LINE    lv_color_hex(0x2A323C)
#define C_ONDARK  lv_color_hex(0x06231A)   /* 绿底上的深字 */

/* ---------- 字库（由 firmware/gen_fonts.py 用 lv_font_conv 生成）---------- */
/* ★★★ 10-05 换字库 + 砍字号档（v1.36）
 *   旧：simhei/Deng（专有，中易＋方正）11/14/18/38/52 五档 ＋ cjk14 ＝ 10.85 MB
 *   新：Ark Pixel 12px（OFL-1.1）＋思源黑体补字（OFL-1.1）12/24 两档 ＝ 4.02 MB
 *
 *   为什么只剩两档：Ark Pixel 是【像素字体】，按整数倍放大才不虚。
 *   12→24 是干净的 2 倍；旧的 11/14/18/38/52 全是非整数缩放，
 *   像素风会被拉糊，与 v2.0「赛博荧光」像素风设计稿背道而驰。
 *   而且 52px 那 4.2 MB 只用来画开机页「拾声」两个字，一个台名都用不到。
 *
 *   ⚠️ 迁移映射（旧名已不再生成，漏改的地方编译器会抓出来，这是好事）：
 *       xs_font_11 / xs_font_14 → xs_font_12
 *       xs_font_18 / xs_font_38 → xs_font_24
 *       xs_font_52               → 【删】改用 24
 *       xs_font_cjk14            → xs_font_cjk12
 *       xs_font_st14 / st18      → xs_font_st12 / st24
 *
 *   授权：Ark Pixel 与思源黑体都是 SIL OFL-1.1，允许嵌入固件、允许子集化。
 *   Ark 的 OFL.txt 无 Reserved Font Name 条款 ⇒ 子集化不触发改名义务。
 *   评估全文见 xiandial-hw/拾声v2.0-字库版权风险评估.md
 *
 *★★★ 10-05 真机反馈 → 【加回 16px 中间档】（v1.37）
 *   v1.36 只留 12/24 两档，真机烧完兰兰报了一串「字体太大、换行、互相遮挡」：
 *   正在播放台名 / 音乐库 / 最近听过 / 地区列表标题 / 封面页署名。
 *   根因【不是布局写错，而是两档之间没有过渡档】：
 *   旧 18px 一次性映射到 24px（放大 33%），而这些 label 的容器宽高
 *   还是按 18px 时代算的 ⇒ 文字撑出容器、压住下一行。
 *   16px 实测体积 0.45 MB（12px 是 0.31、24px 是 0.96），
 *   app 分区还有 59% 余量，换得回所有「太大」的位置，划算。
 *   ⚠️ 16 对12 是 1.333倍非整数缩放，边缘有抗锯齿、锐利度不如 12/24。
 *     所以只用在【短固定文案】上（「音乐库」「最近听过」这类 3~4 字），
 *     长文与长台名仍走 12px + LV_LABEL_LONG_DOT，绝不靠放大解决。 */
extern const lv_font_t xs_font_24;
extern const lv_font_t xs_font_16;
extern const lv_font_t xs_font_12;

/* ★ SD 页专用「大字符集」字库：GB2312 符号区 + 一级汉字（约 4400 字，另带 ASCII）。
 *   其余页面继续用小字库以省 Flash —— 那些页面的文案是编译期写死的，
 *   扫源码生成的字库足够。但 SD 页要显示【运行时才知道的文件名】，
 *   卡里叫「小宇宙播客」源码里从没出现过 ⇒ 小字库画出来全是方块（10-03 兰兰反馈）。
 *   所以 SD 页整体切到这个字库。*/
extern const lv_font_t xs_font_cjk12;

/* ★ 台名专用字库（firmware/gen_fonts_st.py 生成）：
  *   台名、栏目名、省份名都是运行时才知道显示哪一条的，共 772 个汉字，
  *   里面有「圳涿 澧 碚 禺 廣 華 藝 國」这些常用字库（GB2312 一级）没有的字 ——
  *   不单独出一份，三列网格里那些台名就是一排方块。
  *   覆盖率已实测：台单 753 字 + GB2312 4437 字，Ark＋思源【零缺口】。
  *   ⚠️ 自用版与发布版的这两个符号【同名】，字形来源不同（见 CMakeLists 注释）：
  *     自用版  fonts/xs_font_st12.c   ← 按 net_stations.c 的用字烘焙
  *     发布版  fonts/xs_font_pub_st12.c ← 按 GB2312 一级 3755 字烘焙
  *   ⇒ 这里的 extern 和下面两个 #define 【两个版本都不用改】。*/
extern const lv_font_t xs_font_st12;
extern const lv_font_t xs_font_st16;
/* ★★ st24 已于 10-06 删除，不是忘了编译：
  *   10-05 换字号档时把它从 26px 映射到 24px 留着，但【没有任何地方用它】——
  *   全 ui_xiandial.c 检索只有这里的 extern 和下面的 #define 两处提到，
  *   一个 label 都没挂上去。它占 1.2 MB 编译时间和仓库体积，链接器虽会丢弃，
  *   但每次全量编译都要为它编一遍。
  *   真要用 24px 台名，走 F_H2（xs_font_24）—— 但它只含 625 个界面文案字，
  *   台名会出方块，见下面 F_H2 的警告。*/

#define F_TITLE  (&xs_font_24)   /* 大标题（v2.0 收紧，原 52px 已废）*/
#define F_BIG    (&xs_font_24)
#define F_MID    (&xs_font_16)   /* ★ 10-05 新增：3~6 字短标题专用。
                                   「音乐库」「最近听过」「封面署名」这类
                                   字少但用 24px 会撑出容器、用 12px 又太弱，
                                   16px 正好。⚠️ 不要用在长文/长台名上，
                                   16px 对 12px 是 1.333倍缩放，长文会显毛糙。*/
#define F_H2     (&xs_font_24)   /* 大号数字 / 台名（历史遗留，慎用）*/
#define F_BODY   (&xs_font_12)   /* 正文、列表行 */
#define F_TINY   (&xs_font_12)   /* 顶栏时钟、按钮小字 */
#define F_SD     (&xs_font_cjk12)/* SD 页（运行时文件名）*/
/* 台名/栏目/省份三处都用它（含 GB2312 里没有的那些字）*/
#define F_ST12   (&xs_font_st12)
#define F_ST16   (&xs_font_st16)  /* ★ 10-05 新增：播放页大台名 */
#define F_ST14   (&xs_font_st12)   /* 兼容旧名 → 12px */
#define F_ST18   (&xs_font_st16)   /* ★ 10-05：旧名改指 16px（v1.36 指向 24px 太大）*/
/* ★ 台名【永远不要用 F_H2/F_BIG】—— 那是主字库，只有 625 个界面文案字，
 *   台单有 753 个字，「苏州儿童广播」的「儿」「童」就不在里面 ⇒ 真机上方块。*/

/* ★ 顶栏图标字体：LVGL 自带的 montserrat_14 里含 LV_SYMBOL_* 那一段
 *   （码位 U+F000~U+F8A3，WIFI=U+F1EB、SD_CARD=U+F7C2）。
 *   sdkconfig 里 CONFIG_LV_FONT_MONTSERRAT_14=y，所以不用自己画矢量图。
 *   比自绘三条弧 + 一个方块省事得多，而且边缘清晰。*/
#define F_ICON   (&lv_font_montserrat_14)

/* ---------- 布局常量 ---------- */
#define TOPBAR_H   38
#define RAIL_W     56
#define CONT_X     RAIL_W
#define CONT_Y     TOPBAR_H
#define CONT_W     (LCD_H_RES - RAIL_W)     /* 424 */
#define CONT_H     (LCD_V_RES - TOPBAR_H)   /* 282 */

#define PAGES_N    8      /* 0 首页 1 发现 2 播放 3 夜间 4 状态 5 本地音频
                           * 6 全屏电台列表（首页栏目 / 发现页地区 / 收藏 共用）
                           * 7 WiFi 连接设置（第二十二次新增，状态页进去）*/
#define NAV_N      5      /* 左侧导航栏只有前 5 个（第 6/7 页是全屏覆盖页，靠返回键）*/

/* ---------- 静态句柄 ---------- */
static lv_obj_t *s_splash_scr;
static lv_obj_t *s_main_scr;
static lv_obj_t *s_pages[PAGES_N];
static lv_obj_t *s_nav_btns[NAV_N];
static lv_obj_t *s_nav_marks[NAV_N];
/* ★ 第二十二次：导航栏文字句柄。改绿色之后，切页必须同步改亮度，
 *   否则「首页」永远是亮绿、「状态」永远是暗的。*/
static lv_obj_t *s_nav_labels[NAV_N];
static int       s_cur_page = 0;

static lv_obj_t *s_lbl_station;   /* 首页：大卡台名 */
static lv_obj_t *s_lbl_source;    /* 首页：大卡来源/码率 */
static lv_obj_t *s_lbl_clock;     /* 夜间：时钟 */
static lv_obj_t *s_lbl_date;      /* 夜间：日期（10-04 从写死改成真时间） */
static lv_obj_t *s_lbl_playstate; /* 状态：播放状态（★ 首页/发现页/夜间页共用，见下） */
/* ★★ 10-04：s_lbl_playstate 是【跨页共享】的 —— 首页的「播放中 · 台名」
 *   （第 1752 行）和播放页的「播放中/已暂停」（第 2753 行）都往它写。
 *   所以夜间页必须用自己的句柄，否则两个页面的定时器互相覆盖文字
 *   （这正是兰兰看到「夜间页的当前播放显示不对」的原因之一）。*/
static lv_obj_t *s_lbl_night_state; /* 夜间：状态（10-04 独立，专用） */
/* ★★ v1.35：夜间页改成【进页才建、离开就删】（同第 8 页 WiFi 设置）。
 *   真机崩在这：兰兰报「点击睡眠页就重启」。
 *   根因 = 夜间页最重（40 根装饰竖条 + 8 颗胶囊×2 + 时钟 + 滑块 ≈ 62 个对象），
 *   一显示就要为每个对象分配 lv_draw_task_t，而池只剩 13.7 KB：
 *       lv_draw_add_task(): new_task = lv_malloc_zeroed(...);   → 返 NULL
 *                          LV_ASSERT_MALLOC(new_task);         → release 下【空操作】
 *                          new_task->area = *coords;           → 往 NULL 写 → EXCVADDR=0x08
 *   ⚠️ 和第 8 页一样的病、同一个坑位（lv_draw_add_task），只是这次更重。
 *   ⚠️ 钩子必须挂在 goto_page 里，不能挂在返回键上（见那里注释）。*/
static bool s_night_built;
static lv_obj_t *s_lbl_playname;  /* 播放页：当前曲名（大字符集） */
static lv_obj_t *s_lbl_playbtn;   /* 播放页：中间键文字 */
static lv_obj_t *s_lbl_favbtn;    /* 播放页：☆/★ 收藏键 */
static lv_obj_t *s_btn_speed;     /* 播放页：倍速键（第二十二次）*/
static lv_obj_t *s_lbl_speed;     /* 播放页：倍速键上的「倍速 1.2X」*/
static lv_obj_t *s_playbtn_lbl;   /* 首页：暂停圆钮上的文字 */
static lv_obj_t *s_lbl_topstat;   /* 顶栏右侧时钟（HH:MM）*/
static lv_obj_t *s_ic_wifi;       /* 顶栏：WiFi 图标（LV_SYMBOL_WIFI）*/
static lv_obj_t *s_ic_sd;         /* 顶栏：内存卡图标（LV_SYMBOL_SD_CARD）*/
static lv_obj_t *s_ic_batt;       /* 顶栏：电池图标（10-04；只有图标无数字）*/
static int       s_batt_shown_pct = -2;   /* 上次显示的电量档，-2=还没刷过 */
static lv_obj_t *s_lbl_sdinfo;    /* 顶栏：内存卡容量文字「29 G」——唯一一处*/

/* ★ 播放页的返回键回哪一页（兰兰 10-03：「要做到从哪里来能够回到哪里」）。
 *   点首页的台 = 0；点发现页的台 = 1；点全屏分类页/收藏页的台 = 6。
 *   首页大卡进播放页时也记 0。SD 页点文件进播放页记 5。*/
static int       s_play_from = 0;

static lv_timer_t *s_splash_timer;   /* 停留计时（3.5 s）*/
static lv_timer_t *s_spl_timer;      /* 动画推送（60 ms）*/
/* ★ 10-04 第二十九次：splash 只允许撤一次。
 *   声明必须放在这儿（splash_build 在 1800 行就要把它清零，
 *   而定义在文件后段的 splash_done_cb 里）。*/
static bool s_splash_done;
static lv_timer_t *s_stat_timer;
static lv_timer_t *s_clock_timer;
static lv_timer_t *s_date_timer;    /* 夜间页日期（10-04 新增） */
static lv_timer_t *s_nstate_timer;  /* 夜间页播放状态（10-04 新增） */
static lv_timer_t *s_sleep_timer;   /* 睡眠定时倒计时（10-04 新增） */
static lv_timer_t *s_spec_timer;   /* 100 ms：真频谱专用，见 spec_timer_cb */
static lv_timer_t *s_seek_timer;   /* 20 ms：进度条拖动专用，见 seek_timer_cb */
/* ★ 第二十四次：s_wifi_timer（WiFi 页删/建待办）已删 —— 屏幕键盘去掉了。*/

static bool s_playing = true;

/* SD「全卡音频 N 首」缓存。
 * ★ 为什么必须缓存：app_sd_count_audio() 是递归 + 逐条 stat，
 *   在 32 GB 塞满音频的卡上要扫几千条目录项。而 ui_sd_page_refresh()
 *   每次切子目录/返回上一级都会调用 —— 不缓存的话每点一下就卡几秒。
 *   策略：只在「从首页进入 SD 页」时置脏，其余刷新一律用缓存值。*/
static bool s_sd_count_dirty = true;
static int  s_sd_audio_total = -1;

/* ---- 第 3 页「本地播放」的句柄 ----
 * ★ player_refresh() 定义在文件后半段，但 SD 页点击（sd_row_clicked）
 *   要用到它，所以先在这里前向声明，避免把函数搬来搬去。*/
static void player_refresh(void);

/* 同理：播放页的真频谱重画与返回键文案（定义在文件后半段那一节）*/
static void spec_refresh(void);
static const char *play_from_name(void);

/* 播放页返回键上的文字（定义在 page_play_build 那一节，
 *   但 player_refresh 要在切页时同步它 —— 所以这里先声明）*/
static lv_obj_t *s_lbl_playback;

/* 当前时刻（分钟）。定义在文件末尾那一节（顶栏与夜间页共用）。*/
static int now_minutes(void);

/* ★ 按【UTF-8 字符】截断（定义在下面 SD 页那一节）。
 *   首页台名要截断，所以先在这里声明一份，别把函数搬来搬去。*/
static void utf8_copy_n(char *dst, size_t dstsz, const char *src, int max_chars);

/* 播放页左半。10-03 第十一次：200x200 的进度环删掉了（兰兰「圆圈在，频谱就展不开」），
 * 现在是「曲名 + 大频谱 + 底部 6 px 细进度条」。*/
/* ★★ 进度条：可拖动（本地音频），直播时自动变成只读底线
 * ============================================================
 *  ★ 兰兰 10-03 第十四次：「播放页左下方有个横条（应该是进度条吧）
 *    确实是只能显示进度不能拖动，本地音乐播放页也是一样的不能拖动」
 *
 *  我上一版把它做成了【两个纯 box】（底槽 + 绿色前景改宽度），
 *  那是「只显示」不是「可拖」—— 兰兰说得对。
 *
 *  为什么不用 lv_slider 现成的：
 *    lv_slider 自带 style（轨道 + 指示器 + 旋钮）三套，每个都是 lv_obj，
 *    一个就吃掉几 KB；池只剩 21 KB。lv_arc 更贵（10-03 第十一次已经因为它
 *    把频谱挤成 20 px 而删掉了）。所以还是 box 拼，但补上【触摸事件】。
 *
 *  拖动时的三步：
 *    ① 按下 → 记下当时的总时长，按 x 坐标算出目标秒，显示预览时间
 *    ② 拖动中 → 实时改前景宽度 + 刷新预览时间（不 seek，松手才 seek）
 *    ③ 松手 → app_radio_seek(目标秒)，后端 fseek + 重建解码器（约 200 ms）
 *
 *  直播（is_url）不能跳：这时把触摸回调直接不挂，进度条只显示满格绿线。
 */
/* 播放页「横向账」（兰兰 10-03 要求频谱比首页略大即可）：
 *   曲名行 2~22 ｜ 时间行 24~46 ｜ 频谱区 y=58 起、高 138（58~196）
 *   进度条 y=204、高 20（204~224）｜ 提示行 228~244
 * ★ 这几个值被 seek_sec_at()（算「手指点的是不是进度条」）用到，
 *   所以必须是编译期常量，不能是文件后段的 static 变量。*/
#define SPEC_H  138
#define SPEC_Y  58
/* ★★★ 10-03 第十八次重排播放页左半（兰兰：「快进10有了，但10秒太短，
 *   需要分钟级别的，2分钟，10分钟」＋「本地拖动还是不行」）。
 *
 *   第十六次版面：[退10][条 168][进10] 一行 —— 跳转键只能塞两个。
 *   第十八次改成【两行】，跳转键从 2 个变 4 个，条恢复整宽：
 *     上行 y=186..206：4 个跳转键，每个 58×20，缝 4
 *              4 → 62 → 120 → 178 → 236..240（4×58 + 3×4 = 244 ✓）
 *              文案：退2  退10  进10  进2
 *     下行 y=210..238：整宽进度条 244 宽 × 28 高
 *              ★ 28 px 厚：20 px 那版兰兰还是说按不准
 *     热区 y=200..250（50 高），盖满 244 宽 —— 上面是键，下面才是条，不冲突
 *
 *   为什么给到 10 分钟：本地音乐常见 3~8 分钟，±10 s 一次只能跳过一句词；
 *   ±2 min 才是「跳过一整段」的量级。四个键覆盖「微调 / 大段」两档。
 */
/* ★★★ 2026-10-04 第二十次重排（兰兰：「快进10分钟有效，但经常要触碰几次才有效，
 *   2分钟无反应可能不灵敏」）。
 *
 *   ❶ 跳转键高度 20 → 30（SEEK_STEP_H）：一整行 30 高，按得准。
 *   ❷ 事件从 LV_EVENT_CLICKED 改成 **LV_EVENT_PRESSED**（按下即跳）：
 *      CLICKED 要「按下 + 在同一对象内松手」，中途只要 LVGL 判定成滚动就作废。
 *      兰兰「要碰几次」就是这个。改成按下即跳就没有那个窗口。
 *   ❸ 按钮不再挂 ev_press（它按下时改 opa 会触发重排，
 *      对象坐标变了 LVGL 就转成滚动 —— 正是 ❷ 的触发源）。
 *
 *   版面（CONT 高 282，左半 x=4..248 宽 244）：
 *      跳转键行 y=182..212（4 × 58 宽，缝 4 = 4×58+3×4 = 244 ✓）
 *      进度条   y=216..244（28 高）
 *      提示行   y=248..262
 *   热区 216..244 再上下各留 2 px ⇒ SEEK_HIT_Y=214，HIT_H=32
 */
/* ★★★ 2026-10-04 第二十一次重排（兰兰：「快进快退按键挡住了频谱最下端，
 *   不美观，可以把快进按键放在拖动条下面，拖动条可以设置个拖动手柄」）。
 *
 *   问题：第二十次把跳转键放在 y=182..212，而频谱是 58~196
 *   ⇒ 键的上半截 182~196 正好压在频谱最下面 14 px 上（兰兰说的「挡住最下端」）。
 *
 *   新版面（CONT 高 282，左半 x=4..248 宽 244）：
 *      提示行   y=44..56（频谱上方那行，见 s_lbl_level）
 *      频谱     y=58..196
 *      进度条   y=200..226（26 高）
 *      跳转键行 y=234..264（4 × 58 宽，缝 4 = 4×58+3×4 = 244 ✓）
 *      底部余   264..282 空着
 *   ⇒ 键在条【下面】，两者不再抢同一条空间。
 *
 *   ★★ 拖动手柄（兰兰：「拖动条可以设置个拖动手柄，这样或许有效」）：
 *     之前的 knob 只有 4 px 宽、且只在拖动时才显形（LV_OPA_TRANSP），
 *     看起来就是「条上有一道白线」，不知道能拖。
 *     现在改成 18×34 的圆角把手、**常显**（不透明），
 *     停在当前进度位置；直播时整条变灰、把手藏起来（不能拖）。
 *     把手本身也参与命中：seek_sec_at() 算的是「手指 x → 秒」，
 *     所以按在把手两侧 Anywhere 在条内都生效，不要求精确抓住把手。
 */
#define SEEK_BAR_Y   200             /* 进度条顶 */
#define SEEK_BAR_W   244             /* 整宽 */
#define SEEK_BAR_X   4
#define SEEK_BAR_H   26              /* 26 px 厚 */
#define SEEK_KNOB_W  18              /* 拖动手柄宽（比条高 34 > 26，上下各露 4）*/
#define SEEK_KNOB_H  34
#define SEEK_STEP_W  58              /* 4 个键：4×58 + 3×4 = 244 */
#define SEEK_STEP_H  30
#define SEEK_STEP_GAP 4
#define SEEK_ROW1_Y  234             /* ★ 跳转键行移到进度条【下面】 */
/* 触摸热区：盖住条 200..226，上下各留 5 px ⇒ 195..231（36 高）。
 * ⚠️ 上边界 195 会在频谱区（58..196）里压 1 px，但频谱不收事件，
 *   且 s_seek_hit 建在 p 上、频谱柱也建在 p 上 ⇒ 谁在上面谁收事件。
 *   为彻底避开，把热区压到 197..231（34 高），与频谱底 196 差 1 px。*/
#define SEEK_HIT_Y 197
#define SEEK_HIT_H 34

static lv_obj_t *s_arc_play;     /* 进度条底槽（灰色那条，只显示，不收事件）*/
static lv_obj_t *s_prog_play;    /* 进度条前景（绿色，宽 = 进度比例 × SEEK_BAR_W）*/
static lv_obj_t *s_seek_hit;     /* 透明触摸热区，事件挂这里 */
static lv_obj_t *s_seek_knob;    /* 拖动时的白色游标（4 px 宽）*/
static lv_obj_t *s_lbl_level;    /* 进度条下面那行提示（可拖 / 直播只读）*/
static int       s_seek_bar_w = SEEK_BAR_W;   /* 进度条总宽（换算拖动比例用）*/
static bool      s_seek_drag   = false; /* 正在拖 */
static int       s_seek_target = 0;     /* 拖动中的目标秒 */
/* ★ 第二十次：本轮是否由 seek_drag_poll() 接管了这次拖动。
 *   接管了就由轮询负责松手提交，事件回调只兜底（见 ev_seek_release）。*/
static bool      s_seek_polled = false;

static void fmt_mmss(int sec, char *out, size_t n);   /* 定义在文件后段 */
static void tap_note(const char *name);               /* 定义在文件后段 */
static void batt_indicator_refresh(void);             /* 定义在 4048 行 */
static void stat_refresh(void);                       /* 定义在 4127 行 */

/* 已播/总时长标签（定义在「播放页控件」那一段，seek 预览要提前改它）*/
static lv_obj_t *s_np_time;

/* 取当前触摸点（LVGL v9 是两参数版：indev + lv_point_t）
 *
 * ★★★ 2026-10-04 第二十一次：这里必须把【屏幕绝对坐标】换成
 *   【播放页容器内坐标】，否则拖动永远判定失败。
 *
 *   真相：lv_indev_get_point() 给的是整个屏幕（480×320）的绝对坐标，
 *   而 seek_sec_at() 里的 SEEK_BAR_X/SEEK_HIT_Y 全是【页内】坐标
 *   （页容器 p 摆在 (CONT_X, CONT_Y) = (56, 38)）。
 *   差值 x 少 56、y 少 38 —— 于是：
 *     · 手指按在条上（页内 y≈230 ⇒ 绝对 y≈268），
 *       判定 `py >= 214 && py <= 246` 直接不成立 ⇒ 永远不启拖；
 *     · 横坐标也偏 56 ⇒ 即便过了纵向判定，取到的秒数也是错的。
 *   ⇒ 第十八~二十版我一直在调热区高度（20→28→32→50）、调 LV_DIR_NONE、
 *     改用轮询自管手势 —— 全是在错的坐标系里打转，
 *     所以兰兰连报四次「本地拖动依然无效」。
 *
 *   ★ 教训：凡是自己读 indev 坐标做判定（不走 LVGL 事件派发），
 *     必须先做一次「绝对 → 页内」的换算。这是自管手势的固定成本。*/
static void seek_get_point(lv_indev_t *ind, int *px, int *py)
{
    lv_point_t pt;
    *px = *py = 0;
    if (!ind) return;
    lv_indev_get_point(ind, &pt);
    *px = pt.x - CONT_X;      /* 减去左侧导航栏宽 */
    *py = pt.y - CONT_Y;      /* 减去顶栏高 */
}

static void seek_preview_show(int sec)
{
    if (!s_np_time) return;
    int tot = app_radio_total_s();
    char a[16], b[16];
    fmt_mmss(sec, a, sizeof(a));
    if (tot > 0) { fmt_mmss(tot, b, sizeof(b)); lv_label_set_text_fmt(s_np_time, "%s / %s", a, b); }
    else          { lv_label_set_text(s_np_time, a); }
}

/* 触摸坐标 → 秒。返回 -1 表示超出范围（点在条外/无效）
 * ★ 第二十二次：拖动中（s_seek_drag）时【纵向不再设限】。
 *   兰兰报「拖动有作用，但不灵敏」——手指按住手柄后会自然上下浮动
 *   几像素（手的抖动），原来 `py <= SEEK_HIT_Y+SEEK_HIT_H+2` 一越界
 *   就 return -1，手柄立刻「粘住」不动了 ⇒ 主观上就是「不灵敏」。
 *   既然已经起拖，纵向宽容一点；横向仍然按 x 算，位置不会乱。*/
static int seek_sec_at(int px, int py)
{
    int tot = app_radio_total_s();
    if (tot <= 0) return -1;
    if (!s_seek_drag) {
        /* 热区上下各再留 2 px 余量（手指粗）*/
        if (py < SEEK_HIT_Y - 2 || py > SEEK_HIT_Y + SEEK_HIT_H + 2) return -1;
    }
    int w = s_seek_bar_w;
    int d = px - SEEK_BAR_X;              /* 左边 4 px */
    if (d < 0) d = 0;
    if (d > w) d = w;
    return (int)((int64_t)d * tot / w);
}

/* 把拖动手柄放到「离 px 最近的合法位置」。
 * ★ 手柄比条高（34 > 26），所以要垂直居中在条上：y = 条顶 - (手柄高-条高)/2。
 *   横向要【夹】在条内：手柄宽 18，若按最右边，中心不能超过 w-9，
 *   否则手柄有一半跑到条外面，看着像脱靶。*/
static void seek_knob_place(int px)
{
    if (!s_seek_knob) return;
    int d = px - SEEK_BAR_X;
    int half = SEEK_KNOB_W / 2;
    if (d < half)                d = half;
    if (d > s_seek_bar_w - half)  d = s_seek_bar_w - half;
    lv_obj_set_pos(s_seek_knob, SEEK_BAR_X + d - half,
                   SEEK_BAR_Y - (SEEK_KNOB_H - SEEK_BAR_H) / 2);
}

/* 拖动手柄显形 / 藏形。
 * ★ 第二十一次：把手改成【常显】——之前只在拖动时显形，
 *   平时完全透明，看起来就是「一条不能碰的绿线」，
 *   兰兰自然不会想到去拖它。*/
static void seek_knob_show(int px, bool on)
{
    if (!s_seek_knob) return;
    lv_obj_set_style_bg_opa(s_seek_knob, on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    if (on) seek_knob_place(px);
}

/* 按下（事件回调）。★ 只是给轮询【打提前量】：
 *   轮询每 100 ms 一次，有一拍延迟；按下瞬间先把游标显出来，手感才跟手。
 *   真正的起拖判定仍在 seek_drag_poll()（那边能读 indev 状态，判得更准）。*/
static void ev_seek_press(lv_event_t *e)
{
    int px = 0, py = 0;
    seek_get_point(lv_event_get_indev(e), &px, &py);
    if (s_seek_drag) return;          /* 轮询已经起拖了，别重复 */
    if (!app_radio_can_seek()) {
        tap_note(app_radio_is_stream() ? "Cannot seek (live)" : "Duration unavailable");
        return;
    }
    int sec = seek_sec_at(px, py);
    if (sec < 0) return;
    s_seek_drag   = true;
    s_seek_target = sec;
    ESP_LOGI(TAG, "seek DRAG-START -> %d s (tot=%d)", sec, app_radio_total_s());
    seek_knob_show(px, true);
    seek_preview_show(sec);
}

static void ev_seek_release(lv_event_t *e)
{
    (void)e;
    /* 真正的提交在 seek_drag_poll()（它看 indev 状态才知道手指已抬起）。
     * 这里只在「轮询还没接上」时兜一次，避免极端情况下拖了没反应。*/
    if (s_seek_drag && !s_seek_polled) {
        s_seek_drag = false;
        if (app_radio_seek(s_seek_target) == ESP_OK) {
            tap_note("Seek");
            ESP_LOGI(TAG, "seek request(fallback): %d s", s_seek_target);
        }
        player_refresh();
    }
}

/* 拖动：完全自管手势，不依赖 LVGL 的 PRESSING 事件。
 * ------------------------------------------------------------
 * ★★ 兰兰 10-04 报「本地进度条依然无效」，这是本轮第一号问题。
 *
 * 为什么不能靠 LV_EVENT_PRESSING（第十八~十九版的做法）：
 *   热区设了 LV_DIR_NONE（必须，否则父容器抢走手势），
 *   而 LVGL 只有在【对象被选为滚动对象】时才会持续派发 PRESSING；
 *   LV_DIR_NONE 的对象永远不是滚动对象 ⇒ PRESSING 永远不来。
 *   ⇒ 第十八版「靠 PRESSING 更新预览」从一开始就不可能工作。
 *
 * 现在怎么做（只依赖 LVGL 一定会给我的东西：indev 的状态与坐标）：
 *   按下/松手 —— 靠 indev 状态在轮询里自己认（PRESSED→起拖，RELEASED→提交），
 *                 事件回调只作为补充（有些屏按下时轮询刚好错过一拍）。
 *   指到哪    —— 每 100 ms 读一次 indev 当前点，直接改前景宽度。
 *   ⇒ 三件事都不再与 LVGL 的滚动判定竞争。
 */
static void seek_drag_poll(void)
{
    if (!s_seek_hit) return;
    lv_indev_t *ind = lv_indev_get_act();
    if (!ind) return;
    int px = 0, py = 0;
    seek_get_point(ind, &px, &py);
    bool down = (lv_indev_get_state(ind) == LV_INDEV_STATE_PRESSED);
    bool in_bar = (py >= SEEK_HIT_Y && py <= SEEK_HIT_Y + SEEK_HIT_H);

    /* ---- ① 起拖：手指按下且落在条上 ---- */
    if (down && in_bar && !s_seek_drag) {
        if (!app_radio_can_seek()) {
            /* 只提示一次，不反复打断 */
            tap_note(app_radio_is_stream() ? "Cannot seek (live)" : "Duration unavailable");
            return;
        }
        int sec = seek_sec_at(px, py);
        if (sec >= 0) {
            s_seek_drag   = true;
            s_seek_polled = true;      /* 本次拖动由轮询接管 */
            s_seek_target = sec;
            seek_preview_show(sec);
            ESP_LOGI(TAG, "seek DRAG-START -> %d s (tot=%d)", sec, app_radio_total_s());
        }
    }

    if (!s_seek_drag) return;

    /* ---- ② 松手：提交 ---- */
    if (!down) {
        s_seek_drag   = false;
        s_seek_polled = false;
        if (app_radio_seek(s_seek_target) == ESP_OK) {
            tap_note("Seek");
            ESP_LOGI(TAG, "seek request: %d s", s_seek_target);
        } else {
            tap_note("Cannot seek");
            ESP_LOGW(TAG, "seek refused: target=%d can_seek=%d total=%d",
                     s_seek_target, (int)app_radio_can_seek(), app_radio_total_s());
        }
        player_refresh();
        return;
    }

    /* ---- ③ 拖动中：前景与手柄跟着手指走（不 seek）---- */
    int sec = seek_sec_at(px, py);
    if (sec < 0) return;
    int tot = app_radio_total_s();
    int w = (int)((int64_t)sec * s_seek_bar_w / (tot > 0 ? tot : 1));
    if (w != lv_obj_get_width(s_prog_play)) {
        lv_obj_set_width(s_prog_play, w > 0 ? w : 1);
    }
    seek_knob_show(px, true);
    if (sec != s_seek_target) {
        s_seek_target = sec;
        seek_preview_show(sec);
    }
}

/* ★ 第二十次：旧的 ev_seek_pressing / ev_seek_release 已删除。
 *   前者依赖 LV_EVENT_PRESSING —— 而 LV_DIR_NONE 的对象永远不会被
 *   LVGL 选为滚动对象 ⇒ PRESSING 根本不会派发，留着是死代码。
 *   后者的提交职责已并入 seek_drag_poll()（它看 indev 状态更准）。
 */

/* ============================================================
 *  ★★★ 跳转键：退2分 / 退10分 / 进10分 / 进2分（第二十次改「按下即跳」）
 * ============================================================
 * 兰兰 10-03：「快进10有了，但是10秒太短，需要分钟级别的，2分钟，10分钟」。
 * 兰兰 10-04：「快进10分钟有效，但经常要触碰几次才有效，2分钟无反应」。
 *   事件已从 LV_EVENT_CLICKED 改成 LV_EVENT_PRESSED（按下即跳）——
 *   根因与四个按钮的建法见 page_play_build 里的注释。
 */
static void ev_seek_step(lv_event_t *e)
{
    int delta = (int)(intptr_t)lv_event_get_user_data(e);   /* 正负 120 或 600 秒 */
    if (!app_radio_can_seek()) {
        tap_note("Cannot seek (live)");
        return;
    }
    int cur = app_radio_elapsed_s();
    int tgt = cur + delta;
    if (tgt < 0) tgt = 0;
    int tot = app_radio_total_s();
    if (tgt >= tot) tgt = tot - 1;

    if (app_radio_seek(tgt) != ESP_OK) {
        tap_note("Cannot seek");
        return;
    }
    /* ⚠ 缓冲给 32 而不是 16：gcc 的 -Werror=format-truncation 按最坏情况算
     *   「快进 」(7 字节) + int 最长 11 字节 + 「 秒」(4 字节) 会超过 16。
     *   给足 32 就不会判截断（速查卡铁律：报截断时加大数组没用，要算准）。*/
    char msg[32];
    int  a = delta < 0 ? -delta : delta;
    if (a >= 60) snprintf(msg, sizeof(msg), "%s%d:%02d",
                          delta > 0 ? "Fwd" : "Rew", a / 60, a % 60);
    else         snprintf(msg, sizeof(msg), "%s%.10ds", delta > 0 ? "Fwd " : "Rew ", a);
    tap_note(msg);
    ESP_LOGI(TAG, "step %+d s -> %d/%d", delta, tgt, tot);
    player_refresh();
}

/* ============================================================
 *  倍速键（第二十二次）—— 1.0X → 1.2X → 1.4X → 1.0X 循环
 * ============================================================
 * ★ 为什么挂 LV_EVENT_PRESSED 而不是 CLICKED：
 *   与跳转键同一个理由 —— CLICKED 要「按下 + 在同一对象内松手」，
 *   中途被 LVGL 判成滚动就作废，表现为「要碰几次」。
 *   倍速是「立刻要听到效果」的操作，按下即切最合适。
 *
 * ★ 切速会重开 codec（约几十毫秒静默），所以
 *   【没在播的时候不给切】—— 免得按了没声音、还以为机器坏了。
 *   空闲时按它就提示一句。*/
static void ev_speed_cycle(lv_event_t *e)
{
    (void)e;
    if (!app_radio_is_playing()) {
        tap_note("Play first, then set speed");
        return;
    }
    int cur = app_radio_speed();
    int nxt = (cur < 110) ? 120 : ((cur < 130) ? 140 : 100);
    app_radio_set_speed(nxt);

    if (s_lbl_speed) {
        /* 键宽 78，11px 六个字符（倍速 1.4X = 8 字符 ≈ 56 px）放得下 */
        lv_label_set_text_fmt(s_lbl_speed, "Speed %d.%dx", nxt / 100, (nxt / 10) % 10);
        /* 非 1.0X 时把字换成琥珀色，一眼知道「现在在加速」——
         * 不然过一会儿就忘了自己刚才按过。*/
        lv_obj_set_style_text_color(s_lbl_speed, (nxt == 100) ? C_GREEN : C_AMBER, 0);
    }
    char msg[24];
    snprintf(msg, sizeof(msg), "%.10s %d.%dX",
             nxt == 100 ? "Back to normal" : "Switched to", nxt / 100, (nxt / 10) % 10);
    tap_note(msg);
}

static lv_obj_t *s_np_name;      /* 曲名（页面顶部，两行）*/
/*static lv_obj_t *s_np_time;*/      /* 已播 / 总时长 —— 已在文件头为 seek 预览提前声明 */
static lv_obj_t *s_np_hint;      /* 状态提示 */
static lv_obj_t *s_np_meta;      /* 右卡：格式信息 */
static lv_obj_t *s_np_state;     /* 右卡：状态 / 错误原因 */

/* 当前本地播放所在的目录（「上一个 / 下一个」要在同一目录里换曲）。
 * ★ 只存目录名（< 256 字节），不存整个播放列表 —— 一个节目目录几十上百首、
 *   每首文件名最长 255 字节，缓存列表要几十 KB 内存，不值得。*/
static char s_local_dir[256] = "/sdcard";

/* ============================================================
 *  小工具
 * ============================================================ */
static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h, lv_color_t color, int radius)
{
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, color, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
}

static lv_obj_t *label(lv_obj_t *parent, int x, int y, const lv_font_t *font, lv_color_t color,
                       const char *text)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_label_set_text(l, text);
    lv_obj_set_pos(l, x, y);
    return l;
}

/* ★ 前向声明：splash_build 在文件前段，而 now_minutes 定义在后段。
 *   （第 10-04 那次「顶栏时钟死 label」就是前向声明漏了、
 *     结果 stat_refresh 那段代码白写了。）*/
static int  now_minutes(void);
static void splash_done_cb(lv_timer_t *t);

/* 拾声 标识：五根声波柱 */
static lv_obj_t *logo_mark(lv_obj_t *parent, int x, int y, int h, lv_color_t color)
{
    static const int wgt[5] = {40, 70, 100, 66, 46};
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_remove_style_all(c);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, 30, h);
    lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < 5; i++) {
        int bh = h * wgt[i] / 100;
        if (bh < 3) bh = 3;
        box(c, i * 6, (h - bh) / 2, 4, bh, color, 2);
    }
    return c;
}

/* 频谱柱 —— ★ 10-03 起改成【真频谱】。
 * 原来这里画的是一组写死的假柱（static const int wgt[13]），
 * 兰兰要「动态频谱」，就把首页这处也接到 app_radio 的 Goertzel 分析上。
 * 数据源是解码后的 PCM，所以首页那张卡上的柱子和耳朵里听到的声音一致。*/
#define HOME_SPEC_N  13          /* 首页卡上摆几根（比播放页密一点，140 px 宽）*/
static lv_obj_t *s_home_bar[HOME_SPEC_N];
static int       s_home_bw = 8, s_home_gap = 2, s_home_h = 18, s_home_y = 80, s_home_x = 14;

static void spectrum(lv_obj_t *parent, int x, int y, int w, int h, lv_color_t color)
{
    s_home_x = x;
    s_home_y = y;
    s_home_h = h;
    s_home_bw = (w - (HOME_SPEC_N - 1) * s_home_gap) / HOME_SPEC_N;
    if (s_home_bw < 3) s_home_bw = 3;
    for (int i = 0; i < HOME_SPEC_N; i++) {
        /* 初始 3px 底线：不放的时候也别是一排空白 */
        s_home_bar[i] = box(parent, x + i * (s_home_bw + s_home_gap),
                            y + h - 3, s_home_bw, 3, color, 2);
    }
}

/* 首页那排柱子按真频谱重画（每 12 根取 1 根，13 根对应 12 个频段）*/
static void home_spec_refresh(void)
{
    int n = app_radio_spectrum_bins();
    for (int i = 0; i < HOME_SPEC_N; i++) {
        /* 13 根对 12 段：最后一根跟着最高频段走（多一根显得不空）*/
        int idx = (i < n) ? i : (n - 1);
        int v = (n > 0) ? app_radio_spectrum(idx) : 0;
        if (!app_radio_is_playing() || app_radio_is_paused()) v = 0;
        int bh = s_home_h * v / 100;
        if (bh < 3) bh = 3;
        if (bh > s_home_h) bh = s_home_h;
        lv_obj_set_size(s_home_bar[i], s_home_bw, bh);
        lv_obj_set_pos(s_home_bar[i],
                       s_home_x + i * (s_home_bw + s_home_gap),
                       s_home_y + s_home_h - bh);
    }
}

/* ============================================================
 *  通用「可点控件」
 * ============================================================ */
static void tap_note(const char *name)
{
    int rx, ry, lx, ly;
    bool pr;
    app_touch_get_last(&rx, &ry, &lx, &ly, &pr);
    ESP_LOGI(TAG, "tap: %s  lv=(%d,%d)", name, lx, ly);
}

static void ev_press(lv_event_t *e)
{
    lv_obj_set_style_opa(lv_event_get_target(e), LV_OPA_60, 0);
}

static void ev_release(lv_event_t *e)
{
    lv_obj_set_style_opa(lv_event_get_target(e), LV_OPA_COVER, 0);
}

static void ev_named_tap(lv_event_t *e)
{
    const char *tag = (const char *)lv_event_get_user_data(e);
    tap_note(tag ? tag : "?");
}

/* 播放提示音：不接任何音源也能验证「喇叭会响」 */
static void ev_beep(lv_event_t *e)
{
    (void)e;
    tap_note("Playing chime");
    app_audio_beep_async();
}

/* ============================================================
 *  关机（10-04 新增）
 * ============================================================
 *  ⚠️ 为什么必须【二次确认】而不是按了直接关：
 *   app_pwr_shutdown() 不返回（进 deep sleep），
 *   状态页这几个按钮是挨着的（提示音 / 手机配网 / 关机），
 *   一次误触就从「有声可玩」变成「黑屏，要按 BOOT 键才回来」——
 *   对一台还没完全熟悉的机器来说体验很差。
 *   ⇒ 第一次点把文字换成「再点一次关机」，5 秒内不点就自动还原。
 *
 *  ★ 关机后怎么开机：**按住 BOOT 键**，等屏幕亮起再松开。
 *    ⚠️ 10-04 兰兰实测「关机后马上又自己开机」——那是 v1.32 的原理性错误
 *    （EXT1 判电平不判边沿，ANY_HIGH 会在进深睡那一刻立刻自唤醒），
 *    已改成 ANY_LOW。代价见 app_pwr.h 顶部：唤醒时正按着 BOOT，
 *    ESP32 采样 IO0 为低会判成下载模式 ⇒ 屏幕亮起前别松手；
 *    万一没刷出来，松开再按一下 RESET 键即可。*/
static lv_timer_t *s_sdconfirm_timer;   /* 二次确认的自动还原计时 */
static lv_obj_t   *s_btn_power;         /* 关机键本体 */
static lv_obj_t   *s_lbl_power;         /* 关机键上的文字 */

static void power_confirm_timeout_cb(lv_timer_t *t)
{
    lv_timer_del(t);
    s_sdconfirm_timer = NULL;      /* 已自己删，置 NULL 防二次删除 */
    if (s_lbl_power) {
        lv_label_set_text(s_lbl_power, "Power off");
        lv_obj_set_style_text_color(s_lbl_power, lv_color_hex(0xF87171), 0);
    }
}

static void ev_power(lv_event_t *e)
{
    (void)e;
    (void)s_btn_power;

    /* 已经在「等第二次」⇒ 这次就是确认，直接关。*/
    if (s_sdconfirm_timer) {
        ESP_LOGI(TAG, "UI: 二次确认通过 → 关机");
        tap_note("Power off");
        if (s_lbl_power) {
            lv_label_set_text(s_lbl_power, "Powering off...");
            lv_obj_update_layout(s_lbl_power);
        }
        /* 让 LVGL 把「正在关机…」这几个字真的画出来再黑屏。
         * ★ 这里【不能】用 lv_timer —— app_pwr_shutdown() 不返回，
         *   建一个 600 ms 的定时器出来还没等到就进 deep sleep 了，纯浪费。
         *   而且定时器回调在 deep sleep 前必须停掉，留着反而是隐患。
         *   正确做法：在当前（LVGL 任务）上下文里同步延时并让 LVGL 跑一次渲染。*/
        lv_timer_handler();          /* 立刻处理一次布局+渲染 */
        vTaskDelay(pdMS_TO_TICKS(350));
        app_pwr_shutdown();          /* 不返回 */
        return;
    }

    /* 第一次点：换文案 + 5 秒倒计时 */
    tap_note("Power off? Tap again");
    if (s_lbl_power) {
        lv_label_set_text(s_lbl_power, "Tap again to power off");
        lv_obj_set_style_text_color(s_lbl_power, C_AMBER, 0);
    }
    s_sdconfirm_timer = lv_timer_create(power_confirm_timeout_cb, 5000, NULL);
}

/* 离开状态页时把「再点一次」状态清掉，
 * 不然用户在别的页逛一圈回来，按钮还写着「再点一次关机」，
 * 一下就真关了 —— 反而更危险。
 * ⚠️ 实际钩子挂在 goto_page() 里（那里才知道页面要换成哪一页），
 *   这个函数只留权责说明，不单独调用。*/

static lv_obj_t *tick(lv_obj_t *parent, int x, int y, int w, int h, lv_color_t bg, int radius,
                      const char *tag)
{
    lv_obj_t *o = box(parent, x, y, w, h, bg, radius);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(o, ev_press, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(o, ev_release, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(o, ev_release, LV_EVENT_PRESS_LOST, NULL);
    if (tag) lv_obj_add_event_cb(o, ev_named_tap, LV_EVENT_CLICKED, (void *)tag);
    return o;
}

/* ============================================================
 *  单选组 —— 胶囊 / 列表行共用
 * ============================================================ */
#define GRP_MAX 12
#define GRP_FG  2

typedef struct {
    lv_obj_t *bg[GRP_MAX];
    lv_obj_t *fg[GRP_MAX][GRP_FG];
    int        fg_n;
    int        cnt;
    int        sel;
    lv_color_t bg_on, bg_off;
    lv_color_t fg_on, fg_off;
    const char *name;
    void (*on_change)(int idx);   /* 选中变化后的回调（可选）*/
} radio_grp_t;

typedef struct { radio_grp_t *g; int idx; } grp_arg_t;

static grp_arg_t s_arg[32];
static int       s_arg_n;

static grp_arg_t *arg_of(radio_grp_t *g, int idx)
{
    if (s_arg_n >= (int)(sizeof(s_arg) / sizeof(s_arg[0]))) s_arg_n = 0;
    s_arg[s_arg_n].g = g;
    s_arg[s_arg_n].idx = idx;
    return &s_arg[s_arg_n++];
}

static void grp_apply(radio_grp_t *g)
{
    for (int i = 0; i < g->cnt; i++) {
        bool on = (i == g->sel);
        if (g->bg[i]) lv_obj_set_style_bg_color(g->bg[i], on ? g->bg_on : g->bg_off, 0);
        for (int j = 0; j < g->fg_n; j++) {
            if (g->fg[i][j]) {
                lv_obj_set_style_text_color(g->fg[i][j], on ? g->fg_on : g->fg_off, 0);
            }
        }
    }
}

static void grp_clicked(lv_event_t *e)
{
    grp_arg_t *a = (grp_arg_t *)lv_event_get_user_data(e);
    if (!a || !a->g) return;
    a->g->sel = a->idx;
    grp_apply(a->g);
    tap_note(a->g->name ? a->g->name : "Menu");
    if (a->g->on_change) a->g->on_change(a->idx);
}

static lv_obj_t *grp_chip(radio_grp_t *g, lv_obj_t *parent, int x, int y, int w, int h,
                          int radius, const lv_font_t *font, const char *text)
{
    int i = g->cnt++;
    if (i >= GRP_MAX) return NULL;
    lv_obj_t *o = tick(parent, x, y, w, h, g->bg_off, radius, NULL);
    lv_obj_add_event_cb(o, grp_clicked, LV_EVENT_CLICKED, arg_of(g, i));
    lv_obj_t *l = label(o, 0, 0, font, g->fg_off, text);
    lv_obj_center(l);
    g->bg[i] = o;
    g->fg[i][0] = l;
    return o;
}

static void grp_init(radio_grp_t *g, int fg_n, lv_color_t bg_on, lv_color_t bg_off,
                     lv_color_t fg_on, lv_color_t fg_off, const char *name)
{
    memset(g, 0, sizeof(*g));
    g->fg_n = fg_n;
    g->bg_on = bg_on;
    g->bg_off = bg_off;
    g->fg_on = fg_on;
    g->fg_off = fg_off;
    g->name = name;
}

/* ★ 10-03 第九次：首页分类（s_grp_cat）和发现页地区（s_grp_prov）
 *   这两个单选组拆掉了 —— 栏目/省份条改成了「横向可滑的胶囊 + 点一下
 *   进全屏页」，不再需要 grp_apply() 那种「选中态跟着切」的单选语义。
 *   夜间页的 s_grp_sleep 还在用。*/
static radio_grp_t s_grp_sleep;  /* 夜间页定时 */

/* ============================================================
 *  虚拟双列网格 —— 几百个电台不可能一次全建成控件
 * ============================================================
 *  ★ 为什么必须「虚拟」：
 *    LVGL 的内置池只有 112 KB（sdkconfig CONFIG_LV_MEM_SIZE），
 *    而台单现在有 686 个台。就算一行只放一个 label（label + 文本约
 *    120~200 B），686 行也要 80~140 KB —— 直接把池抽干，
 *    release 下 lv_malloc 返回 NULL 而 LV_ASSERT_MALLOC 是空操作 ⇒ 往
 *    NULL 写 ⇒ 无限重启（10-02 踩过）。
 *
 *  ★ 做法（和手机列表一样）：
 *    只建「可视行数 × 2」个格子当作回收站（首页 3 行 6 个、
 *    全屏页 7 行 14 个），跑再多台内存也不变。
 *    再放一个高 = 总行数 × 行高 的 spacer 把内容撑起来，
 *    靠 LV_EVENT_SCROLL 读 scroll_y 算出「现在是第几行」，
 *    把那 10 个格子重新填成可见的那几行。内存恒定，与台数无关。
 *
 *  ★ 为什么不能靠 LVGL 自己的布局：
 *    LVGL 不虚拟化 —— 不会自动回收屏幕外的子控件。
 */
/* ★★★ 10-05 v2.0：三列（兰兰拍板「电台页双列比较好，能三列最好，毕竟电台太多了」）
 *   依据 _design/v2-07-列表页.png：三列 × 8 行 = 一屏 24 台。
 *   几何：屏 480 宽 − 列表页左右边距(6×2) = 468，格间距 6 ⇒
 *         cell_w = (468 − 6×2) / 3 = 152 px。
 *   ★ 连带影响：列宽从 230 掉到 152 ⇒ 台名截断必须从 13 字降到 8 字
 *     （12px 中文字宽约 12 px，152 宽减去内边距只放得下 8 个汉字 + 留白）。
 *     改 utf8_copy_n 的第二个参数时【必须同步改这一行的 cell_w 计算】，
 *     否则就是台名撞出格子边框。真机照片一眼能看出来。*/
#define GRID_COLS  3
#define GRID_ROWS  8      /* 上限：全屏页 7 行 + 一点余量 */
#define GRID_VIS   (GRID_COLS * GRID_ROWS)
#define GRID_MAX   512          /* 一个筛选结果最多记 512 个台下标 */

typedef void (*grid_fill_fn)(int station_idx);   /* 把格子填成这一台 */

typedef struct {
    lv_obj_t *cont;                 /* 可滚动容器 */
    lv_obj_t *cell[GRID_VIS];       /* 复用的格子 */
    lv_obj_t *nm[GRID_VIS];         /* 格子里的台名 label */
    lv_obj_t *spacer;               /* 撑内容高度用的透明对象 */
    int       idx[GRID_VIS];        /* 每个格子当前显示的台下标，-1 = 空 */
    int       pool[GRID_MAX];       /* 当前筛选结果的下标池 */
    int       total;                /* 池里几个台 */
    int       row_h;                /* 一行高（含缝） */
    int       cell_h;               /* 格子自身高 */
    int       cell_w;               /* 格子宽 */
    int       first;                /* 当前窗口第一个是第几行 */
    int       vis_rows;             /* 可视几行（运行期定：首页 3 / 发现 5 / 全屏 7）*/
    int       nslot;                /* 实际建了几个格子 = vis_rows * GRID_COLS */
    lv_color_t bg, bg_sel;
    lv_color_t fg;
    const lv_font_t *font;
    void (*on_play)(int station_idx);    /* 点格子 */
    void (*on_fav)(int station_idx);     /* 长按格子 = 收藏/取消 */
    int       from_page;                 /* 这层网格属于第几页：
                                           * 播放页的返回键就回这一页。
                                           * 兰兰 10-03：「要做到从哪里来能够回到哪里」*/
    int       mode;                      /* 0 = 显示台名；1 = 显示【海外地区块】
                                           *（pool 里存的也变成地区下标）*/
    void (*on_region)(int slot, int idx);    /* mode==1 时点格子的回调 */
} st_grid_t;

/* ============================================================
 *  横向虚拟胶囊条 —— 栏目（11 个）/ 省份（31 个）共用
 * ============================================================
 *  ★ 为什么也是虚拟的：31 个省份每个一颗胶囊（胶囊 + 文字 = 2 个
 *    lv_obj，各约 250~400 B）就是 60~120 B×62 ≈ 25 KB。
 *    LVGL 内置池只有 98 KB，而同屏还有三套网格 —— 直接把池抽干，
 *    渲染线程申请不到缓冲就崩在 lv_draw_sw_mask（10-03 踩过）。
 *
 *  ★ 做法：横向也只建 STRIP_N 颗胶囊，靠 LV_EVENT_SCROLL 读 scroll_x
 *    算出「现在是第几颗」，把那几颗循环填上内容。内存与条目数无关。
 *    跟手机上的横滑 tab bar 是一个道理。
 */
#define STRIP_N 7          /* 同屏可见几颗（480 宽，一颗 64+5 缝 ⇒ 7 颗刚好）*/

typedef struct {
    lv_obj_t *cont;
    lv_obj_t *chip[STRIP_N];
    lv_obj_t *txt[STRIP_N];
    lv_obj_t *spacer;
    int       n;           /* 总共几项 */
    int       first;        /* 当前窗口第一颗是第几项 */
    int       cw;           /* 一颗宽 */
    int       vis;          /* ★ 第二十次：实际同屏可见几颗（按宽度算，不是写死 7）*/
    lv_obj_t *btn_prev;      /* 第十九次：左翻页按钮（放在 cont 外面） */
    lv_obj_t *btn_next;      /* 第十九次：右翻页按钮 */
    void (*on_pick)(int idx);
} hstrip_t;

static hstrip_t s_strip_cat;    /* 首页栏目条 */
static hstrip_t s_strip_prov;   /* 发现页省份条 */

/* 栏目胶囊的短名 —— 13 个栏目名有 4 个字（「新闻综合」「怀旧老歌」），
 * 胶囊只有 57 px 宽放不下，用短名；进全屏页后显示全名。
 *
 * ★★★ 2026-10-04 第二十一次：补齐到 NET_CAT_N + 1 = 14 项。
 *   之前这份数组只写了 11 项，但声明是 [NET_CAT_N + 1]（14）⇒
 *   下标 11/12/13 是【未初始化的野指针】，hstrip_show() 直接
 *   lv_label_set_text() 拿它当 C 字符串 ⇒ 读到相邻内存里的东西。
 *   兰兰看到的「栏目最右边有两个全部按键」就是这里来的 ——
 *   第 11 项写的是「全部」，第 12、13 项读到的是「全部」附近的残值。
 *   ⇒ 教训：定长数组的「声明长度」与「初始化项数」必须一致，
 *      这类错编译器不报、运行起来也只是「显示怪怪的东西」，最难查。
 *
 * ★★ 2026-10-04 第二十一次：栏目从 13 档缩到 12 档（删掉空着的「综合」），
 *   这里同步删掉它的短名「综合」，剩 12 + 「全部」= 13 项。
 *
 * ★★★ 2026-10-04 第二十八次：前面插 3 个【自定义入口】（兰兰定）：
 *   收藏 / 历史 / 音乐库，然后才是 12 个栏目 + 全部。
 *   下标约定（下标是 hstrip 与 ev_pick_cat 之间唯一的契约，别乱改）：
 *       0      收藏    → 进全屏收藏页
 *       1      历史    → 进全屏历史页
 *       2      音乐库  → 进 SD 本地音频页（第 5 页）
 *       3..14  12 个栏目
 *       15     全部
 *   ⚠️ 这三项【不是栏目】（cat index 越界），ev_pick_cat 里必须先分流，
 *     否则 s_list_cat=0 会被当成「新闻」列出一堆无关的台。
 *
 * ★★ 标签名「音乐库」在标签条上写不下：
 *   hstrip 胶囊宽只有 51 px（= 340/6 - 5），「音乐库」三个 14px 汉
 *   要 42 + 内边距 8 = 50 px ⇒ 只余 1 px，一个字形宽度都可能挤掉。
 *   ⇒ 标签条上用两字「卡内」；首页那张大卡片才写全名「音乐库」
 *     （卡片宽 152，放得下）。同一页两个地方叫不同名字会有点跳，
 *     但 51 px 是物理上限，硬塞会截字。*/
#define HCAT_FAV  0
#define HCAT_HIST 1
#define HCAT_SD   2
#define HCAT_REAL 3          /* 第一个真栏目的下标 */
#define HCAT_ALL  (HCAT_REAL + NET_CAT_N)   /* = 15 */

static const char *const k_cat_short[HCAT_ALL + 1] = {
    /* 3 个自定义入口（标签条上用短名，理由见上）*/
    "Fav", "Hist", "SD",
    /* 12 个栏目（顺序与 g_cat_name 一致） */
    "News", "Traf", "Mus", "Arts", "Tell", "Oper", "Oldi",
    "Net", "Edu", "TV", "Reli", "Intl",
    /* 全部 */
    "All",
};

/* ★★★ 英文版专用：显示用名称表（协议表 g_*_name 保持中文，
 *   它是 TSV 匹配键，改了就静默失效）。下标与协议表严格一致。*/
const char *const xs_cat_name_en[NET_CAT_N] = {
    "News", "Traffic", "Music", "Arts", "Story", "Opera", "Oldies", "Online", "Edu", "TV Audio", "General", "Relig", "Intl News",
};

/* ★★★ 英文版专用：显示用名称表（协议表 g_*_name 保持中文，
 *   它是 TSV 匹配键，改了就静默失效）。下标与协议表严格一致。*/
const char *const xs_prov_short_en[NET_PROV_N] = {
    "BJ", "TJ", "HEB", "SX", "NM", "LN", "JL", "HLJ", "SH", "JS", "ZJ", "AH", "FJ", "JX", "SD", "HA", "HB", "HN", "GD", "GX", "HI", "CQ", "SC", "GZ", "YN", "XZ", "SN", "GS", "QH", "NX", "XJ", "TW", "HK", "MO", "ZH", "NAm", "EU", "JP/KR", "SG/MY", "SEA", "OC", "ZH-OS",
};

/* ★★★ 英文版专用：显示用名称表（协议表 g_*_name 保持中文，
 *   它是 TSV 匹配键，改了就静默失效）。下标与协议表严格一致。*/
const char *const xs_prov_long_en[NET_PROV_N] = {
    "Beijing", "Tianjin", "Hebei", "Shanxi", "Inner Mongolia", "Liaoning", "Jilin", "Heilongjiang", "Shanghai", "Jiangsu", "Zhejiang", "Anhui", "Fujian", "Jiangxi", "Shandong", "Henan", "Hubei", "Hunan", "Guangdong", "Guangxi", "Hainan", "Chongqing", "Sichuan", "Guizhou", "Yunnan", "Tibet", "Shaanxi", "Gansu", "Qinghai", "Ningxia", "Xinjiang", "Taiwan, China", "Hong Kong, China", "Macao, China", "Other Chinese", "North America", "Europe", "Japan / Korea", "Singapore / Malaysia", "Southeast Asia", "Oceania", "Chinese Overseas",
};

static void hstrip_show(hstrip_t *h, int slot, int idx)
{
    if (idx < 0 || idx >= h->n || slot >= h->vis) {
        lv_obj_set_style_bg_opa(h->chip[slot], LV_OPA_TRANSP, 0);
        lv_label_set_text(h->txt[slot], "");
        return;
    }
    lv_obj_set_style_bg_opa(h->chip[slot], LV_OPA_COVER, 0);
    const char *s = (h == &s_strip_cat) ? k_cat_short[idx]
                                        : xs_prov_short_en[idx % NET_PROV_N];
    lv_label_set_text(h->txt[slot], s);
    /* ★ 海外地区（第 31 项起）用琥珀色胶囊，一眼能认出「这边是海外」。
     *   兰兰 10-03 问「海外台没有做进去？」—— 做了（194 台），
     *   但它们排在 31 个国内省之后，滑到底才看得到，看着像没做。
     * ⚠️ 循环复用：胶囊是 7 颗轮流填内容的，所以每次都要【两个分支都显式写】，
     *   不能只在海外分支设色 —— 否则从海外滑回国内那一格还带着琥珀底。*/
    bool ovs = (h == &s_strip_prov) && (idx >= NET_PROV_CN_N);
    if (ovs) {
        lv_obj_set_style_bg_color(h->chip[slot], lv_color_hex(0x3A2E12), 0);
        lv_obj_set_style_text_color(h->txt[slot], C_AMBER, 0);
    } else if (h == &s_strip_prov) {
        lv_obj_set_style_bg_color(h->chip[slot], C_PANEL, 0);
        lv_obj_set_style_text_color(h->txt[slot], C_TEXT, 0);
    }
}

/* 槽位 → 它当前代表第几项（用 idx 数组存，模块少一点）*/
static int hstrip_idx_of(hstrip_t *h, lv_obj_t *chip)
{
    for (int i = 0; i < STRIP_N; i++) {
        if (h->chip[i] == chip) return i;
    }
    return -1;
}

static void hstrip_fill(hstrip_t *h)
{
    for (int i = 0; i < STRIP_N; i++) {
        int idx = h->first + i;
        /* ★ 环绕：末尾后面接回开头，这样往右滑到底能直接看到第一项，
         *   不用「松手弹回去」那一下 —— 兰兰说的「向右边滑动选择」。*/
        if (h->n > 0) idx = (h->first + i) % h->n;
        hstrip_show(h, i, idx);
    }
}

static void hstrip_pick(lv_event_t *e)
{
    hstrip_t *h = (hstrip_t *)lv_event_get_user_data(e);
    if (!h) return;
    int slot = hstrip_idx_of(h, lv_event_get_target(e));
    if (slot < 0 || slot >= h->vis) return;
    int idx = (h->n > 0) ? (h->first + slot) % h->n : -1;
    if (idx < 0) return;
    if (h->on_pick) h->on_pick(idx);
}

/* ============================================================
 *  ★★★ 第二十次：左「←」右「→」翻页按钮（重写，位置是本轮最大 bug）
 * ------------------------------------------------------------
 *  兰兰 10-04 原话：「主页中间有个向右的箭头按键挡住了滑动条栏目中间的按键，
 *                    栏目左右滑动无效，所以无法滑动到伴音这个项目进行试验」
 *
 *  —— 这两条是**同一个 bug 的两面**，而且是我 10-04 上一版亲手造的：
 *
 *  ❶ 【箭头压在胶囊上】上一版把按钮建在 cont 【外面】的同一个坐标带
 *      （x 与 x+w-30，各 30 宽），而胶囊也在这个带里、还更宽。
 *      ⇒ 兰兰看到的「中间那个向右箭头」就是 btn_next，
 *      它正好压住第 5 颗胶囊 ⇒ 点不到。而 btn_prev 压住第 1 颗。
 *
 *      屏幕上只有 7 颗可见（STRIP_N），去掉被压的两颗 → 可点的只剩 5 颗，
 *      「伴音」在第 10 项，滑不到。
 *
 *  ❷ 【两页共用一个 s_page_h】上一版用一个文件级 `s_page_h` 存「当前是哪条」，
 *      而 hstrip_create 被调用了**两次**（首页栏目条 + 发现页地区条），
 *      后一次把前一次覆盖 ⇒ **首页的箭头翻的是发现页的地区条**。
 *      所以首页按箭头，画面上毫无反应。
 *      ⇒ 这就是「栏目左右滑动无效」的第一层原因，跟手势一点关系都没有。
 *
 *  ⇒ 本轮三处一起改，缺一不可：
 *     ① 方向改用 user_data 携带（把 step 塞进一个小结构），不再靠全局变量
 *     ② 按钮【移出胶囊带】：胶囊整体右移 34 px，让出左侧一条 34 宽的翻页区
 *     ③ 翻页后强制把 cont 的 scroll 归零（虚拟条不用真滚动，见下）
 */
typedef struct { hstrip_t *h; int step; } hpage_t;
static void hstrip_sync_scroll(hstrip_t *h);   /* 前向声明：下面两个回调会调它 */

static void hstrip_page(lv_event_t *e)
{
    hpage_t *pp = (hpage_t *)lv_event_get_user_data(e);
    if (!pp || !pp->h || pp->h->n <= 0) return;
    hstrip_t *h = pp->h;
    h->first = (h->first + pp->step + h->n * 2) % h->n;
    hstrip_fill(h);
    hstrip_sync_scroll(h);
}

/* 按钮翻完页后把容器 scroll 拨到 0。
 * ★ 为什么强制归零、而不是拨到 h->first*step：
 *   这条是【虚拟条】—— 屏上永远只有 STRIP_N 颗胶囊，靠 h->first 换内容。
 *   cont 本身【不需要真的滚出去】；让它保持 scroll_x = 0，
 *   才能保证「胶囊 x=0 起排」这条布局假设永远成立。
 *   上一版拨到 h->first*step，等于自己把胶囊推出可视区，
 *   于是「翻页后看起来更空了」—— 兰兰说的「箭头在中间」也有这个成因。
 *   滚动条想表达进度，改用首项高亮（选中态）来表达，见 hstrip_fill。*/
static void hstrip_sync_scroll(hstrip_t *h)
{
    if (h->cont) lv_obj_scroll_to_x(h->cont, 0, LV_ANIM_OFF);
}

static void hstrip_scroll_cb(lv_event_t *e)
{
    hstrip_t *h = (hstrip_t *)lv_event_get_user_data(e);
    if (!h || h->n <= 0) return;
    int step = h->cw + 5;
    /* ★ 第二十次：滚动中只做一件事 —— 累计位移超过半颗就翻一页，
     *   然后【立刻把 scroll 拨回 0】。
     *   上一版是 f = scroll_x / step 直接当 first，但 scroll_x 是弹性动画的量，
     *   与「一颗 = step」并不严格对应（一屏放不下 7 颗时更不成立）
     *   ⇒ first 算出来经常是 0，看起来就是滑不动。
     *   ⚠️ 防抖：拨回 0 会再触发一次 SCROLL，靠 sx==0 直接 return 挡住。*/
    int sx = lv_obj_get_scroll_x(h->cont);
    if (sx > -step && sx < step) return;      /* 不足半颗，不动 */
    int dir = (sx > 0) ? +1 : -1;              /* 右滑（内容右移）= 看后面 */
    h->first = (h->first + dir + h->n) % h->n;
    hstrip_fill(h);
    lv_obj_scroll_to_x(h->cont, 0, LV_ANIM_OFF);
}

/* ============================================================
 *  hstrip_create —— 建一条【虚拟胶囊条】
 * ------------------------------------------------------------
 *  ★★ 第二十次重写（10-04）：从「真滚动容器」改成「定点翻页」
 *
 *  为什么彻底不靠手势了（兰兰三轮反馈：地区 8 个 / 栏目滑不动 / 箭头挡胶囊）：
 *   这条屏是 480×320 + FT6336，横滑在这套组合上**时灵时不灵**，
 *   而我又拿不到「到底为什么」的日志（LV_ASSERT 在 release 下是空的）。
 *   ⇒ 改成**只靠确定性按钮**：左「←」右「→」，点一下内容整体移一格。
 *      手势仍保留（想滑就滑），但不再是指望它。
 *
 *  ★★ 版面账（10-04 真机日志逼出来的，原设计是错的）：
 *   上一版把两个箭头放在 cont 外面但【坐标落在胶囊带里】，
 *   正好压住第 5 颗 ⇒ 兰兰看到的「主页中间那个向右箭头」。
 *   本轮箭头【独占左侧 HSTRIP_ARROW_W】，胶囊带整体右移。
 *   ⚠️ 但真机日志立刻报出副作用：
 *       hstrip: n=14 cw=80 vis=4   ← 栏目条只剩 4 颗（原来 7）
 *       hstrip: n=42 cw=64 vis=5   ← 地区条只剩 5 颗
 *   原因：cw 是写死的 80/64，扣掉箭头区就放不下 7 颗了。
 *   ⇒ 修法：**胶囊宽度按可用宽反算**，而不是写死。
 *      cw = (可用宽 / 目标颗数) - 缝，目标 6 颗 → cw 57。
 *      「新闻综合」四个字 14px 约 56 px，57 刚好放得下（LV_LABEL_LONG_DOT
 *      兜底），比原来 80 少 23 px，换来多两颗。
 */
/* ★★ 2026-10-04 第二十一次「箭头挡住一个栏目/地区」的修法与账：
 *
 *   兰兰报「首页和发现页的箭头都挡住了一个按键，很容易误触到栏目」。
 *   真因是【声明的宽度 ≠ 实际画出来的总宽】：
 *     上一版 HSTRIP_ARROW_W 写 34，但实际画了两个 30 宽的按钮（x 和 x+32）
 *     ⇒ 实际占 62 px；而胶囊带只从 x+36 开始
 *     ⇒ 右边那个箭头有 26 px 压在胶囊带上。
 *     开机日志 `vis=6` 看着完全正常，所以拖了几轮才发现。
 *
 *   本轮的取舍（480 宽放不下所有想要的东西，只能挑）：
 *     箭头 26×26（放得下 14px 箭头字，好按）⇒ 预留 26×2+2+2 = 56 px
 *     胶囊带 cont = 408－56 = 352；step = 352/6 = 58；cw = 58－5 = **53**
 *     同屏 **6 颗**（6×58 = 348 ≤ 352，余 4 px）⇒ 不因修箭头而少一颗。
 *     cw_min 由 56 降到 **48**：栏目短名最长 2 字（「新闻」「境外」），
 *     14px 两字约 28 px，48 绰绰有余；「新闻综合」这种全名只在全屏页出现。
 *   ⇒ 另加运行期断言日志（arrows occupy / reserved / OK），下次再撞直接看日志。
 *
 * ★★ 2026-10-04 第二十二次：箭头改【绿色 + 加大到 32 px + 18 px 箭头字】
 *
 *   兰兰：「首页栏目箭头按键太小，不容易发现，可以设置绿色箭头显示直观点」
 *
 *   为什么原来「小」不只是尺寸问题：11 px 的 ← → 是【次要灰白】，
 *   视觉权重跟胶囊上的文字一模一样，一眼扫过去就滑过去了 ——
 *   尺寸只是表象，【颜色不显眼】才是「不容易发现」的根因。
 *   ⇒ 三个维度一起改：
 *     ① 尺寸 26 → 32（更好按）
 *     ② 字  11 px(F_TINY) → 18 px(F_H2)
 *     ③ 色  C_TEXT(白) → C_GREEN(拾声绿)，与所有可交互元素同色系
 *
 *   ★ 横向账（480 宽，CONT 424，写死坐标前先算，别叠）：
 *     箭头区 = 32×2 + 2（缝）+ 2（余） = **68**
 *     胶囊带 cont = 408 － 68 = 340
 *     step = 340 / 6 = 56　　cw = 56 － 5 = **51**
 *     同屏 6 颗：6×56 = 336 ≤ 340（余 4 px）✓ 不因加大箭头而少一颗
 *   ⚠️ 箭头字必须用 F_H2(18px)：32 px 的方块配 11 px 字会显得头重脚轻。
 *   ⚠️ ◀▶ 实测 SimHei/Deng 都没有这两个字形（会变方块），
 *     所以只能用 ← →，而 F_H2 走主字库，这两个字肯定有。*/
#define HSTRIP_ARROW_W  32      /* 单个箭头边长（32×32）*/
#define HSTRIP_ARROW_GAP 2      /* 两箭头之间的缝 */
#define HSTRIP_BAND_X   (HSTRIP_ARROW_W * 2 + HSTRIP_ARROW_GAP + 2)  /* = 68 */
#define HSTRIP_TARGET_N  6      /* 目标同屏颗数 */

static void hstrip_create(hstrip_t *h, lv_obj_t *parent, int x, int y,
                          int w, int hgt, int cw_min, int n, void (*on_pick)(int))
{
    memset(h, 0, sizeof(*h));
    h->n = n;
    h->on_pick = on_pick;

    /* 胶囊带：从箭头右侧开始，宽度扣掉箭头区 */
    int cx = x + HSTRIP_BAND_X;
    int cw_total = w - HSTRIP_BAND_X;

    /* ★ 胶囊宽度按可用宽反算：先按目标颗数算 step，再留 5 px 缝给每颗 */
    int step  = cw_total / HSTRIP_TARGET_N;
    int cw    = step - 5;
    if (cw < cw_min) {                 /* 太窄就退而求其次，按最小宽度排 */
        cw    = cw_min;
        step  = cw + 5;
    }
    h->cw = cw;

    h->cont = lv_obj_create(parent);
    lv_obj_remove_style_all(h->cont);
    lv_obj_set_pos(h->cont, cx, y);
    lv_obj_set_size(h->cont, cw_total, hgt);
    lv_obj_set_style_bg_opa(h->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(h->cont, 0, 0);
    /* ★ 保留 SCROLLABLE 让手势仍可用，但 scroll_x 由 sync_scroll 钉在 0 */
    lv_obj_add_flag(h->cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(h->cont, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(h->cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(h->cont, hstrip_scroll_cb, LV_EVENT_SCROLL, h);

    /* 实际能放几颗：算出来，不写死（少一颗也不能画出 cont 外）*/
    int fit = (cw_total + 5) / step;
    if (fit > STRIP_N)     fit = STRIP_N;
    if (fit < 1)           fit = 1;
    h->vis = fit;

    for (int i = 0; i < STRIP_N; i++) {
        lv_obj_t *c = box(h->cont, i * step, 2, cw, hgt - 4, C_PANEL, (hgt - 4) / 2);
        lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
        /*★★★ 10-04 第二十六次：这两行 clear_flag 就是「触摸滑不动」的根因，
         *   兰兰报「首页栏目触摸滑动，发现页触摸滑动」——两条全都不响应，
         *   而 ← → 箭头一直好用（他没抱怨箭头）。
         *
         * 【为什么箭头能用、滑动不能】这个组合曾是最大的谜。查 LVGL 源码才定案：
         *   lv_indev_scroll.c:289~340  lv_indev_find_scroll_obj()
         *       obj_act = 按到的那个对象（胶囊 c，不是 cont）
         *       while(obj_act) {
         *           if(!lv_obj_is_scrollable(obj_act)) {
         *               if(!lv_obj_is_scroll_chain_hor(obj_act) && hor_en) break; ← ★
         *               obj_act = lv_obj_get_parent(obj_act); continue;
         *           }
         *           ...
         *       }
         *   ★ 第 335 行：胶囊不可滚动时，若 scroll_chain_hor 为 false 就【break】，
         *     【不再往上找父容器】⇒ cont 永远拿不到 scroll_obj ⇒ 手势 100% 失效。
         *   而箭头是 box() 建的、没清 chain_hor、也不依赖滚动 ⇒ 一直好用。
         *
         * 【初值】lv_obj.c:1505  obj->scroll_chain_hor = 1;  ← LVGL 默认【开】
         * 【我做错的】lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
         *            把默认的 1 清成了 0。
         *
         * ★ 我在 container 层（LV_DIR / spacer / scroll 回调）来回查了三轮都没查到，
         *   因为根因在【子对象的一个标志位】上 —— 层级选错了。
         *   速查卡铁律 21 记的「LV_DIR_NONE 不抢滚动」方向是对的，
         *   但那只覆盖了容器自己的 scroll_dir，漏了「子对象能否把滚动交回父容器」这一层。
         *
         * ⚠️ 铁律补充：**清 LVGL 的「默认开启」标志之前，先去源码确认它的默认值。
         *   默认 1 的东西被你 clear 成 0，症状是「某个手势整类失效」，
         *   而日志里什么异常都看不到。*/
        lv_obj_set_style_shadow_width(c, 0, 0);
        if (i < h->vis) {
            lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(c, hstrip_pick, LV_EVENT_CLICKED, h);
        } else {
            /* 放不下的胶囊彻底不可点、不占视觉 —— 不靠透明躲，
             * 因为透明对象照样吃触摸（10-04 又踩过一次这个坑）。*/
            lv_obj_clear_flag(c, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
        }
        h->chip[i] = c;
        h->txt[i]  = label(c, 0, 0, F_BODY, C_TEXT, "");
        lv_obj_set_width(h->txt[i], cw - 6);
        lv_label_set_long_mode(h->txt[i], LV_LABEL_LONG_DOT);
        lv_obj_center(h->txt[i]);
    }
    /* spacer 只作内容宽度的量尺：宽 = 全部 n 项，x 从 0 起，透明且不可点。*/
    h->spacer = box(h->cont, 0, 0, n * step, hgt - 4, C_BG, 0);
    lv_obj_set_style_bg_opa(h->spacer, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(h->spacer, LV_OBJ_FLAG_CLICKABLE);

    hstrip_fill(h);

    /* ---- 左「←」右「→」：**独占左侧一条**，胶囊带整体右移，绝不重叠 ----
     *  ⚠️ 箭头用 ← →（SimHei/Deng 有字形）；◀▶ 实测没有，会变方块。
     *
     *  ★★ 2026-10-04 第二十一次修「箭头挡住一个栏目/地区」：
     *   兰兰报「首页和发现页的箭头都挡住了一个按键，很容易误触」。
     *   真因是【算账漏了】：
     *     上一版把 HSTRIP_ARROW_W 写成 34，但实际画了两个 30 宽的按钮
     *     （x 和 x+32）⇒ 实际占用 62 px；而胶囊带只从 x+36 开始
     *     ⇒ 右边那个箭头有 26 px 压在胶囊带【上面】。
     *     日志里 vis=6 看着正常，所以一直没发现。
     *   ⇒ 修法：HSTRIP_BAND_X = 箭头真实总宽 + 缝 + 余量 = 26×2+2+4 = 58，
     *     胶囊带从 x+58 起，两个箭头严格落在 x..x+56 内，永不重叠。*/
    static hpage_t s_pp[4];      /* 每条两个按钮各一份上下文 */
    static int     s_pp_n = 0;
    if (s_pp_n + 2 > 4) s_pp_n = 0;
    hpage_t *p_prev = &s_pp[s_pp_n++];
    hpage_t *p_next = &s_pp[s_pp_n++];
    p_prev->h = h; p_prev->step = -1;
    p_next->h = h; p_next->step = +1;

    int by = y + (hgt - HSTRIP_ARROW_W) / 2;
    h->btn_prev = box(parent, x, by, HSTRIP_ARROW_W, HSTRIP_ARROW_W, C_HILITE, 6);
    h->btn_next = box(parent, x + HSTRIP_ARROW_W + HSTRIP_ARROW_GAP, by,
                      HSTRIP_ARROW_W, HSTRIP_ARROW_W, C_HILITE, 6);
    /* ★ 第二十二次：箭头字 11 px → 18 px、颜色 白 → 绿（兰兰：「箭头太小不容易发现，
     *   可以设置绿色箭头显示直观点」）。字号与边长一起算：18 px 的箭头字放进
     *   32 px 的方块里居中，视觉上才「像按钮」而不是「像一个字符」。*/
    lv_obj_t *t1 = label(h->btn_prev, 0, 0, F_H2, C_GREEN, "←");
    lv_obj_t *t2 = label(h->btn_next, 0, 0, F_H2, C_GREEN, "→");
    lv_obj_center(t1);
    lv_obj_center(t2);
    /* 绿字在深灰底（C_HILITE）上对比度够，但为了「一眼看见」，
     * 再给方块描一圈绿边 —— 边界清楚了才知道那是个可按的东西。*/
    lv_obj_set_style_border_width(h->btn_prev, 1, 0);
    lv_obj_set_style_border_color(h->btn_prev, C_GREEN, 0);
    lv_obj_set_style_border_width(h->btn_next, 1, 0);
    lv_obj_set_style_border_color(h->btn_next, C_GREEN, 0);
    lv_obj_add_flag(h->btn_prev, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(h->btn_next, LV_OBJ_FLAG_CLICKABLE);
    /* ★ 箭头不挂 ev_press/ev_release（按下改 opa 会触发重排，
     *   而重排会让 LVGL 把这次按下判成滚动 —— 20-04 跳转键「要碰几次」的同款坑）*/
    lv_obj_set_scroll_dir(h->btn_prev, LV_DIR_NONE);
    lv_obj_set_scroll_dir(h->btn_next, LV_DIR_NONE);
    lv_obj_add_event_cb(h->btn_prev, hstrip_page, LV_EVENT_CLICKED, p_prev);
    lv_obj_add_event_cb(h->btn_next, hstrip_page, LV_EVENT_CLICKED, p_next);
    /* ★ 记账断言：箭头实际占的宽度必须 ≤ 预留区，否则一定压胶囊。
     *   这条是本轮 bug 的教训 —— 别再手算版面了，让机器算。*/
    int arrows_real = HSTRIP_ARROW_W * 2 + HSTRIP_ARROW_GAP;
    ESP_LOGW(TAG, "hstrip: arrows occupy %d px, reserved %d px (margin %d) %s",
             arrows_real, HSTRIP_BAND_X, HSTRIP_BAND_X - arrows_real,
             (arrows_real <= HSTRIP_BAND_X) ? "OK" : "!! OVERLAP !!");
    ESP_LOGI(TAG, "hstrip: n=%d cw=%d(min %d) vis=%d step=%d cont=%d wide",
             n, cw, cw_min, h->vis, step, cw_total);
}

/* ============================================================
 *  切页
 * ============================================================ */
/* 导航栏文字：选中亮绿 / 未选暗绿（第二十二次，见 nav_build 的注释）*/
#define C_NAV_DIM  lv_color_hex(0x1F7A5C)

static const uint32_t s_page_bg[PAGES_N] = {
    0x0B0E12, 0x0B0E12, 0x0B0E12, 0x0A0906, 0x0B0E12, 0x0B0E12, 0x0B0E12,
    0x0B0E12
};

/* ★ 第 8 页 WiFi 设置的建/销（定义在文件后段 3500 多行）。
 *   goto_page 在下面就要调 wifi_page_destroy，所以声明必须放在这里 ——
 *   放在 1700 多行的声明段里就晚了，gcc 会报 implicit declaration。*/
static void wifi_page_destroy(void);
static void night_page_destroy(void);   /* v1.35：定义在 page_night_build 之后 */
static void night_page_ensure(void);
static void grid_pages_destroy(int page_idx);   /* v1.40：三套网格按需建/离开删 */
static void grid_pages_ensure(int page_idx);
/* ★ v1.40：下面这三个是 grid_pages_ensure 的实现要用到的，
 *   但它们都定义在 goto_page 之后（1719/2762 行往后）。
 *   本工程 -Werror ⇒ 缺声明会直接编译失败，注释得写清楚为什么。*/
static st_grid_t *grid_create_ex(lv_obj_t *parent, int x, int y, int w, int h, int vis_rows);
static void on_grid_play(int idx);
static void on_grid_fav(int idx);
static void home_grid_refresh(void);
static void find_grid_refresh(void);
static void list_page_refresh(void);   /* v1.40：case 6 建完网格要刷内容 */
static void clock_cb(lv_timer_t *t);       /* 同上：定义在文件后段 */
static void clock_date_cb(lv_timer_t *t);
static void sleep_left_cb(lv_timer_t *t);
static void night_state_cb(lv_timer_t *t);

static void goto_page(int idx)
{
    if (idx < 0 || idx >= PAGES_N || idx == s_cur_page) return;
    /* ★ 离开状态页就把关机键的「再点一次」清掉（10-04）：
     *   否则用户逛一圈回来，按钮还写着「再点一次关机」，
     *   一下就真关了 —— 二次确认反而变成更危险的坑。*/
    if (s_cur_page == 4 && idx != 4 && s_sdconfirm_timer) {
        lv_timer_del(s_sdconfirm_timer);
        s_sdconfirm_timer = NULL;
        if (s_lbl_power) {
            lv_label_set_text(s_lbl_power, "Power off");
            lv_obj_set_style_text_color(s_lbl_power, lv_color_hex(0xF87171), 0);
        }
    }
    /* ★★ 离开第 8 页（WiFi 设置）就把整页对象删掉，把 LVGL 池还回去。
     *   第二十二次真机崩在这：常驻时建页开销 17 KB，池 free 从 26.9 KB
     *   掉到 9.5 KB（used 91%），splash 一撤、main 屏开始渲染就崩在
     *   lv_draw_add_task —— 那里 lv_malloc 返 NULL，而 release 构建下
     *   LV_ASSERT_MALLOC 是空操作，于是往 NULL 里写，EXCVADDR=0x08。
     *   ⚠️ 钩子必须挂在 goto_page 里而不是返回键上：万一用户是从左边
     *   导航栏切走的，只挂返回键就会漏掉，池会一直缺这一块。*/
    if (s_cur_page == 7) wifi_page_destroy();
    /* ★★ v1.35：夜间页（第 4 页）同样「离开就删」。
     *   兰兰真机报「点击睡眠页就重启」＝ 这一页一显示就把池抽干，
     *   崩在 lv_draw_add_task（lv_malloc 返 NULL，release 下断言是空操作）。
     *   它比第 8 页更重，所以这轮【必须】做，不能拖。*/
    if (s_cur_page == 3) night_page_destroy();
    /* ★★★ v1.40：三套电台网格也改成「进页才建、离开就删」。
     *
     *   10-05 兰兰报「测试中经常重启」。抓日志抓到确切现场：
     *     Guru Meditation: StoreProhibited, EXCVADDR=0x00000008
     *     Backtrace: lv_draw_add_task ← lv_draw_label ← draw_main ← ...
     *     崩前最后两行正是「XSUI: page -> 2」「XSUI: tap: 导航」
     *   ★ 这与第 1502 行注释里记的【第二十二次】是【同一个病】，
     *     只是这次触发点从第 8 页换成了首页 —— 症状一字不差
     *     （lv_malloc 返 NULL，release 下 LV_ASSERT_MALLOC 是空操作，
     *      紧接着往 NULL 里写第一个字段 ⇒ 偏移 8）。
     *
     *   为什么必须动网格：开机实测池 free 只剩 14728 B（used 86%），
     *     而三套网格 9+12+24 = 45 格 × 2 对象 = 90 个 LVGL 对象，
     *     常驻约 21 KB ＝ 池的 21%。绘制任务（draw task）是在
     *     lv_display_refr_timer 里【临时】分配的，渲染完才释放，
     *     所以「对象建得下」不等于「重绘时分配得下」——
     *     切页那几毫秒里大批对象同时标脏 + 重绘范围大，就撞上了。
     *     ⇒ 表现为「偶尔」而不是「每次」，是概率性的。
     *
     *   修法与夜间页/WiFi 页同一套机制：离开即删、进页再建。
     *   顺序同样关键：先删旧页把池还回来，再建新页。
     */
    if (s_cur_page == 0 || s_cur_page == 1 || s_cur_page == 6)
        grid_pages_destroy(s_cur_page);
    /* 建页放在清完页之后、显示之前：确保 s_pages[idx] 刚建出来就渲染。
     * ⚠️ 顺序有讲究：先删旧页把池还回来，再建新页 —— 反过来会短时间
     *   同时持有两页，正是最容易被池卡死的时刻。*/
    if (idx == 3) night_page_ensure();
    if (idx == 0 || idx == 1 || idx == 6) grid_pages_ensure(idx);
    for (int i = 0; i < PAGES_N; i++) {
        lv_obj_set_style_bg_color(s_pages[i], lv_color_hex(s_page_bg[i]), 0);
        lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(s_pages[idx], LV_OBJ_FLAG_HIDDEN);
    for (int i = 0; i < NAV_N; i++) {
        bool on = (i == idx);
        lv_obj_set_style_bg_opa(s_nav_btns[i], on ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(s_nav_btns[i], C_HILITE, 0);
        lv_obj_set_style_opa(s_nav_marks[i], on ? LV_OPA_COVER : LV_OPA_40, 0);
        /* ★ 文字颜色也要跟着切。原来只在建的时候设一次，
         *   于是「首页」永远是亮绿、「状态」永远是灰的 —— 切页后不更新。*/
        if (s_nav_labels[i]) {
            lv_obj_set_style_text_color(s_nav_labels[i], on ? C_GREEN : C_NAV_DIM, 0);
        }
    }
    s_cur_page = idx;
    ESP_LOGI(TAG, "page -> %d", idx);
}

/* 由格子控件反查它是第几号槽位。
 * ★ 为什么不把槽位号塞进 user_data：LVGL 一个 user_data 只能带一个指针，
 *   而这里指针已经被 st_grid_t* 占了。直接拿事件目标比对指针最省事。*/
static int grid_slot_of(st_grid_t *g, lv_obj_t *cell)
{
    for (int i = 0; i < g->nslot; i++) {
        if (g->cell[i] == cell) return i;
    }
    return -1;
}

/* 一格显示的台；station_idx < 0 = 空 */
static void grid_show_cell(st_grid_t *g, int slot, int station_idx)
{
    g->idx[slot] = station_idx;
    lv_obj_t *c = g->cell[slot];
    if (station_idx < 0) {
        /* ★★★★★★ 10-05 v1.39 修【背后露出上一页电台名】
         *   兰兰原话：「点海外 10 个地区页应该后面不要出现其他电台名称，
         *     会有其他页面的电台露出，点击具体海外地区电台页，也会露出背后其他地区电台」
         *
         *   根因：清空格子时**只把底色设为透明，没清 label 的文字**。
         *     原来这里只有一句 `lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);`
         *     就 return 了 —— 底子透明了，可 nm[slot] 里上一屏的
         *     台名/地区名还留着，孤零零地浮在背景上。
         *   为什么「每页都露」而不是偶尔露：
         *     这一层是【虚拟网格】，nslot 固定 24 格（8 行×3 列），
         *     而 pool 只有 11~20 条 ⇒ grid_fill 里 pos >= total 的格子
         *     全部走 station_idx < 0 这条分支 ⇒ 每次切换都有十几格残留。
         *     海外地区页 11 格 → 进「欧洲」可能只有 8 个台 ⇒ 后面 16 格全露。
         *   ⇒ 修法：底色透明 **＋ 文字清空 ＋ 恢复默认文字色**
         *     （文字色也要重置：地区块模式写过 C_AMBER，
         *       不还原的话空格子留着琥珀色，下一屏正常台名会变琥珀）。
         *   ⚠️ 必须清 nm[slot]，不能用 lv_obj_get_child(c,0)：
         *     格子第一个子对象是 box 不是 label（见下方 mode==0 的注释）。*/
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
        if (g->nm[slot]) {
            lv_label_set_text(g->nm[slot], "");
            lv_obj_set_style_text_color(g->nm[slot], g->fg, 0);
        }
        return;
    }
    /* ★ 用建时存下的句柄，不要 lv_obj_get_child(c, 0) ——
     *   格子里的第一个子对象是「收藏小点」(box)，对它调 lv_label_set_text
     *   会按 label 的布局结构解释一个 box 的指针 ⇒ LoadProhibited 崩在
     *   lv_free_core。（10-03 踩过，回溯：grid_show_cell → set_text_internal）*/
    char buf[80];
    if (g->mode == 1) {
        /* ★ mode==1：这一格不是台，是【地区块】（海外二级分类用）。
         *   pool 里存的是地区下标，idx 也是。这里【不能调 app_fav_has()】——
         *   拿地区下标当台下标去问收藏，轻则恒 false，重则越界读。*/
        int pv = station_idx;
        if (pv >= 0 && pv < NET_PROV_N) {
            int cnt = 0, tot = app_radio_station_count();
            for (int i = 0; i < tot; i++) {
                if (app_st_get(i)->prov == pv) cnt++;
            }
            snprintf(buf, sizeof(buf), "%.12s · %d", xs_prov_long_en[pv], cnt);
        } else {
            snprintf(buf, sizeof(buf), "?");
        }
        if (g->nm[slot]) lv_label_set_text(g->nm[slot], buf);
        lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(c, g->bg, 0);
        /* 地区块用琥珀字，跟地区条里那些海外胶囊同一个色系 */
        if (g->nm[slot]) {
            lv_obj_set_style_text_color(g->nm[slot], C_AMBER, 0);
        }
        return;
    }
    /* ★ 10-05 三列：台名截断 13 → 8 字。理由见 GRID_COLS 上方注释：
     *   cell_w 从 230 掉到 152 px，12px 中文字宽约 12 px，只放得下 8 个。
     *   「中国之声经济广播」这类长台名会显示成「中国之声经济…」。*/
    utf8_copy_n(buf, sizeof(buf), app_st_get(station_idx)->name, 8);
    if (g->nm[slot]) lv_label_set_text(g->nm[slot], buf);
    /* 正在播的那台高亮 */
    bool cur = (station_idx == app_radio_station_current());
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(c, cur ? g->bg_sel : g->bg, 0);
    /* ★ 收藏标记用【台名颜色】表示，不另建一个小方块。
     *   绿 = 已收藏。30 个格子省下 30 个 lv_obj（每个约 250~400 B）——
     *   LVGL 池只有 98 KB，这一项就是 10 KB。*/
    if (g->nm[slot]) {
        lv_obj_set_style_text_color(g->nm[slot],
                                    app_fav_has(station_idx) ? C_GREEN : g->fg, 0);
    }
}

/* 按当前 first 重填所有格子 */
static void grid_fill(st_grid_t *g)
{
    for (int i = 0; i < g->nslot; i++) {
        int row = g->first + i / GRID_COLS;
        int col = i % GRID_COLS;
        int pos = row * GRID_COLS + col;
        int st  = (pos < g->total) ? g->pool[pos] : -1;
        lv_obj_set_pos(g->cell[i], col * (g->cell_w + 6), row * g->row_h);
        grid_show_cell(g, i, st);
    }
}

static void grid_slot_clicked(lv_event_t *e)
{
    st_grid_t *g = (st_grid_t *)lv_event_get_user_data(e);
    if (!g) return;
    int slot = grid_slot_of(g, lv_event_get_target(e));
    if (slot < 0) return;
    int st = g->idx[slot];
    if (st < 0) return;
    /* ★ 记住「是从哪一层点进来的」：播放页的返回键要回这一页，
     *   而不是固定回首页（兰兰 10-03：「要返回刚刚在发现页选择的地区栏」）。*/
    s_play_from = g->from_page;
    /* mode==1：这一格是地区块，走地区回调（海外二级分类）*/
    if (g->mode == 1) {
        if (g->on_region) g->on_region(slot, st);
        return;
    }
    if (g->on_play) g->on_play(st);
}

/* 长按 = 收藏/取消。★ 必须自己把滚动按住不放时的高亮收掉，
 *   LVGL 的 LONG_PRESSED 在滑动时不会发，但按下时的压暗效果还在。*/
static void grid_slot_long(lv_event_t *e)
{
    st_grid_t *g = (st_grid_t *)lv_event_get_user_data(e);
    if (!g) return;
    int slot = grid_slot_of(g, lv_event_get_target(e));
    if (slot < 0) return;
    int st = g->idx[slot];
    if (st < 0) return;
    /* mode==1：这一格是【地区块】不是台，收藏没有意义 ——
     *   而且 on_fav 会拿 idx 当【台下标】用，越界。必须在这里拦住。*/
    if (g->mode == 1) return;
    if (g->on_fav) g->on_fav(st);
    grid_show_cell(g, slot, st);      /* 收藏标记要立刻变 */
}

/* 滚动：算出现在是第几行，变了才重填（否则每滚一个像素就重排 10 个 label）*/
static void grid_scroll_cb(lv_event_t *e)
{
    st_grid_t *g = (st_grid_t *)lv_event_get_user_data(e);
    if (!g || g->total <= 0) return;
    int rows = (g->total + GRID_COLS - 1) / GRID_COLS;
    int max_first = rows - g->vis_rows;
    if (max_first < 0) max_first = 0;
    int f = lv_obj_get_scroll_y(g->cont) / g->row_h;
    if (f < 0) f = 0;
    if (f > max_first) f = max_first;
    if (f == g->first) return;
    g->first = f;
    grid_fill(g);
}

/* 建一个网格。x/y/w/h = 容器在页面里的位置与可视区大小。*/
static st_grid_t *grid_create_ex(lv_obj_t *parent, int x, int y, int w, int h, int vis_rows)
{
    static st_grid_t s_grids[4];      /* 4 处用到：首页 / 发现 / 分类页（收藏共用它）*/
    static int s_gn = 0;
    if (s_gn >= 4) s_gn = 0;
    st_grid_t *g = &s_grids[s_gn++];

    memset(g, 0, sizeof(*g));
    if (vis_rows < 1) vis_rows = 1;
    if (vis_rows > GRID_ROWS) vis_rows = GRID_ROWS;
    g->vis_rows = vis_rows;
    g->nslot    = vis_rows * GRID_COLS;
    g->row_h    = h / vis_rows;
    g->cell_h   = g->row_h - 4;
    g->cell_w = (w - 6) / GRID_COLS;
    g->font   = F_ST14;
    g->bg     = C_PANEL;
    g->bg_sel = C_HILITE;
    g->fg     = C_TEXT;
    g->first  = 0;

    g->cont = lv_obj_create(parent);
    lv_obj_remove_style_all(g->cont);
    lv_obj_set_pos(g->cont, x, y);
    lv_obj_set_size(g->cont, w, h);
    lv_obj_set_style_bg_opa(g->cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(g->cont, 0, 0);
    lv_obj_add_flag(g->cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(g->cont, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(g->cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(g->cont, C_LINE, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(g->cont, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(g->cont, 3, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(g->cont, 2, LV_PART_SCROLLBAR);
    lv_obj_add_event_cb(g->cont, grid_scroll_cb, LV_EVENT_SCROLL, g);

    for (int i = 0; i < g->nslot; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        lv_obj_t *c = box(g->cont, col * (g->cell_w + 6), row * g->row_h,
                          g->cell_w, g->cell_h, g->bg, 8);
        lv_obj_add_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(c, ev_press,  LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(c, ev_release, LV_EVENT_RELEASED, NULL);
        lv_obj_add_event_cb(c, grid_slot_clicked, LV_EVENT_CLICKED, g);
        lv_obj_add_event_cb(c, grid_slot_long, LV_EVENT_LONG_PRESSED, g);
        g->cell[i] = c;
        g->nm[i]   = label(c, 0, 0, g->font, g->fg, "");
        lv_obj_center(g->nm[i]);
    }

    /* spacer：LVGL 用子对象包围盒算可滚范围，而格子会随滚动被搬回顶部，
     * 所以必须有一个「永远在底部」的高对象把内容高度顶起来。*/
    g->spacer = box(g->cont, 0, 0, 1, 1, C_BG, 0);
    lv_obj_set_style_bg_opa(g->spacer, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(g->spacer, LV_OBJ_FLAG_CLICKABLE);
    return g;
}

/* 设定网格内容（pool 里是台下标），并回到顶部 */
static void grid_set(st_grid_t *g, const int *pool, int n)
{
    if (n > GRID_MAX) n = GRID_MAX;
    g->total = n;
    for (int i = 0; i < n; i++) g->pool[i] = pool[i];
    g->first = 0;
    lv_obj_scroll_to_y(g->cont, 0, LV_ANIM_OFF);

    int rows = (n + GRID_COLS - 1) / GRID_COLS;
    int content_h = rows * g->row_h;
    int min_h = g->vis_rows * g->row_h;
    if (content_h < min_h) content_h = min_h;
    lv_obj_set_size(g->spacer, 1, content_h);
    grid_fill(g);
}


/* ============================================================
 *  开机画面（splash）
 *
 * ★★ 2026-10-04 第二十八次：加「呼吸光带 + 颜色流转 + 问候语」，
 *   停留 1.2 s → 3.5 s。兰兰：「开机海报页显示时间稍微长一点，
 *   最好是动态的或者有颜色光影或者动态的变化，吸引年轻人使用」，
 *   但「UI 还是要保持深色底色」。
 *
 * ★★★ 为什么底色坚持纯黑（0x000000），不加任何深蓝/深紫：
 *   兰兰说的「深色底」在这块 ST77922 上有个硬约束 ——
 *   ★ 屏本身是 DIO 模式 + invert_color(true)（见 main 里的面板配置），
 *     颜色的「亮/暗」在 LCD 控制器里是反相算过的。
 *     在软件里调一个「很好看的深蓝」，到屏上可能被算成一片脏灰。
 *     纯黑是唯一可预测的深色：它反相后仍是最干净的边界。
 *   ⇒ 光影全部靠【前景元素】（环形光带、logo、字）去做，
 *     底色一个字节都不动。这是「动前景、不动背景」的做法。
 *
 *  三层动效，从慢到快：
 *   ① 3 个同心环：透明度呼吸（周期 2.4 s，相位错开 ⇒ 看起来像涟漪）
 *   ② 环的描边色在三个品牌色之间缓慢流转（周期 9 s）
 *   ③ logo 五根声波柱：高度轻微起伏（周期 1.1 s）
 *   ★ 三层都用同一个 60 ms 定时器推，不各自建定时器 ——
 *     省 3 个 lv_timer 对象，splash 期间 LVGL 池本来就紧。
 * ============================================================ */

/* 呼吸光带的几何（★ 屏是横版 480×320，但 LVGL 软旋转后逻辑分辨率
 *   仍是 LCD_H_RES=480 宽 / LCD_V_RES=320 高）*/
#define SPL_RING_CX   (LCD_H_RES / 2)     /* 240 */
#define SPL_RING_CY   118                 /* 环心 y：让下方文字排得下 */
#define SPL_RING_N    3
#define SPL_RING_MS   60                  /* 推一次动画的间隔 */
#define SPL_DWELL_MS  3500                /* ★ 停留 3.5 s（原 1200） */

/* 品牌三色（流转用）：绿 → 琥珀 → 青 → 绿
 * ★ 存 uint32_t 原始色值、用 lv_color_hex() 转，不直接写 lv_color_t 数组：
 *   lv_color_t 是个结构体（带 f_pred / f_succ 之类的附加字段），
 *   花括号逐字段初始化既容易漏字段、被编译器静默补 0，
 *   也会被 _c_lint.py 的「定长数组项数」检查数错。
 * ★★ 自查教训（写这段时被门禁拦了两次）：
 *   两次都是我的【注释里写了示例数组】，被门禁当成真数组去数项数。
 *   第一次还顺带暴露了 _c_lint.py 自己不剥注释的 bug（已修脚本）。
 *   ⇒ 规矩：注释里不要出现「名字[数字] = {」这种形状。 */
static const uint32_t k_spl_cycle_hex[3] = {
    0x1B9E77,     /* 拾声绿 */
    0xF0B429,     /* 琥珀   */
    0x3AA6C8,     /* 青     */
};

static lv_obj_t *s_spl_ring[SPL_RING_N];
static lv_obj_t *s_spl_bar[5];          /* logo 的五根柱 */
static lv_obj_t *s_spl_greet;           /* 问候语那一行 */
static uint32_t  s_spl_tick;            /* 累计的 tick 数（推导相位）*/
static lv_timer_t *s_spl_timer;

/* logo 五根柱的基准高度百分比（复用 logo_mark 那组 wgt）*/
static const int s_spl_wgt[5] = {40, 70, 100, 66, 46};

/* 问候语：按小时给一句。★ 全部用【现有字库一定有的字】——
 *   F_BODY 主字库 551 字，而这些字都在「早/上/好/午/晚/夜/间/听/个/音/乐/电/台」
 *   这类常用范围里。写生僻字会变方块（铁律 22）。*/
static const char *greeting_for_hour(int h)
{
    if (h < 5)  return "Late night?";
    if (h < 9)  return "Good morning";
    if (h < 12) return "Good morning";
    if (h < 14) return "Good noon";
    if (h < 18) return "Good afternoon";
    if (h < 22) return "Good evening";
    return "Late night?";
}

/* 一拍动画。所有对象都在 splash 上，splash 删了就停。*/
static void splash_anim_cb(lv_timer_t *t)
{
    (void)t;
    s_spl_tick++;

    /* ---- ① 环：透明度呼吸 ----
     * 用三角波而不是 sin：sin 要浮点运算，ESP32 上算 3 个环 × 60 Hz
     * 纯属浪费；三角波用取模 + 折返，几次整数运算就够，视觉差别看不出来。
     * 相位错开 r 档 ⇒ 三层像涟漪往外扩，而不是一起亮灭。*/
    const uint32_t PH = 40;              /* 一个呼吸周期 = 40 拍 = 2.4 s */
    for (int i = 0; i < SPL_RING_N; i++) {
        if (!s_spl_ring[i]) continue;
        uint32_t ph = (s_spl_tick + (uint32_t)i * (PH / SPL_RING_N)) % PH;
        int tri = (ph < PH / 2) ? (int)(ph * 2) : (int)((PH - ph) * 2);
        /* 透明度 20 ~ 85 呼吸 */
        int opa = 20 + (tri * 65) / (int)PH;
        lv_obj_set_style_border_opa(s_spl_ring[i], (lv_opa_t)opa, 0);
    }

    /* ---- ② 描边色流转（周期 9 s = 150 拍，30 拍一档）---- */
    uint32_t cslot = (s_spl_tick / 30) % 3;
    lv_color_t cc = lv_color_hex(k_spl_cycle_hex[cslot]);
    for (int i = 0; i < SPL_RING_N; i++) {
        if (!s_spl_ring[i]) continue;
        /* ★ 内环最亮、外环最暗 ⇒ 有纵深感。
         *   实现方式：内层用本色，外层用【底色与本色各半的混色】
         *   （lv_color_mix），而不是找一个「更暗的本色」——
         *   ★★ LVGL v9 没有 lv_obj_set_style_border_opa_dsc 这个函数
         *      （我写了才被 gcc 拦下来；只有 get_style_border_opa_internal）。
         *      想单独调描边透明度就用 lv_obj_set_style_border_opa()，
         *      但那个会和第 ① 层的呼吸动画互相覆盖 —— 两处都在写
         *      同一个属性，后写的赢 ⇒ 呼吸效果会消失。
         *   ⇒ 结论：呼吸走 opacity、纵深走颜色深浅，两个属性分开管。*/
        lv_color_t dim = lv_color_mix(cc, lv_color_hex(0x000000), LV_OPA_50 + (lv_opa_t)(i * 30));
        lv_obj_set_style_border_color(s_spl_ring[i], dim, 0);
    }

    /* ---- ③ logo 柱起伏（周期 1.1 s ≈ 18 拍）----
     * ★★ 幅度用【绝对像素】不用百分比 —— 10-04 差点写错：
     *   原本写「基准 ±22%」，离线验算出来实际幅度是 30~43%：
     *   三角波 0..(BP-1) 折返后 base+22%*tri/9-11% 并不是 ±11%，
     *   最矮那根（基准 13）被顶到 16，最高的（基准 34）冲到 **45** ——
     *   而 logo 容器只有 34 px，柱子会【顶出容器被裁掉】。
     *   ⇒ 改成「基准 ± 4 px」：幅度封顶 8 px，任何一根都不越界。
     *     容器高度也给到 40，34+4 有地方放。
     *   为什么不是 ±2 px：13 px 的柱动 2 px 只有 15% 变化，
     *   1.1 s 一周期在 60 ms 一帧下只差 1 px，看着像没动。
     *   4 px 是「看得见又不夸张」。*/
    const uint32_t BP = 18;
    for (int i = 0; i < 5; i++) {
        if (!s_spl_bar[i]) continue;
        uint32_t ph = (s_spl_tick + (uint32_t)i * 3) % BP;
        int tri = (ph < BP / 2) ? (int)(ph * 2) : (int)((BP - ph) * 2);
        int amp = (tri * 8) / (int)BP - 4;      /* -4 ~ +3 */
        int h = 34 * s_spl_wgt[i] / 100 + amp;
        if (h < 4)  h = 4;
        if (h > 40) h = 40;
        lv_obj_set_size(s_spl_bar[i], 4, h);
        lv_obj_set_pos(s_spl_bar[i], i * 6, (40 - h) / 2);
    }
}

static void splash_skip_cb(lv_event_t *e)
{
    (void)e;
    /* 触摸提前跳过：兰兰要「吸引人」但不能逼人看广告。
     * 延迟满 3.5 s 自动走；点一下立刻走。*/
    if (s_splash_timer) {
        lv_timer_del(s_splash_timer);
        s_splash_timer = NULL;
    }
    splash_done_cb(NULL);
}

static void splash_build(void)
{
    s_splash_scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(s_splash_scr);
    lv_obj_set_style_bg_color(s_splash_scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_splash_scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_splash_scr, LV_OBJ_FLAG_SCROLLABLE);
    /* ★ 整页可点 = 点击提前跳过。splash_scr 本身没有 CLICKABLE 标志，
     *   不加的话只有点到具体子对象才收得到事件。*/
    lv_obj_add_flag(s_splash_scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_splash_scr, splash_skip_cb, LV_EVENT_CLICKED, NULL);

    /* ---- 呼吸光带：3 个同心环 ---- */
    for (int i = 0; i < SPL_RING_N; i++) {
        /* ★ 环径 120/168/216，不是老版的 150/212/274。
         *   老版最大环 274 px、环心 y=118 ⇒ 上边落在 118-137 = **-19**
         *   ⇒ 被屏幕顶边裁掉一截，一直没人发现是因为它「空心描边」，
         *   少掉的那截在深色底上看着像本来就是那样。
         *   新账：最大环 216 ⇒ 纵向 10~226、横向 132~348，都在屏内。*/
        int d = 120 + i * 48;
        lv_obj_t *ring = box(s_splash_scr, SPL_RING_CX - d / 2,
                             SPL_RING_CY - d / 2, d, d,
                             lv_color_hex(0x0F1A18), d / 2);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ring, 2, 0);
        lv_obj_set_style_border_color(ring, lv_color_hex(k_spl_cycle_hex[0]), 0);
        lv_obj_set_style_border_opa(ring, LV_OPA_50, 0);
        s_spl_ring[i] = ring;
    }

    /* ---- logo：自己建五根柱（要单独控制每一根的高度，所以不用 logo_mark）---- */
    {
        lv_obj_t *c = lv_obj_create(s_splash_scr);
        lv_obj_remove_style_all(c);
        /* 容器高 40，不是 34 —— 柱子在动画里会最高到 34+3=37，
         *   容器只给 34 的话柱子顶边会被裁掉一截，看着像断了一节。*/
        lv_obj_set_pos(c, SPL_RING_CX - 15, 86);
        lv_obj_set_size(c, 30, 40);
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        for (int i = 0; i < 5; i++) {
            int bh = 34 * s_spl_wgt[i] / 100;
            s_spl_bar[i] = box(c, i * 6, (40 - bh) / 2, 4, bh, C_GREEN, 2);
        }
    }

    lv_obj_t *t = label(s_splash_scr, 0, 130, F_TITLE, C_TEXT, "XianDial");
    lv_obj_set_width(t, LCD_H_RES);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *en = label(s_splash_scr, 0, 188, F_TINY, C_GREEN, "X I A N   D I A L");
    lv_obj_set_width(en, LCD_H_RES);
    lv_obj_set_style_text_align(en, LV_TEXT_ALIGN_CENTER, 0);

    /* ★ 问候语：按开机时刻给一句。放在英文名下面，
     *   比放在最下面（原来「正在准备…」那行）更醒目 ——
     *   兰兰要的是「吸引年轻人」，问候是整页唯一一句【对人说的话】。*/
    {
        int h = 12;
        int m = now_minutes();
        if (m >= 0) h = (m / 60) % 24;
        s_spl_greet = label(s_splash_scr, 0, 206, F_BODY, C_AMBER,
                            greeting_for_hour(h));
        lv_obj_set_width(s_spl_greet, LCD_H_RES);
        lv_obj_set_style_text_align(s_spl_greet, LV_TEXT_ALIGN_CENTER, 0);
    }

    box(s_splash_scr, (LCD_H_RES - 120) / 2, 234, 120, 1, C_LINE, 0);

    /* ★ 开机页署名也放大提亮（兰兰：「毕竟是推广作用的展示」）*/
    /* ★ 10-05 兰兰反馈「作者署名字体太大，挡住下面字」：
     *   原来用 F_H2(24px)，而下面 266 处还有一行英文副标题（y=266），
     *   24px 字高就吃掉 24px，244+24=268 ⇒ 两行压在一起。
     *   改F_MID(16px)，字高 16px，244~260，与 266 那行留 6px 间隙。 */
    lv_obj_t *sign = label(s_splash_scr, 0, 244, F_MID, C_TEXT, "© Lanlan Eternal");
    lv_obj_set_width(sign, LCD_H_RES);
    lv_obj_set_style_text_align(sign, LV_TEXT_ALIGN_CENTER, 0);

    lv_obj_t *en2 = label(s_splash_scr, 0, 266, F_TINY, lv_color_hex(0x55606C),
                          "L A N L A N   E T E R N A L");
    lv_obj_set_width(en2, LCD_H_RES);
    lv_obj_set_style_text_align(en2, LV_TEXT_ALIGN_CENTER, 0);

    /* ★ 底部提示改成「点一下跳过」—— 加了跳过功能就必须说清楚，
     *   否则用户盯着不放会以为卡住。原来的「正在准备拾声…」删掉：
     *   它描述的是内部进度，而 splash 只显示固定 3.5 s，
     *   这个文案本身就是假的（同夜间页那三行假数据一个毛病）。*/
    lv_obj_t *hint = label(s_splash_scr, 0, 292, F_TINY, C_MUTED, "Tap to start");
    lv_obj_set_width(hint, LCD_H_RES);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);

    lv_screen_load(s_splash_scr);

    /* ★ 动画定时器：60 ms 一拍，推进去 Splash 的生命周期绑定 ——
     *   splash_done_cb 里会 del 它。*/
    s_spl_tick = 0;
    s_splash_done = false;                     /* 只允许撤一次（见 splash_done_cb 注释）*/
    s_splash_timer = lv_timer_create(splash_done_cb, SPL_DWELL_MS, NULL);
    s_spl_timer = lv_timer_create(splash_anim_cb, SPL_RING_MS, NULL);
}

/* ============================================================
 *  顶栏
 * ============================================================ */
static void topbar_build(lv_obj_t *scr)
{
    lv_obj_t *bar = box(scr, 0, 0, LCD_H_RES, TOPBAR_H, C_BG, 0);
    box(scr, 0, TOPBAR_H - 1, LCD_H_RES, 1, C_LINE, 0);

    logo_mark(bar, 12, 8, 22, C_GREEN);

    /* ★★★★★★ 10-05 第七次：「拾声」当【硬件商标】重做（v1.39）
     *
     *   兰兰原话：「拾声这个名称字体作为硬件商标的字体不好看，
     *     白色字体像素感很严重，商标展示很重要，可以重点美化一点，
     *     拾声字体大小应该和署名一样即可」
     *
     *   —— 病根不是「字小」，是【24px 白字】这个组合本身难看：
     *     Ark Pixel 是像素字体，24px 是 12px 的 2 倍整数缩放（本身够锐），
     *     但纯白 #F2F5F8 落在 #0B0E12 深底上对比度接近 20:1 ——
     *     每个像素台阶都被拉成硬边，看着像「贴上去的色块」而不是字。
     *     ⇒ 美化靠三件事，都不额外吃内存：
     *       ① 字号 24 → 16（= 署名那档）：像素台阶从 2px 缩到 1.33px，
     *          硬边变细，锐利度靠 16px 自己的笔画撑着，不像色块了。
     *       ② 色 白 → 品牌绿 C_GREEN：与左边 logo 声波柱同一个色，
     *          「图标 + 字」读起来是一个整体商标，而不是两个东西。
     *          对比度从 ~20:1 降到 ~9:1，硬边存在感大幅下降。
     *       ③ 加 3px 字间距：两个字分开 3px，像 logo 的字距，
     *          不再是「两个方块贴在一起」。
     *       ④ 下面加一条双色装饰条（亮绿 14 + 青 12，共 1 个 box 的高度 2px），
     *          这是最省内存的「商标感」来源 —— 纯文字没有视觉锚点，
     *          一条横线就把「这是品牌名」这件事说清楚了。
     *       ⚠️ 装饰条只用【2 个 box】（每个约 300 B），不放底衬大色块：
     *          LVGL 池只剩 15 KB，一块 96×26 的底衬要 1 KB+，不值。
     *
     *   ★ 第七次横账（480 宽，全部 hmtx 实测，unitsPerEm=1200）：
     *     12 ~ 42   logo 声波柱（30 宽）
     *     48 ~ 79   「拾声」16px + 3px 字距 = 26.7+3 = 30.7
     *     48 ~ 79   装饰条 y=31 高 2（亮绿 14 + 电波青 12）
     *     96        竖分隔线（y=10 高 18）
     *     106 ~ 187 「© 永远的兰兰」16px = 80.7
     *     214 ~ 348 时钟（右对齐 16px）
     *     352 ~ 366 电池图标 (14px)
     *     392 ~ 406 SD 卡图标 (14px)
     *     404 ~ 444 容量文字（12px）
     *     450 ~ 464 WiFi 图标 (14px)
     *   ⇒ 拾声(79) 与署名(106) 之间空 27px，两块牌子各自分开。
     */
    {
        lv_obj_t *brand = label(bar, 48, 10, F_MID, C_GREEN, "XianDial");
        lv_obj_set_style_text_letter_space(brand, 3, 0);
        /* 双色装饰条：亮绿 + 电波青，两段各圆角，像一条被截断的频谱 */
        box(bar, 48, 31, 14, 2, C_GREEN, 1);
        box(bar, 64, 31, 12, 2, C_CYAN, 1);
    }
    box(bar, 96, 10, 1, 18, C_LINE, 0);

    /* ★ 署名（兰兰 10-03，两次反馈）：
     *   「字体和颜色更醒目，毕竟是推广作用的展示」
     *   「如果标题栏太挤，署名可以小一点点就好了，颜色醒目是关键」
     *   ⇒ 字号只从 11px 提到 14px（小一点点，不跟 18px 的「拾声」抢地方），
     *     醒目靠【颜色】：0x8A96A3 那种灰几乎看不见，换成 C_GREEN 亮绿
     *     在 0x0B0E12 深底上对比度极高，一眼就能扫到。
     *   不加底衬 box：多一个 lv_obj ≈ 300 B，池只剩 15 KB，没必要为装饰花内存。
     *
     * ★ 10-05 兰兰反馈「作者署名可稍微放大」⇒ F_BODY(12px) → F_MID(16px)。
     *   16px 下「© 永远的兰兰」约 100px 宽，106+100=206 < 时钟框 214 ⇒ 不叠。*/
    lv_obj_t *sign = label(bar, 106, 11, F_MID, C_GREEN, "© Lanlan Eternal");

    /* ---- 右上角：时钟 + SD 卡（图标紧贴容量） + WiFi ----
     * ★ 10-03 第十次（兰兰原话）：
     *   「标题栏出现两个内存容量显示，在TF卡旁边正确的显示容量，
     *     在WiFi标志左边还有一个淡灰色的容量显示是多余的」
     *   「标题栏显示就绪的含义，如果不重要可以替换为当前时间」
     *   ⇒ 「● 就绪 · TF 29G」那一格改成【当前时间】，
     *     容量只在 SD 卡图标右边出现一次。
     * ★ 第十一次（兰兰：「保留的那个距离内存卡标志太远，而且不是绿色字体」）：
     *   容量文字原来框在 396~448 且右对齐 ⇒ 字实际落在 420 附近，
     *   离 SD 图标（378~392）空了 28 px，看着像两样不相干的东西。
     *   ⇒ 改成【左对齐紧贴图标】，并用 C_GREEN + 14px 字号（有卡时）。
     *
     * ★ 顶栏横向账（480 宽，写死坐标前先算一遍，别再叠字）：
     *     106 → 190  署名「© 永远的兰兰」(14px ≈ 84)
     *     214 → 370  时钟（右对齐，18px，「18:34」≈ 56）
     *     378 → 392  SD 卡图标 (14px)
     *     396 → 436  容量文字（左对齐紧贴图标，14px，「29 G」≈ 34）
     *     444 → 458  WiFi 图标 (14px)
     *   历史老账：SD 容量框曾是 402~468 而 WiFi 图标在 452 ⇒ 叠了 16 px。
     *
     * ★ 第十二次（兰兰：「当前时间字体可以大一点，醒目一点，如足够地方」
     *   +「标题时钟可以往中间移动点」）：
     *   ① 11px 琥珀色太不起眼 ⇒ 提到 F_H2(18px)。颜色保持 C_AMBER
     *     （琥珀在深底上比绿更醒目，且和「拾声」的白、署名的绿、容量的绿
     *     都能一眼区分开）。
     *   ② 「往中间移」：原来框 198~372（174 宽）而「18:34」只 56 px 宽，
     *     靠右对齐 ⇒ 字落在 316~372，整个屏幕正中间偏右，看着是「贴着 SD 卡」。
     *     现在框 214~330（116 宽）⇒ 字落在 274~330，中心在 302，
     *     顶栏中线是 240 ⇒ 中心偏右 62 px，但比原来（中心 344）左移了 42 px。
     *     不敢再往左：署名结束于 190，要留 20 px 呼吸。
     *
     *   ⚠️ 顺带说明一件【故意没做】的事：第 7 页要显示满 12 格电平条，
     *   100 ms 的 LVGL 定时器在长时间重绘时理论上可能被饿死。解法是
     *   `CONFIG_FREERTOS_HZ=100`（心跳 2→10 拍/秒），但改 sdkconfig 一次 =
     *   **LVGL 877 文件 + wpa_supplicant 全量重编 25~30 分钟**，
     *   为「可能饿死」付这个代价不划算。现状：频谱与电平实测都正常跳动。
     *   ⇒ **别为了预防性风险去动 sdkconfig。**
     *
     * ★ 顶栏横向账（480 宽，写死坐标前先算一遍，别再叠字）：
     *     106 → 190  署名「© 永远的兰兰」(14px ≈ 84)
     *     214 → 330  时钟（右对齐，18px，「18:34」≈ 56 ⇒ 实际落在 274~330）
     *     336 → 350  电池图标 (14px)              ← 10-04 新增
     *     352 → 380  电量文字（14px，「85」≈ 16）   ← 10-04 新增
     *     378 → 392  SD 卡图标 (14px)             ← 与电量重叠 14px！
     *     396 → 436  容量文字（左对齐紧贴图标，14px，「29 G」≈ 34）
     *     444 → 458  WiFi 图标 (14px)
     *   历史老账：SD 容量框曾是 402~468 而 WiFi 图标在 452 ⇒ 叠了 16 px。
     *
     * ⚠️⚠️ 10-04 教训：上面这份账**自己算错了**（电量 352~380 与 SD 378~392
     *   重叠 14 px）。插电池加电量图标时我把「往左挤」当成理所当然，
     *   实际右边已经被三个指示物占满 —— 顶栏是这台机唯一的全局指示区，
     *   加任何东西之前必须把四者（时钟/电量/SD/WiFi）一起重算。
     *
     * ★ 第四次横账（10-04，加电池图标后的实际排布）——
     *   把 SD 容量框与 WiFi 图标整体【右移 14 px】给电池腾位置：
     *     106 → 190  署名「© 永远的兰兰」(14px ≈ 84)
     *     214 → 330  时钟（右对齐 18px，「18:34」≈ 56 ⇒ 实际 274~330）
     *     374 → 388  ★电池图标 (14px)  ← 新增
     *     392 → 406  SD 卡图标 (14px)  ← 原 378，右移 14
     *     404 → 444  容量文字（14px，「29 G」≈ 34） ← 原 396，右移 8
     *     450 → 464  WiFi 图标 (14px)  ← 原 444，右移 6
     *   ★ 电池图标只有 16 px 宽，**放不下百分比数字** ⇒ 顶栏只用【颜色】表电量
     *     （绿/琥珀/红），百分比数字放到夜间页与状态页那两行整宽的地方。
     *   ⚠️ 顶栏再加任何东西之前，先把这四个一起重算，别只算自己那一个。
     *
     * ★★★★★★ 10-05 第六次横账（v1.38）—— 兰兰真机复验：
     *   「时钟没有向左移动」＋「电源标志向左移动了，挡住了时钟分钟位」
     *
     *   【我上一版算错了，注释还写着"更靠中间"，实际是右移了 36px —— 假账害死人】
     *   原因：把框宽从 116 加到 152，右边缘随之从 330 推到 366，
     *   而右对齐的字是【贴着右边缘】放的 ⇒ 字从 274~330 挪到 310~366。
     *   同时电池左移到 352，正好落在 310~366 里间 ⇒ 压住分钟位。
     *   ⇒ **教训：右对齐的框，加宽等于右移；改框宽前必须先算字落在哪。**
     *
     *   ★ 本版所有宽度改成【实测值】（用 fontTools hmtx 按 unitsPerEm 折算），
     *     不再拍脑袋估算 —— 前五次的账全部是估的，所以连错两次。
     *       12px 署名 78px ｜ 16px 署名 104px ｜ 24px 署名 156px
     *       24px「18:34」= 4×12 + 8 = 56px（数字 advance 600/1000，冒号 400/1000）
     *
     *   第六次横账（480 宽）：
     *     106 → 210  署名「© 永远的兰兰」(16px = 104px 实测)
     *     214 → 348  时钟（24px，右对齐，框宽 134；字落 292~348，中心 320）
     *     352 → 366  电池图标 (14px)              ← 保持 352，与时钟框留 4px
     *     392 → 406  SD 卡图标 (14px)              ← 不动（兰兰两次确认正确）
     *     404 → 444  容量文字（12px，「29 G」≈ 34）   ← 不动
     *     450 → 464  WiFi 图标 (14px)              ← 不动
     *
     *   时钟为什么只能到中心 320（而不是顶栏中线 240）：
     *   左边被 210px 宽的署名占着，署名不能压（那是兰兰要求放大的），
     *   右边被电池/SD/容量/WiFi 占着 112px。剩下 210~352 这 142px 空档，
     *   中心是 281；字宽 56 ⇒ 右对齐后落在 292~348（中心 320）。
     *   要真正居中到 240 得让时钟【居中对齐】而非右对齐，代价是
     *   「--:--」占位期间（未校时时）字会左右跳。⇒ 保持右对齐。
     */
    s_lbl_topstat = label(bar, 214, 11, F_MID, C_AMBER, "--:--");
    /* ★★ 10-05 第七次：F_H2(24px) → F_MID(16px)（兰兰：「时钟字体大小也和署名一样，
     *   这样整体协调」）。右对齐框宽也跟着重算 ——
     *   ★ 铁律：改字号必须同时改框宽的账，否则又是「按 24px 时代的框放 16px 的字」。
     *     实测 16px：「18:34」= 33.3px、「--:--」= 22.2px（数字 advance 600/1000、
     *     冒号与短横 400/1000，unitsPerEm=1200）。
     *   框 214~348（宽 134）右对齐 ⇒ 字落 315~348，离电池图标(352)还有 4px。
     *   为什么还留 4px 而不是顶到 352：--:-- 占位期只有 22.2px 宽，
     *   贴着 352 会在校时前后看到字「跳一下」，留白反而更稳。*/
    lv_obj_set_width(s_lbl_topstat, 134);
    lv_obj_set_style_text_align(s_lbl_topstat, LV_TEXT_ALIGN_RIGHT, 0);

    s_ic_sd = label(bar, 392, 7, F_ICON, C_MUTED, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(s_ic_sd, C_GREEN, 0);
    /* 左对齐、紧贴图标右边 4 px；用 F_BODY(14px) 与绿色，
     * 这样「图标 + 容量」读起来是一件事。字号从 11 提到 14 是有意的：
     * 11px 灰字在 0x0B0E12 深底上太弱，等于看不见。*/
    s_lbl_sdinfo = label(bar, 404, 11, F_BODY, C_GREEN, "TF");
    lv_obj_set_width(s_lbl_sdinfo, 40);

    /* WiFi 图标（最右）*/
    s_ic_wifi = label(bar, 450, 7, F_ICON, C_MUTED, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(s_ic_wifi, C_MUTED, 0);

    /* ---- 电池图标（10-04 加，10-05 左移）----
     * ★ 位置 352~366：★ 10-05 从 374 左移 22px。
     *   原因：原来 374 与 SD 图标 392 只差 18px，图标 14px 宽 ⇒ 净间隙 4px，
     *   兰兰真机反馈「充电和电源标志向左移动一点点，太紧贴内存卡标志了」。
     *   左移后与 SD 拉开 40px，且离时钟右端（366）刚好接上，不留空隙浪费。
     *   宽度只有 14 px ⇒ **放不下百分比文字**，只用颜色表示电量档位
     *   （绿/琥珀/红）。百分比数字放夜间页和状态页（那里有整行空间）。*/
    s_ic_batt = label(bar, 352, 7, F_ICON, C_MUTED, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(s_ic_batt, lv_color_hex(0x3A424C), 0);
    s_batt_shown_pct = -2;      /* -2 = 还没读过，强制首次刷 */
}

/* ============================================================
 *  左侧竖导航
 * ============================================================ */
static void nav_clicked(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    goto_page(idx);
    tap_note("Menu");
}

static void nav_build(lv_obj_t *scr)
{
    static const char *names[NAV_N] = {"Home", "Browse", "Playing", "Night", "Status"};
    lv_obj_t *rail = box(scr, 0, TOPBAR_H, RAIL_W, CONT_H, C_PANEL, 0);
    box(scr, RAIL_W - 1, TOPBAR_H, 1, CONT_H, C_LINE, 0);

    for (int i = 0; i < NAV_N; i++) {
        lv_obj_t *b = box(rail, 4, 8 + i * 52, RAIL_W - 8, 46, C_HILITE, 12);
        lv_obj_set_style_bg_opa(b, i == 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
        /* ★★ 第二十二次：导航按钮不挂 ev_press/ev_release。
         *   它们按下时改 opa ⇒ 触发重排 ⇒ LVGL 在同一帧发现指针下的对象变了
         *   就把它判成滚动并放弃 CLICKED —— 20-04「跳转键要碰几次」的同款坑，
         *   当时只修了跳转键，导航栏这几个一直没摘。*/
        lv_obj_add_event_cb(b, nav_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_nav_btns[i] = b;

        s_nav_marks[i] = box(b, 8, 9, 4, 28, i == 0 ? C_GREEN : C_MUTED, 2);

        /* ★★ 兰兰：「首页左边导航栏字体都改成绿色」。
         *   原来未选中的用 C_MUTED(#7C8894 灰)，在 C_PANEL(#151A21) 底上
         *   对比度只有 3.4:1，看着像「禁用」。
         *   ⇒ 全部改绿；当前页用亮绿 C_GREEN，其余用暗绿 0x1F7A5C ——
         *     「都是绿的」满足要求，同时当前页仍能一眼分出来
         *     （靠亮度差，而不是靠颜色差）。*/
        s_nav_labels[i] = label(b, 22, 14, F_TINY,
                                i == 0 ? C_GREEN : C_NAV_DIM, names[i]);
    }
}

/* ============================================================
 *  首页 —— 正在播放 + 栏目条（横滑）+ 双列电台网格（上下滑）
 * ============================================================ */

static lv_obj_t *s_lbl_list_title;    /* 网格上方那行「全部 · 共 N 台」*/
/* ★ 10-04 第二十八次：原来只有一张「N 个常用台」的收藏卡，
 *   现在收藏搬去标签条了，这里改显示【最近听过】。
 *   s_lbl_favcnt 保留（收藏页标题、收藏数还靠它），但不再指向首页卡。*/
static lv_obj_t *s_lbl_favcnt;        /* 收藏数量文字（收藏页/全屏页用）*/
static lv_obj_t *s_lbl_histcnt;       /* 首页「最近听过」卡：最近一电台名 */
static lv_obj_t *s_lbl_sdinfo2;       /* 首页「音乐库」卡：内存卡音频数 */
static int       s_hr_sel = -1;       /* 当前选中的台（g_stations 下标）*/
/* ★ 10-04 第二十九次：记着上一次看到的 SD 挂载状态。
 *   app_main 里 ui_init() 早于 app_sd_mount()，建卡时卡还没挂上，
 *   所以首页「音乐库」那行字要等挂载状态【发生变化】时再刷一次。*/
static bool      s_last_sd_mounted = false;
static int       s_pool[GRID_MAX];    /* 当前筛选结果的下标池 */
static st_grid_t *s_grid_home;        /* 首页那层（固定「全部」）*/
static st_grid_t *s_grid_find;        /* 发现页那层 */
static st_grid_t *s_grid_list;        /* 第 7 页全屏列表（栏目/省份/收藏共用一个）*/
/* 第 7 页标题「新闻综合 · 共 128 台」。ovs_grid_refresh() 在本行之前就要用它，
 * 所以声明提前到这儿，定义仍留在文件后段（page_list_build 里）。*/
static lv_obj_t *s_lbl_list_title2;

/* 前向声明：utf8_copy_n / ev_open_hist / home_cards_refresh 都定义在文件后段，
 * 但首页建页（page_home_build）要用。*/
static void utf8_copy_n(char *dst, size_t dstsz, const char *src, int max_chars);
static void ev_open_hist(lv_event_t *e);
static void home_cards_refresh(void);
/* ★ on_grid_play / on_grid_fav（第 1784/1785 行）在 home_grid_refresh（1858）
 *   之前，但它们要调它重排 ⇒ 必须在这里先声明。
 *   ★★★ 别为了「顺序好看」把它挪到 on_grid_fav 后面：
 *   网格排序的真相源应该在【建网格的地方】，不是在使用者后面。*/
static void home_grid_refresh(void);

/* 第 7 页当前在看什么 */
enum { LIST_BY_CAT = 0, LIST_BY_PROV, LIST_BY_FAV, LIST_BY_HIST,
LIST_BY_ALL_OVS,      /* 把全部海外台平铺一页（保留，给日志对照用）*/
LIST_BY_OVS_REGION };  /* ★ 海外【二级】：先列 11 个地区，点一个再看它的台 */
static int         s_list_mode  = LIST_BY_CAT;
static int         s_list_cat   = 0;
static int         s_list_prov  = 0;
static const char *s_list_title = "";
static int         s_list_total = 0;
static int         s_list_back  = 0;   /* 返回键回哪一页 */

static void on_grid_play(int idx);
static void on_grid_fav(int idx);
static void list_page_refresh(void);   /* 第 7 页全屏列表刷新（定义在文件后段）*/
static void goto_page(int idx);        /* 切页（定义在网格段之后，这里先声明）*/
static void ev_open_fav(lv_event_t *e); /* 收藏页入口（定义在文件后段）*/
/* 第 8 页 WiFi 设置入口（定义在文件后段）。★ 必须在这里前向声明：
 * 状态页 page_stat_build 在 3400 多行就要挂它，而定义在 3500 多行。*/
static void ev_open_wifi_page(lv_event_t *e);
/* 这一页的静态部分（标题/状态行/动作键）建页时建一次；
 * wifi_page_destroy 的声明在 goto_page 之前（那里更早）。*/
static void wifi_page_ensure(void);
/* ★ 第二十四次：WiFi 页的 60 ms 待办定时器已删（屏幕键盘整个去掉了，
 *   就没有「在事件回调里删/建对象」这个需求了）。*/
/* 海外二级：第 7 页在「地区块」与「该地区的台」之间来回切（定义在文件后段）*/
static void list_page_back_to_ovs(void);
static void ovs_grid_refresh(void);

static void home_update_card(int st_idx)
{
    int n = app_radio_station_count();
    if (st_idx < 0 || st_idx >= n) return;
    const net_station_t *st = app_st_get(st_idx);

    if (s_lbl_station) {
        char nm[64];
        utf8_copy_n(nm, sizeof(nm), st->name, 16);
        lv_label_set_text(s_lbl_station, nm);
    }
    if (s_lbl_source) {
        const char *pn = (st->prov == NET_PROV_NONE) ? "Nationwide"
                                                      : xs_prov_long_en[st->prov % NET_PROV_N];
        lv_label_set_text_fmt(s_lbl_source, "%s · %s · 64 kbps", pn,
                              xs_cat_name_en[st->cat % NET_CAT_N]);
    }
    /* ★ 这里【不】写 s_lbl_playname：第 3 页才是真正的播放界面，
     *   曲名由 player_refresh() 管；首页再改它就会串台。*/
    if (s_lbl_playstate) {
        lv_label_set_text_fmt(s_lbl_playstate, "%s · %s",
                              app_radio_is_paused() ? "Paused" : "Playing",
                              st->name);
    }
}

/* 把符合筛选条件的台下标收进 pool，返回个数。
 * cat < 0 = 不按栏目筛；prov < 0 = 不按省份筛。*/
static int collect_stations(int *pool, int cap, int cat, int prov)
{
    int n = app_radio_station_count();
    int pn = 0;
    for (int i = 0; i < n && pn < cap; i++) {
        if (cat  >= 0 && (int)app_st_get(i)->cat  != cat)  continue;
        if (prov >= 0 && (int)app_st_get(i)->prov != prov) continue;
        pool[pn++] = i;
    }
    return pn;
}

/* 首页那层网格：固定「全部」（栏目走全屏页） */
/* ★★★ 10-04 第二十八次：首页网格不再只是「按台单顺序的前 6 台」。
 *
 *   兰兰：「在启动时首页显示了全部页面的前六个在前排直观显示，
 *         如果有收藏页保存电台，可以在首页最优先显示收藏页，
 *         就是栏目增加收藏放第一，如果有历史播放最好」
 *   ⇒ 三段拼接：**收藏 → 历史 → 台单其余**，前 6 格优先给它们。
 *
 *   为什么是这个顺序（不是听播放次数）：
 *     收藏是【用户主动表达过的意图】——我明确想留这个台；
 *     历史是【刚才发生过的事实】——我上次的注意力在哪；
 *     台单顺序是【默认值】——没什么信息量。
 *     三者可信度递减，所以放前面的理由递减。
 *
 *   ★ 去重：三段之间必须互相去重，否则同一个台会占掉 6 格里的 3 格。
 *     app_fav_list() 和 app_hist_top() 各自只保证【自己内部】不重复，
 *     跨段的重叠要在这里用一个已用数组挡掉。*/
static void home_grid_refresh(void)
{
    int n = 0;
    static bool used[512];        /* ★ 定长 512：GRID_MAX 是这个量级，
                                    越界访问会读坏栈上的相邻变量。
                                    台单 686 台但首页只取 6 个，
                                    前 6 个下标不可能超过 512。*/

    memset(used, 0, sizeof(used));

    /* ---- 第 1 段：收藏 ---- */
    int fav[16];
    int nf = app_fav_list(fav, (int)(sizeof(fav) / sizeof(fav[0])));
    for (int i = 0; i < nf && n < GRID_MAX; i++) {
        int idx = fav[i];
        if (idx < 0 || idx >= 512 || used[idx]) continue;
        used[idx] = true;
        s_pool[n++] = idx;
    }

    /* ---- 第 2 段：历史（最近听的在前）---- */
    int hist[16];
    int nh = app_hist_top(hist, (int)(sizeof(hist) / sizeof(hist[0])));
    for (int i = 0; i < nh && n < GRID_MAX; i++) {
        int idx = hist[i];
        if (idx < 0 || idx >= 512 || used[idx]) continue;
        used[idx] = true;
        s_pool[n++] = idx;
    }

    /* ---- 第 3 段：台单其余，按原顺序补满 ---- */
    int rest[256];
    int nr = collect_stations(rest, (int)(sizeof(rest) / sizeof(rest[0])), -1, -1);
    for (int i = 0; i < nr && n < GRID_MAX; i++) {
        int idx = rest[i];
        if (idx < 0 || idx >= 512 || used[idx]) continue;
        used[idx] = true;
        s_pool[n++] = idx;
    }

    /* ★ v1.40：网格按需建 ⇒ 可能为 NULL。
     *   本函数在启动时被 page_home_build 调过一次（那时网格还没建，
     *   因为改成由 grid_pages_ensure 建），之后由 ensure 再调。
     *   ⇒ 两种情况下都可能进来，判空是必须的。*/
    if (!s_grid_home) {
        ESP_LOGW(TAG, "home: 网格还没建 —— 先建再填台");
        grid_pages_ensure(0);
        if (!s_grid_home) return;
    }
    grid_set(s_grid_home, s_pool, n);
    if (s_lbl_list_title) {
        int total = app_radio_station_count();
        if (n < total) {
            /* ★ 说清楚「只显示了 6 个」而不是撒谎说「共 6 台」——
             * 兰兰能看到还剩多少台没显示，才知道要点栏目进全屏页。*/
            lv_label_set_text_fmt(s_lbl_list_title,
                                  "Fav/History first - %d of %d",
                                  n, total);
        } else {
            lv_label_set_text_fmt(s_lbl_list_title,
                                  "All stations - %d - scroll for more", n);
        }
    }
    ESP_LOGI(TAG, "home grid: %d 格（收藏 %d + 历史 %d + 其余补满）", n, nf, nh);
}

/* 点某一格：★ 真的开始播这一台，并跳到播放页（「按了有反应」看得见）*/
static void on_grid_play(int idx)
{
    if (idx < 0 || idx >= app_radio_station_count()) return;
    s_hr_sel = idx;
    home_update_card(idx);
    tap_note(app_st_get(idx)->name);
    /* 网格里正在播的那格要高亮，播完把几层都重填一次 */
    if (s_grid_home) grid_fill(s_grid_home);
    if (s_grid_find) grid_fill(s_grid_find);
    if (s_grid_list) grid_fill(s_grid_list);

    esp_err_t err = app_radio_play_station(idx);
    if (err != ESP_OK) {
        /* 具体原因由播放页那一行显示（app_radio_last_error 给中文原因）*/
        ESP_LOGW(TAG, "play_station(%d) -> %s", idx, esp_err_to_name(err));
    }
    /* ★★ 10-04 第二十八次：刚播的这一台已经进了历史（app_radio 里记的），
     *   而首页网格的排序依赖历史 ⇒ 必须重排。
     *   ★ 只在首页建过网格时才重排：网格是懒建的，
     *     切台发生在播放页时 s_grid_home 可能还是 NULL。
     *   ⚠️ 顺序：先刷网格（它会按新顺序重填），再刷卡片
     *     （卡片显示「最近听的台名」，依赖的也是历史）。*/
    if (s_grid_home) home_grid_refresh();
    home_cards_refresh();

    goto_page(2);
    player_refresh();
}

/* 长按一格 = 收藏 / 取消收藏。★ 收藏存 NVS，掉电不丢（见 app_fav.c）*/
static void on_grid_fav(int idx)
{
    if (idx < 0 || idx >= app_radio_station_count()) return;
    bool had = app_fav_has(idx);
    esp_err_t r = app_fav_toggle(idx);
    if (r != ESP_OK) {
        ESP_LOGW(TAG, "fav_toggle(%d) -> %s", idx, esp_err_to_name(r));
        tap_note("Favorites full (max 64)");
        return;
    }
    tap_note(had ? "Removed" : "Favorited");
    /* 所有层的收藏标记都要跟着变 */
    if (s_grid_home) grid_fill(s_grid_home);
    if (s_grid_find) grid_fill(s_grid_find);
    if (s_grid_list) grid_fill(s_grid_list);
    if (s_lbl_favcnt) {
        lv_label_set_text_fmt(s_lbl_favcnt, "%d favorites", app_fav_count());
    }
    /* ★★ 10-04 第二十八次：收藏是首页排序的【第 1 段】，
     *   收藏/取消都会改变首页前 6 格是谁 —— 只 grid_fill 不重排的话，
     *   刚收藏的台不会真的「顶到最前面」，只是高亮变了。*/
    if (s_grid_home) home_grid_refresh();
}

static void ev_home_play(lv_event_t *e)
{
    (void)e;

    if (app_radio_is_playing()) {
        app_radio_toggle_pause();
        tap_note(app_radio_is_paused() ? "Pause" : "Resume");
    } else if (s_hr_sel >= 0) {
        app_radio_play_station(s_hr_sel);
        tap_note(app_st_get(s_hr_sel)->name);
    } else {
        tap_note("Pick a station below");
        return;
    }

    s_playing = !app_radio_is_paused();
    if (s_playbtn_lbl)   lv_label_set_text(s_playbtn_lbl, s_playing ? "Pause" : "Playing");
    if (s_lbl_playbtn)   lv_label_set_text(s_lbl_playbtn, s_playing ? "Pause" : "Playing");
    if (s_hr_sel >= 0)   home_update_card(s_hr_sel);
}

static void ev_open_play(lv_event_t *e)
{
    (void)e;
    tap_note("Now playing");
    s_play_from = 0;              /* 从首页大卡进来的 ⇒ 返回键回首页 */
    goto_page(2);
}

/* 「城市声音集」卡片 -> SD 本地音频页 */
static void ev_open_sd(lv_event_t *e)
{
    (void)e;
    tap_note("Local audio");
    s_sd_count_dirty = true;      /* 每次从首页进来重新数一次全卡音频 */
    goto_page(5);
    ui_sd_page_refresh();
}

/* hstrip 的回调原型：void (*)(int) */
static void ev_pick_cat(int cat);
static void ev_pick_prov(int prov);

/* 点栏目胶囊 → 进第 7 页全屏列表看这一栏的台。
 * ★ 兰兰 10-03 原话：「首页按新闻就直接进入全屏的新闻电台页，
 *   只要有退回键就可以了，毕竟屏幕小，需要更大的分类页」。
 *
 * ★★ 10-04 第二十八次：前 3 颗是【自定义入口】，先分流再谈栏目。
 *   下标 0/1/2 见 k_cat_short 上面的定义。
 *   ⚠️ 分流必须做在【最前面】：下标 3..14 才是真栏目，
 *     若把 0 也当 cat 传下去，s_list_cat=0 恰好是「新闻」，
 *     点「收藏」会列出一整页新闻台 —— 静默错，编译零报错。*/
static void ev_pick_cat(int cat)
{
    if (cat < 0) return;

    if (cat == HCAT_FAV) {
        tap_note("Favorites");
        s_list_mode  = LIST_BY_FAV;
        s_list_title = "My favorites";
        s_list_back  = 0;
        list_page_refresh();
        goto_page(6);
        return;
    }
    if (cat == HCAT_HIST) {
        tap_note("History");
        s_list_mode  = LIST_BY_HIST;
        s_list_title = "Recently played";
        s_list_back  = 0;
        list_page_refresh();
        goto_page(6);
        return;
    }
    if (cat == HCAT_SD) {
        /* 卡内 = 内存卡本地音频，直接进第 5 页。*/
        tap_note("SD audio");
        s_sd_count_dirty = true;      /* 每次进来重新数一次全卡音频 */
        goto_page(5);
        ui_sd_page_refresh();
        return;
    }

    /* 「全部」不进全屏页 —— 首页那层网格本来就是全部。*/
    if (cat >= HCAT_ALL) {
        tap_note("All stations below");
        return;
    }
    s_list_mode  = LIST_BY_CAT;
    s_list_cat   = cat - HCAT_REAL;      /* ★ 减去 3，把自定义入口让出去 */
    s_list_title = xs_cat_name_en[s_list_cat];
    s_list_back  = 0;
    tap_note(k_cat_short[cat]);
    list_page_refresh();
    goto_page(6);
}

static void page_home_build(lv_obj_t *p)
{
    /* ---------- 正在播放 大卡 ---------- */
    lv_obj_t *card = tick(p, 8, 6, 248, 104, C_PANEL, 16, NULL);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, C_LINE, 0);
    lv_obj_add_event_cb(card, ev_open_play, LV_EVENT_CLICKED, NULL);

    label(card, 14, 8, F_TINY, C_AMBER, "* Now playing");
    /* ★ 台名必须用台名专用字库（st）：主字库只有 625 个界面文案字，
     *   台单 753 字，「圳涿 碚 禺 廣 華 藝 國」这些它没有 ⇒ 会出方块。
     *   16px 是 st 的中间档（gen_fonts_st.py 里旧名 F_ST18 已改指 16px）。*/
    /* ★★★ 10-05 兰兰真机反馈：「正在播放的电台字体太大了，长电台名会换行
     *   挡住下面标签」。这是 v1.36 砍字号档的必然代价：
     *   旧 F_ST18(18px) 被映射到 st24(24px)，一次放大 33%。
     *   容器宽 162px：24px 下只放得下 6 个汉字，而台单里「中央人民广播电台」
     *   这类 8 字台名一换行就压到 y=64 的来源行。
     *   修法两条一起上：
     *     ① 字号 F_ST18 → F_ST16（已在 gen_fonts_st.py 里把旧名指向 16px），
     *        162px 下放得下 10 个汉字。
     *     ② LONG_WRAP → LONG_DOT：★ 这是关键 —— 就算遇到 10 字以上的超长
     *        台名，也【只出一行加省略号】，绝不换行往下压。
     *        ★★ 别再用 WRAP：v2.0 起所有会显示运行时长文本的地方
     *        都必须是 DOT，WRAP 一律换行 ⇒ 必然溢出容器。
     *   卡内还有 y=64 来源行与 y=80 频谱，留白充足。*/
    s_lbl_station = label(card, 14, 26, F_ST18, C_TEXT, "Loading...");
    lv_obj_set_width(s_lbl_station, 162);
    lv_obj_set_height(s_lbl_station, 20);
    lv_label_set_long_mode(s_lbl_station, LV_LABEL_LONG_DOT);

    s_lbl_source = label(card, 14, 64, F_TINY, C_MUTED, "Net stream - 64 kbps");
    spectrum(card, 14, 80, 140, 18, C_GREEN);

    lv_obj_t *pb = tick(card, 186, 44, 48, 48, C_GREEN, 24, NULL);
    lv_obj_add_event_cb(pb, ev_home_play, LV_EVENT_CLICKED, NULL);
    s_playbtn_lbl = label(pb, 0, 0, F_TINY, C_ONDARK, "Pause");
    lv_obj_center(s_playbtn_lbl);

    /* ---------- 音乐库（SD 本地音频）----------
     * ★ 10-04 第二十八次：原来这卡叫「城市声音集」，那是【网页版的栏目名】，
     *   搬到这里语义是错的 —— 它指的是「本地音频」这个功能，
     *   不是某个栏目。兰兰原话「首页城市声音集标签可以暂时不用，
     *   因为本地内存卡音频种类众多」⇒ 改成中性的「音乐库」。
     *   ★ 现在「音乐库」既是首页这张卡，也是标签条里的第 3 颗 —— 两个入口
     *     指向同一个页面，符合「入口越多越好用」，不冲突。*/
    lv_obj_t *c2 = tick(p, 264, 6, 152, 48, C_PANEL, 14, NULL);
    lv_obj_set_style_border_width(c2, 1, 0);
    lv_obj_set_style_border_color(c2, C_LINE, 0);
    lv_obj_add_event_cb(c2, ev_open_sd, LV_EVENT_CLICKED, NULL);
    /* ★ 10-05 兰兰反馈「音乐库/最近听过字体太大，挡住下面的副标题」。
     *   原因同上：这两个是 3~4 字短标题，却用了 F_H2(24px)。
     *   卡片高 48px，字号 24px 时下方 28px 处的副标题（"内存卡音频→"）
     *   会被压住。改 F_MID(16px)：字高 16，6+16=22 < 28 ⇒ 留 6px 间隙。*/
    label(c2, 12, 6, F_MID, C_TEXT, "SD audio");
    s_lbl_sdinfo2 = label(c2, 12, 28, F_TINY, C_CYAN, "SD audio >");

    /* ---------- 常听（最近播过的台）----------
     * ★ 10-04 第二十八次：原来叫「收藏电台 / N 个常用台」，
     *   但收藏已经搬到标签条第 1 颗了，这张卡改成显示
     *   【最近在听】—— 兰兰「如果有历史播放最好」，
     *   而「最近在听什么」是最该在开机第一眼看到的信息。*/
    lv_obj_t *c3 = tick(p, 264, 62, 152, 48, C_PANEL, 14, NULL);
    lv_obj_set_style_border_width(c3, 1, 0);
    lv_obj_set_style_border_color(c3, C_LINE, 0);
    lv_obj_add_event_cb(c3, ev_open_hist, LV_EVENT_CLICKED, NULL);
    /* ★ 10-05 同上：「最近听过」也是 4 字短标题，24px 会压住
     *   下方 y=28 的「还没有记录」⇒ 改 F_MID(16px)。*/
    label(c3, 12, 6, F_MID, C_TEXT, "Recently played");
    s_lbl_histcnt = label(c3, 12, 28, F_TINY, C_AMBER, "Nothing yet");

    /* ---------- 栏目条（横向可滑 · 虚拟）----------
     * ★ 兰兰 10-03：「首页的栏目只有新闻，音乐，交通，但是 HTML 版的栏目
     *   有很多，分类栏应该可以向右滑动选择」。
     *   用 hstrip 虚拟条：只建 7 颗胶囊循环复用。
     *   点一个 = 进全屏页看这一栏（不在首页就地筛）。
     * ★★ 10-04 第二十八次：项数从 NET_CAT_N+1（13）改成 HCAT_ALL+1（16），
     *   前面多了 收藏 / 历史 / 音乐库 三颗（兰兰定：收藏放最优先）。*/
    label(p, 8, 116, F_TINY, C_MUTED, "Fav - History - SD | 12 categories - swipe");
    hstrip_create(&s_strip_cat, p, 8, 130, 408, 38, 48,
                  HCAT_ALL + 1, ev_pick_cat);

    /* ---------- 电台网格（双列 + 上下滑）----------
     * ★ 版面账：内容区 282 高。顶栏卡片区 y=6..110（104）、栏目条
     *   y=116..168（52）、标题行 y=172..184，剩下 y=188..276 = 88px。
     *   88 / 5 行 = 17.6px 一行 —— 14px 字塞不下。
     *   所以网格只做【3 行 = 6 台】的预览（每行 29px，舒服），
     *   要挑台点上面的栏目进全屏页（兰兰要的「屏幕小，分类页更大」）。*/
    s_lbl_list_title = label(p, 8, 172, F_TINY, C_MUTED, "Loading all stations...");

    /* ★ v1.40：网格本体搬走了，挪到 grid_pages_ensure()（文件后段）。
     *   原因是它 9 格 × 2 对象常驻约 4.2 KB，而池只剩 14.7 KB ——
     *   详见 goto_page 里那段 v1.40 注释。标题行留在建页里，它很轻。*/

    /* 两张卡的副标题（最近听的台 / 卡里几首歌）*/
    home_cards_refresh();
}

/* ============================================================
 *  v1.40：三套电台网格的「进页才建 / 离开就删」
 * ============================================================
 *  ★ 为什么单独拆出来而不是继续写在 page_*_build 里：
 *    那三个建页函数是【一次性】的（启动时全跑一遍，8 个页面对象全常驻）。
 *    要让网格离开就删，就得把建网格的动作从建页里摘出来，
 *    由 goto_page 在切页时按需调用。
 *
 *  ★ 三套网格与页号的对应（page_idx 从 0 数）：
 *      0 = 首页     3 行 × 3 列 =  9 格
 *      1 = 发现页   4 行 × 3 列 = 12 格
 *      6 = 分类页   8 行 × 3 列 = 24 格
 *
 *  ★ 销毁时必须把句柄置 NULL：网格的格子绑了 4 个事件回调
 *    （PRESSED/RELEASED/CLICKED/LONG_PRESSED），LVGL 随父容器
 *    一起删对象时会把回调摘掉，安全；但我们自己的 st_grid_t 里
 *    还留着 cell[]/nm[] 指针，置 NULL 才能保证「已删」这件事
 *    只由 s_grid_built[] 一个事实源说了算。
 *    —— 这与夜间页那条「三个定时器下一拍会往已释放句柄上写」
 *      是同一类错误（use-after-free），必须同标准要求。
 *
 *  ★★★ 为什么每套网格要套一个自己的容器（s_grid_box[]），不能直接
 *     lv_obj_clean(页面)：
 *     那个 API 会把页面下【所有】子对象删光 —— 包括首页的「正在播放」
 *     大卡、音乐库卡、最近听过卡、标题行，以及分类页的返回键。
 *     而这些对象是建页时建的、 supposed 常驻，删了就没了
 *     （首页网格重建时并不会重画那三张卡）。
 *     ⇒ 所以给网格一个专属父容器：容器铺满内容区 (0,0,CONT_W,CONT_H)
 *        且透明/不可滚/不收事件，网格在容器内沿用原来的 x/y，
 *        视觉与 v1.39 完全一致；销毁时只删这个容器。
 *        —— 这条是「删除的范围要精确到【谁建的】」，
 *          和网格句柄置 NULL 是同一条原则的两面。*/
static bool     s_grid_built[8];
static lv_obj_t *s_grid_box[8];

static void grid_pages_destroy(int page_idx)
{
    if (page_idx < 0 || page_idx >= 8 || !s_grid_built[page_idx]) return;
    s_grid_built[page_idx] = false;
    if (page_idx == 0) s_grid_home = NULL;
    else if (page_idx == 1) s_grid_find = NULL;
    else if (page_idx == 6) s_grid_list = NULL;
    /* ★ 只删自己那个容器。绝不能写 lv_obj_clean(s_pages[i]) —— 那样
     *   会连页面上的卡片、标题、返回键一起删掉（上一段注释详述）。*/
    if (s_grid_box[page_idx]) {
        lv_obj_del(s_grid_box[page_idx]);
        s_grid_box[page_idx] = NULL;
    }
    ESP_LOGI(TAG, "grid: 第 %d 页网格已删（池已还回）", page_idx);
}

/* ---- 建：三套网格。几何参数与 v1.39 完全一致，一个值都没改 ----
 *   ⚠️ 改这些 x/y/w/h 之前先想清楚：它们是 v1.39 刚按
 *   「横向账以父容器 CONT_W=424 为基准」重算过的（铁律 30），
 *   分类页尤其别改回 468 —— 那会让第三列整列画出页外。*/
static void grid_pages_ensure(int page_idx)
{
    if (page_idx < 0 || page_idx >= 8 || s_grid_built[page_idx]) return;
    /* 专属容器：铺满内容区、透明、不可滚、不吃事件。
     * 网格在它里面用【原来的 x/y】⇒ 视觉零变化。
     * ⚠️ 容器不能省 —— 删网格时只能删它，否则会把整页清空。*/
    lv_obj_t *bx = box(s_pages[page_idx], 0, 0, CONT_W, CONT_H, C_BG, 0);
    lv_obj_set_style_bg_opa(bx, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(bx, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bx, LV_OBJ_FLAG_CLICKABLE);
    s_grid_box[page_idx] = bx;

    switch (page_idx) {
    case 0:
        s_grid_home = grid_create_ex(bx, 8, 188, 408, 88, 3);
        s_grid_home->on_play   = on_grid_play;
        s_grid_home->on_fav    = on_grid_fav;
        s_grid_home->from_page = 0;
        home_grid_refresh();
        break;
    case 1:
        s_grid_find = grid_create_ex(bx, 8, 100, 408, 176, 4);
        s_grid_find->on_play   = on_grid_play;
        s_grid_find->on_fav    = on_grid_fav;
        s_grid_find->from_page = 1;
        find_grid_refresh();
        break;
    case 6:
        s_grid_list = grid_create_ex(bx, 6, 40, 412, 236, 8);
        s_grid_list->on_play   = on_grid_play;
        s_grid_list->on_fav    = on_grid_fav;
        /* ★ 返回键回第 6 页自己（它自己还有一个返回键，回首页/发现页）。
         *   兰兰要的是「要返回刚刚在发现页选择的地区栏」——
         *   而这个地区栏列表正是第 6 页，所以两层返回链是：
         *     播放页 →（本键）→ 第 6 页（还停在这一省/这一栏）→（‹ 返回）→ 发现/首页 */
        s_grid_list->from_page = 6;
        /* ★ 建完必须刷内容，否则进这一页看到的是【空网格】——
         *   原来它常驻，page_list_build 建完就是满的；
         *   改成按需建后，建完这一步没人做了。
         *   ⚠️ list_page_refresh 内部会判 s_grid_list，
         *   而这里已赋值 ⇒ 不会回调 ensure，递归安全。*/
        list_page_refresh();
        break;
    default:   /* 那几页没有网格：刚建的容器没用上，删掉别留空对象 */
        lv_obj_del(bx);
        s_grid_box[page_idx] = NULL;
        return;
    }
    s_grid_built[page_idx] = true;
    lv_mem_monitor_t m; lv_mem_monitor(&m);
    ESP_LOGI(TAG, "grid: 第 %d 页网格已建，池 free=%u B（used %u%%）",
             page_idx, (unsigned)m.free_size, (unsigned)m.used_pct);
    /* ★ 建完立刻体检：若建完就只剩不到 8 KB，说明这一页太重，
     *   下一次重绘极可能分配失败 —— 那种情况下宁可现在报出来，
     *   也不要等到渲染时崩在 lv_draw_add_task 里（那里看不出是谁）。*/
    if (m.free_size < 8192) {
        ESP_LOGE(TAG, "★★ 建完第 %d 页网格后池只剩 %u B ——"
                      "重绘时极可能 lv_malloc 返 NULL 而崩溃",
                 page_idx, (unsigned)m.free_size);
    }
}

/* ============================================================
 *  第 6 页：城市声音集（SD 卡本地音频浏览）
 * ============================================================ */
#define SD_ROWS 6

static lv_obj_t *s_sd_icon[SD_ROWS];
static lv_obj_t *s_sd_name[SD_ROWS];
static lv_obj_t *s_sd_size[SD_ROWS];
static lv_obj_t *s_sd_row[SD_ROWS];
static lv_obj_t *s_lbl_sd_path;
static lv_obj_t *s_lbl_sd_info;
static lv_obj_t *s_lbl_sd_play;      /* 「● 正在播放：xxx」一行 */

/* 按【UTF-8 字符】截断复制（不是按字节）。
 * ★ 为什么不能用 snprintf("%.*s")：那是按字节截。中文 3 字节/字，
 *   正好切在字符中间就会留下半个 UTF-8 序列 ⇒ 屏上多一个乱码字。
 *   卡里的中文长文件名很常见，所以这里老老实实按字符数截。*/
static void utf8_copy_n(char *dst, size_t dstsz, const char *src, int max_chars)
{
    int    n = 0;
    size_t o = 0;
    while (src && *src && n < max_chars) {
        unsigned char c = (unsigned char)*src;
        int len = 1;
        if      ((c & 0x80) == 0x00) len = 1;
        else if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        if (o + (size_t)len + 1 > dstsz) break;
        for (int i = 0; i < len; i++) {
            if (!src[i]) { dst[o] = '\0'; return; }
            dst[o++] = src[i];
        }
        src += len;
        n++;
    }
    dst[o] = '\0';
}

static app_sd_entry_t s_sd_ents[SD_ROWS];
static int            s_sd_cnt = 0;
static char           s_sd_path[256] = "/sdcard";
static char           s_sd_prev[256] = "";

static void sd_row_clicked(lv_event_t *e)
{
    int r = (int)(intptr_t)lv_event_get_user_data(e);
    if (r < 0 || r >= s_sd_cnt) return;
    const app_sd_entry_t *it = &s_sd_ents[r];
    tap_note(it->name);

    if (it->is_dir) {
        /* 进子目录。
         * ★ 必须给 %s 加精度上限（%.250s）：gcc 的 -Wformat-truncation 会按
         *   「s_sd_path 最大 255 + '/' + it->name 最大 259」算最坏情况，
         *   目标缓冲区开多大都可能被判「可能截断」，而 IDF 默认 -Werror → 编译不过。
         *   加了精度它就能算出上界 250+1+250=501，缓冲区 600 足够，警告消失。*/
        char next[600];
        snprintf(next, sizeof(next), "%.250s/%.250s", s_sd_path, it->name);
        strncpy(s_sd_prev, s_sd_path, sizeof(s_sd_prev) - 1);
        s_sd_prev[sizeof(s_sd_prev) - 1] = '\0';
        strncpy(s_sd_path, next, sizeof(s_sd_path) - 1);
        s_sd_path[sizeof(s_sd_path) - 1] = '\0';
        ui_sd_page_refresh();
    } else {
        /* 音频文件：交给播放器（解码 -> I2S -> 喇叭）。
         * ★ 路径拼接同样要加 %s 精度上限，理由见上面目录分支的注释。*/
        char full[600];
        snprintf(full, sizeof(full), "%.250s/%.250s", s_sd_path, it->name);

        for (int i = 0; i < SD_ROWS; i++) {
            if (s_sd_row[i]) lv_obj_set_style_bg_color(s_sd_row[i], C_PANEL, 0);
        }
        if (s_sd_row[r]) lv_obj_set_style_bg_color(s_sd_row[r], C_HILITE, 0);

        /* 记住目录：播放页的「上一个 / 下一个」要在同一目录里换曲 */
        snprintf(s_local_dir, sizeof(s_local_dir), "%.250s", s_sd_path);

        ESP_LOGI(TAG, "SD file picked: %s (%u B)", full, (unsigned)it->size);

        esp_err_t pr = app_radio_play_file(full);
        ESP_LOGI(TAG, "play_file -> %s", esp_err_to_name(pr));

        if (s_lbl_sd_play) {
            if (pr == ESP_OK) {
                char nm[96];
                utf8_copy_n(nm, sizeof(nm), it->name, 14);
                lv_label_set_text_fmt(s_lbl_sd_play, "* Now playing: %s", nm);
            } else {
                lv_label_set_text_fmt(s_lbl_sd_play, "Playback failed (%s)",
                                      esp_err_to_name(pr));
            }
        }

        /* ★ 关键：点了就【跳到播放页】。
         *   兰兰 10-03 反馈「按一下本地音频文件没有对应的播放」——
         *   上一版点了只在 SD 页那一行写个字，屏幕上什么都没有变化，
         *   所以体感就像「没反应」。现在直接把用户带到正在播放的界面。
         *   返回键回 SD 页（from=5），不是回首页。*/
        s_play_from = 5;
        player_refresh();
        goto_page(2);
    }
}

/* ============================================================
 *  台单热重载后重建 UI（10-06）
 * ============================================================
 *  串口敲 st_reload 之后调这个：把三套网格全拆掉再重建，
 *  否则界面上还是旧的台单名字和旧的台数。
 *
 *  ★ 为什么用 grid_pages_destroy + grid_pages_ensure（v1.40 建立的机制）
 *    而不是重新 ui_init()：ui_init 会重建整个界面，代价大且会闪屏；
 *    网格机制本来就是为「按需建/按需拆」设计的，正好复用。
 *
 *  ★ LVGL 线程安全：串口命令任务不是 LVGL 任务，直接调 lv_obj_del 会炸。
 *    所以这里只【置脏标记】，真正的重建交给 LVGL 自己的定时器
 *    （见 ui_tick 里的处理），这和本文件其它地方的约定一致。
 */
static bool s_stations_dirty = false;

static void st_reload_apply(void)
{
    for (int idx = 0; idx < 8; idx++) {
        if (!s_grid_built[idx]) continue;
        grid_pages_destroy(idx);
        grid_pages_ensure(idx);
    }
    /* 收藏/历史不是独立页面，是网格里的格子 —— grid_pages_ensure() 里
     * 会顺带调 home_grid_refresh() / find_grid_refresh() 重建它们，
     * 所以这里不用另外刷（v1.45 实测：app_fav/app_hist 都走 app_st_get，
     * 名字变了它们下次打开自然就是新的）。*/
    ESP_LOGI(TAG, "Station list reloaded");
}

void ui_reload_stations(void)
{
    s_stations_dirty = true;
    /* 兜底：如果当前不在任何网格页，立刻重建，别等下一次 tick */
    if (s_grid_built[0] || s_grid_built[1] || s_grid_built[6])
        st_reload_apply();
    s_stations_dirty = false;
}

void ui_sd_page_refresh(void)
{
    if (!s_lbl_sd_path) return;

    if (!app_sd_is_mounted()) {
        lv_label_set_text(s_lbl_sd_path, "No SD card");
        lv_label_set_text(s_lbl_sd_info, "Insert a FAT32 TF card and retry");
        for (int i = 0; i < SD_ROWS; i++) {
            if (s_sd_name[i]) lv_label_set_text(s_sd_name[i], "");
            if (s_sd_size[i]) lv_label_set_text(s_sd_size[i], "");
            if (s_sd_icon[i]) lv_label_set_text(s_sd_icon[i], "");
            if (s_sd_row[i])  lv_obj_set_style_bg_color(s_sd_row[i], C_PANEL, 0);
        }
        s_sd_cnt = 0;
        return;
    }

    /* 列表里插一条「.. 返回上一级」 */
    int n = app_sd_list(s_sd_path, s_sd_ents, SD_ROWS);
    if (n < 0) n = 0;
    s_sd_cnt = n;

    uint32_t mb = 0;
    const char *fs = "?";
    app_sd_get_info(&mb, &fs);

    /* 全卡音频数：只在置脏时重扫，否则用缓存（见文件上方 s_sd_count_dirty 注释）*/
    s_sd_audio_total = app_sd_audio_total(s_sd_count_dirty);
    s_sd_count_dirty = false;

    lv_label_set_text_fmt(s_lbl_sd_path, "%s", s_sd_path);
    lv_label_set_text_fmt(s_lbl_sd_info, "%s - %u MB - %d here - %d total",
                          fs, (unsigned)mb, n,
                          s_sd_audio_total < 0 ? 0 : s_sd_audio_total);

    for (int i = 0; i < SD_ROWS; i++) {
        if (i < n) {
            const app_sd_entry_t *it = &s_sd_ents[i];
            /* ★ 图标只能用 simhei 真有的符号（◎ ● ◆ ★ …）；
             *   ♪ ▣ ↻ 这类在 SimHei 和 Deng 里都没有字形，会画成空白。*/
            if (s_sd_icon[i]) lv_label_set_text(s_sd_icon[i], it->is_dir ? "◎" : "●");
            if (s_sd_name[i]) {
                /* 按字符（不是按字节）截断：中文长文件名很常见，
                 * 按字节截会把一个字切成半个 UTF-8 序列，屏上就多个乱码字 */
                char buf[96];
                utf8_copy_n(buf, sizeof(buf), it->name, 16);
                lv_label_set_text(s_sd_name[i], buf);
            }
            if (s_sd_size[i]) {
                if (it->is_dir) lv_label_set_text(s_sd_size[i], "Folder >");
                else            lv_label_set_text_fmt(s_sd_size[i], "%u KB",
                                                      (unsigned)(it->size / 1024));
            }
            if (s_sd_row[i]) lv_obj_clear_flag(s_sd_row[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            if (s_sd_name[i]) lv_label_set_text(s_sd_name[i], "");
            if (s_sd_size[i]) lv_label_set_text(s_sd_size[i], "");
            if (s_sd_icon[i]) lv_label_set_text(s_sd_icon[i], "");
        }
        if (s_sd_row[i]) lv_obj_set_style_bg_color(s_sd_row[i], C_PANEL, 0);
    }

    /* 播放状态回填：从其它页切回 SD 页时也要显示对 */
    if (s_lbl_sd_play) {
        const char *np = app_radio_now_playing();
        if (np && np[0]) {
            char nm[96];
            utf8_copy_n(nm, sizeof(nm), np, 14);
            lv_label_set_text_fmt(s_lbl_sd_play, "* Now playing: %s", nm);
        } else {
            lv_label_set_text(s_lbl_sd_play, "");
        }
    }

    ESP_LOGI(TAG, "SD page: %s -> %d entries", s_sd_path, n);
}

static void ev_sd_up(lv_event_t *e)
{
    (void)e;
    tap_note(s_sd_prev[0] ? "Up one level" : "Home");
    if (s_sd_prev[0]) {
        strncpy(s_sd_path, s_sd_prev, sizeof(s_sd_path) - 1);
        s_sd_path[sizeof(s_sd_path) - 1] = '\0';
        s_sd_prev[0] = '\0';
        ui_sd_page_refresh();
    } else {
        goto_page(0);
    }
}

static void page_sd_build(lv_obj_t *p)
{
    /* 标题行（固定文案，用常规大字库即可）*/
    label(p, 8, 8, F_H2, C_TEXT, "Local audio");
    label(p, 132, 11, F_SD, C_CYAN, "SD audio");

    lv_obj_t *back = tick(p, 328, 6, 88, 28, C_HILITE, 14, NULL);
    lv_obj_add_event_cb(back, ev_sd_up, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bt = label(back, 0, 0, F_SD, C_TEXT, "< Back");
    lv_obj_center(bt);

    /* 路径 / 卡信息 / 播放状态
     * ★ 这三行都必须用 F_SD（大字符集）：它们会显示【卡里的名字】，
     *   小字库遇到源码里没出现过的汉字只能画方块。
     *   行距按 14px 字高排（每行 16px），列表从 86 开始 —— 最后一行到 281，不出屏。*/
    s_lbl_sd_path = label(p, 8, 36, F_SD, C_GREEN, "/sdcard");
    s_lbl_sd_info = label(p, 8, 52, F_SD, C_MUTED, "Reading...");
    s_lbl_sd_play = label(p, 8, 68, F_SD, C_HILITE, "");

    /* 6 行列表 */
    for (int i = 0; i < SD_ROWS; i++) {
        int y = 86 + i * 33;
        lv_obj_t *r = tick(p, 8, y, 408, 30, C_PANEL, 8, NULL);
        lv_obj_add_event_cb(r, sd_row_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_sd_row[i] = r;

        s_sd_icon[i] = label(r, 10, 8, F_SD, C_CYAN, "");
        s_sd_name[i] = label(r, 32, 8, F_SD, C_TEXT, "");
        s_sd_size[i] = label(r, 0, 9, F_SD, C_MUTED, "");
        lv_obj_set_width(s_sd_size[i], 396);
        lv_obj_set_style_text_align(s_sd_size[i], LV_TEXT_ALIGN_RIGHT, 0);
    }
}

/* ============================================================
 *  页面二：发现（省份筛选 + 双列网格）
 * ============================================================
 *  10-03 第八次：这页原来在台名里做子串匹配找城市名（一个省配一组关键词），
 *    只能覆盖 7 个省，而且台单一换就得重配关键词。
 *  第九次：台单里每条台本来就带【省份字段】（_gen_stations.py 照抄网页版的
 *    c 字段），所以直接按字段筛 —— 31 个省全都能选，一个不漏、一个不错。
 *
 *  ★ 兰兰 10-03 原话：「发现页，地区无法向右边滑动选择，造成只有几个地区
 *    可以选，还是和首页一样最好双列显示，直观更容易」
 *    → 省份条改成横滑（32 个胶囊：全部 + 31 省），结果区是双列网格。
 *    点省份也是进第 7 页全屏看（跟首页栏目一致的操作逻辑）。
 */

static lv_obj_t *s_lbl_find_title;
static int       s_fr_national = 0;   /* 全国台几个 */

/* 点省份胶囊 → 进第 7 页全屏看这一省 */
static void ev_pick_prov(int prov)
{
    if (prov < 0 || prov >= NET_PROV_N) return;
    s_list_mode  = LIST_BY_PROV;
    s_list_prov  = prov;
    s_list_title = xs_prov_long_en[prov];
    s_list_back  = 1;
    tap_note(xs_prov_long_en[prov]);
    list_page_refresh();
    goto_page(6);
}

/* 发现页那层网格：默认显示【央广 / 国广 / 无省份的全国台】。
 * ★ 为什么不给「全部」：那跟首页那一层完全重复，两个页面看一样的东西
 *   就等于白占一屏。发现页的价值是「按地方找」，
 *   而这层先给覆盖面最广的那批当入口，想按省就点上面的省份条。
 *   两遍扫描：第一遍收全国台，第二遍才收地方台（保证全国台一定在最前）。*/
static void find_grid_refresh(void)
{
    static int pool[GRID_MAX];
    int n = 0;
    int total = app_radio_station_count();
    for (int i = 0; i < total && n < GRID_MAX; i++) {
        if (app_st_get(i)->prov == NET_PROV_NONE) pool[n++] = i;
    }
    s_fr_national = n;
    for (int i = 0; i < total && n < GRID_MAX; i++) {
        if (app_st_get(i)->prov != NET_PROV_NONE) pool[n++] = i;
    }
    /* ★ v1.40：网格按需建 ⇒ 可能为 NULL（页面刚建时先于 ensure 调用）。
     *   同 home_grid_refresh，判空 + 兜底建。*/
    if (!s_grid_find) {
        ESP_LOGW(TAG, "find: 网格还没建 —— 先建再填台");
        grid_pages_ensure(1);
        if (!s_grid_find) return;
    }
    grid_set(s_grid_find, pool, n);
    if (s_lbl_find_title) {
        lv_label_set_text_fmt(s_lbl_find_title, "%d nationwide - %d total - scroll",
                              s_fr_national, n);
    }
    ESP_LOGI(TAG, "find grid: national=%d total=%d", s_fr_national, n);
}

/* ============================================================
 *  发现页右上角「海外」入口 —— 二级
 * ============================================================
 *  ★ 兰兰 10-03 第十二次：
 *   「发现页海外标签点中以后所有电台都集中在一页太多了，
 *     我的自用版HTML或者exe的海外版有详细分类的，
 *     北美欧洲澳洲香港台湾都分开的」
 *
 *   ⇒ 网页版（XianHai.html）的地区就是这 11 个：
 *     中国台湾 / 中国香港 / 中国澳门 / 其他华语 / 北美 / 欧洲 /
 *     日韩 / 新马 / 东南亚 / 大洋洲
 *     固件台单里【已经是分开的】（g_prov_name 的 31~41 项），
 *     所以只要把这个子分类列表摆出来就是网页版那个结构。
 *
 *   做法：第 7 页（全屏列表）的网格换成「先选地区、点了再看台」——
 *     用现成的 st_grid_t 画 11 个地区块，每块显示「地区名 + 台数」。
 *     不新建页面、不多占 lv_obj：复用第 7 页那套虚拟网格。
 */

/* 11 个海外地区在 g_prov_name 里的下标（= 31..41，与台单生成一致）*/
#define OVS_PROV_FIRST NET_PROV_CN_N     /* 31 */

static int s_ovs_pick = -1;              /* 已选的海外地区下标；-1 = 正在选地区 */

static void ovs_slot_clicked(int slot, int idx);   /* 下面定义，这里先引用 */

/* 在第 7 页画「海外地区块」：每块 = 地区名 + 台数 */
static void ovs_grid_refresh(void)
{
    static int pool[GRID_MAX];
    int n = 0;
    int total = app_radio_station_count();

    for (int pv = OVS_PROV_FIRST; pv < NET_PROV_N && n < GRID_MAX; pv++) {
        int cnt = 0;
        for (int i = 0; i < total; i++) {
            if (app_st_get(i)->prov == pv) cnt++;
        }
        if (cnt > 0) pool[n++] = pv;      /* pool 存的是【地区下标】，不是台下标 */
    }
    s_list_total = n;
    if (s_lbl_list_title2) {
        lv_label_set_text_fmt(s_lbl_list_title2,
                              "World - %d regions - tap one", n);
    }
    /* ★ v1.40：网格改成按需建，所以这里必须判空。
     *   ovs_grid_refresh() 由「点海外按钮」触发，而那个按钮在发现页上 ——
     *   若此时【还没进过第 6 页】，s_grid_list 就是 NULL（以前它常驻，
     *   永远不会为空，所以旧代码没判）。不加判空就是往 NULL 写字段。*/
    if (!s_grid_list) {
        ESP_LOGW(TAG, "ovs: 第 6 页网格还没建（未进过该页）—— 先建再填地区");
        grid_pages_ensure(6);
        if (!s_grid_list) return;
    }
    s_grid_list->mode = 1;               /* 这一层画的是地区块，不是台 */
    s_grid_list->on_region = ovs_slot_clicked;
    grid_set(s_grid_list, pool, n);
    ESP_LOGI(TAG, "ovs regions: %d", n);
}

/* 网格里点一块：按当前模式分派（海外地区 vs 台名）*/
/* ★★★★ 10-05 真机 bug（兰兰：「点最后一个海外地区，跳到国内电台了」）：
     *   原来这里读的是【全局 s_pool[idx】——那是首页 s_grid_home 的池子。
     *   而 ovs_grid_refresh 把海外地区填进的是它的【局部 pool[]】，
     *   grid_set 已经把数据复制进 s_grid_list->pool[]（见 st_grid_t.pool）。
     *   ⇒ 点第N 格时读的是首页缓存的第 N 条 ⇒ 「北美」点到「欧洲」、
     *     甚至点到国内的台，完全串位。
     *   ⇒ 改成读【自己那个网格】的池子：s_grid_list->pool[idx]。
     *   ★ 这类 bug 的教训：同一个「池」概念在代码里有两个实例时，
     *     回调必须明确用哪一个，不能凭函数名里的 pool 想当然。*/
static void ovs_slot_clicked(int slot, int idx)
{
    (void)slot;
    /* ★★ 10-05 修正两次才改对，这里必须钉死：
     *   `idx` 【已经是 prov 索引】（grid_show_cell 里 `g->idx[slot] = station_idx`，
     *   而 mode==1 时 pool[] 装的就是 prov，见 ovs_grid_refresh）。
     *   ⇒ 不要再 `pool[idx]` 反查一次，那会取到别的台的下标。
     *   ⇒ 也【不能拿 s_grid_list->total 做上界】：total 是「地区个数」（≤11），
     *   而海外 prov 从 OVS_PROV_FIRST(=31) 起算，31 >= 11 恒成立 ⇒ 点了全被拦掉，
     *   表现为「点海外地区没反应」。上界只能是 NET_PROV_N。*/
    if (idx < OVS_PROV_FIRST || idx >= NET_PROV_N) return;
    int pv = idx;
    s_ovs_pick = pv;
    s_list_mode  = LIST_BY_PROV;
    s_list_prov  = pv;
    s_list_title = xs_prov_long_en[pv];
    s_list_back  = 6;                 /* 返回键回「海外地区选择」这一层 */
    tap_note(xs_prov_long_en[pv]);
    list_page_refresh();
}

static void ev_pick_overseas(lv_event_t *e)
{
    (void)e;
    s_ovs_pick   = -1;
    s_list_back  = 1;                 /* 返回键回发现页 */
    list_page_back_to_ovs();
}

/* ★ 点「海外」→ 进第 7 页但显示地区块（不是台）*/
static void list_page_back_to_ovs(void)
{
    s_list_mode  = LIST_BY_OVS_REGION;
    s_list_title = "World";
    goto_page(6);
    ovs_grid_refresh();
}

static void page_find_build(lv_obj_t *p)
{
    /* ---------- 省份条（横向可滑 · 虚拟）----------
     * ★ 42 个地区每个一颗实体胶囊 = 84 个 lv_obj ≈ 34 KB，
     *   而 LVGL 池只有 103 KB（同屏还有两套网格）⇒ 必然抽干。
     *   所以是 hstrip：只建 7 颗，滑到哪填到哪。*/
    label(p, 8, 6, F_TINY, C_MUTED, "Regions - swipe");

    /* 海外直达（右上角）。琥珀色与地区条里那些海外胶囊同色，一眼认得出。
     *   ⚠️ tick() 的第 8 参只是「日志 tag」，不是回调 —— 靠它做点击是接不上的，
     *   必须自己 lv_obj_add_event_cb。这个坑我差点又踩。*/
    lv_obj_t *ovs = tick(p, 330, 2, 86, 22, lv_color_hex(0x3A2E12), 11, "Intl");
    lv_obj_add_event_cb(ovs, ev_pick_overseas, LV_EVENT_CLICKED, NULL);
    /* 台数实时数一遍，不写死 —— 台单一改，硬编码的 194 就变成谎话 */
    int ovs_n = 0, tot = app_radio_station_count();
    for (int i = 0; i < tot; i++) {
        unsigned char pv = app_st_get(i)->prov;
        if (pv != NET_PROV_NONE && pv >= NET_PROV_CN_N) ovs_n++;
    }
    lv_obj_t *ovs_t = label(ovs, 0, 0, F_TINY, C_AMBER, "");
    lv_label_set_text_fmt(ovs_t, "Intl %d", ovs_n);
    lv_obj_center(ovs_t);
    ESP_LOGI(TAG, "find page: overseas stations = %d", ovs_n);

    hstrip_create(&s_strip_prov, p, 8, 20, 408, 40, 48,
                  NET_PROV_N, ev_pick_prov);

    /* ---------- 提示行 ---------- */
    label(p, 8, 66, F_TINY, C_MUTED,
          "Swipe for a region, tap it to list - long-press to favorite");

    /* ---------- 电台网格（双列 + 上下滑）---------- */
    s_lbl_find_title = label(p, 8, 84, F_TINY, C_MUTED, "Loading...");
    /* ★ v1.40：网格本体搬走，改由 grid_pages_ensure(1) 建。详见那段注释。*/
}

/* ============================================================
 *  第 7 页：全屏电台列表（首页栏目 / 发现页地区 / 收藏 共用）
 * ============================================================
 *  ★ 兰兰 10-03：「首页按新闻就直接进入全屏的新闻电台页，只要有退回键
 *    就可以了，毕竟屏幕小，需要更大的分类页」「最好都是电台页都是双列的，
 *    显示只要电台名称就足够了」
 *  → 一屏 7 行 × 2 列 = 14 个台名，只有标题 + 返回键，没有别的干扰。
 */

static lv_obj_t *s_btn_back;

static void ev_list_back(lv_event_t *e)
{
    (void)e;
    tap_note("Back");
    /* ★ 海外二级：s_list_back==6 表示「上一层也是第 7 页」（地区块列表），
     *   所以不是简单 goto_page —— 得先把这一层重画回地区块，
     *   否则会看到【同一个页面里内容没变】。*/
    if (s_list_back == 6) {
        list_page_back_to_ovs();
        return;
    }
    goto_page(s_list_back);
}

/* 收藏：把 NVS 里的台名解析成下标 */
static void list_page_refresh(void)
{
    static int pool[GRID_MAX];
    int n = 0;
    const char *title = s_list_title;

    if (s_list_mode == LIST_BY_CAT) {
        if (s_list_cat < 0 || s_list_cat >= NET_CAT_N) s_list_cat = 0;
        title = xs_cat_name_en[s_list_cat];
        n = collect_stations(pool, GRID_MAX, s_list_cat, -1);
    } else if (s_list_mode == LIST_BY_PROV) {
        if (s_list_prov < 0 || s_list_prov >= NET_PROV_N) s_list_prov = 0;
        title = xs_prov_long_en[s_list_prov];
        n = collect_stations(pool, GRID_MAX, -1, s_list_prov);
    } else if (s_list_mode == LIST_BY_ALL_OVS) {
        /* ★ 把地区 >= 31（中国台湾起）的台一次收齐。
        *   判据用 prov 下标而不是台名里找「台/港/澳」字 ——
        *   台名是数据不是结构，改一次数据就要重配关键词（第八次踩过这个坑）。*/
        int total = app_radio_station_count();
        for (int i = 0; i < total && n < GRID_MAX; i++) {
            unsigned char pv = app_st_get(i)->prov;
            if (pv != NET_PROV_NONE && pv >= NET_PROV_CN_N) pool[n++] = i;
        }
    } else if (s_list_mode == LIST_BY_OVS_REGION) {
        /* 这一层画的是地区块，由 ovs_grid_refresh() 处理 */
        n = 0;
    } else if (s_list_mode == LIST_BY_HIST) {
        /* 播放历史：app_hist_top() 已经顺手做了「解析不出来的跳过」
         *   和「同名去重」，这里直接用。*/
        title = "Recently played";
        n = app_hist_top(pool, GRID_MAX);
    } else {
        title = "My favorites";
        n = app_fav_list(pool, GRID_MAX);
    }
    s_list_total = n;
    ESP_LOGI(TAG, "list title: %s (%d)", title ? title : "?", n);

    /* ★ v1.40：网格按需建 ⇒ 这里可能还没建。
     *   典型触发：从播放页按返回直接跳第 6 页，或点标签条进「新闻」
     *   而机器还没进过第 6 页。grid_set 里会写 g->idx[] / g->total，
     *   s_grid_list 为 NULL 就是往地址 0x0.. 写。*/
    if (!s_grid_list) {
        ESP_LOGW(TAG, "list: grid not built yet");
        grid_pages_ensure(6);
        if (!s_grid_list) return;
    }

    /* ★ 回到台名模式（上一行可能还停在「地区块模式」）——
     *   grid 的 mode 是常驻字段，不清的话这一页会把【地区下标】
     *   当【台下标】显示，越界读 g_stations[]。*/
    s_grid_list->mode = 0;
    s_grid_list->on_region = NULL;

    if (s_lbl_list_title2) {
        if (n == 0) {
            lv_label_set_text_fmt(s_lbl_list_title2, "%s - empty", title);
        } else {
            lv_label_set_text_fmt(s_lbl_list_title2, "%s - %d - scroll", title, n);
        }
    }
    grid_set(s_grid_list, pool, n);
}

/* 首页收藏卡 → 全屏收藏页
 * ★ 10-04 第二十八次：首页那张卡已改成「最近听过」，
 *   收藏的首页入口搬到了标签条第 1 颗（走 ev_pick_cat 的 HCAT_FAV 分支）。
 *   这个函数保留：全屏收藏页内部、以及将来别处要跳收藏页时还能用。*/
static void ev_open_fav(lv_event_t *e)
{
    (void)e;
    tap_note("Favorites");
    s_list_mode  = LIST_BY_FAV;
    s_list_title = "My favorites";
    s_list_back  = 0;
    list_page_refresh();
    goto_page(6);
}

/* 首页「最近听过」卡 → 全屏历史页（10-04 第二十八次新增）*/
static void ev_open_hist(lv_event_t *e)
{
    (void)e;
    tap_note("Recently played");
    s_list_mode  = LIST_BY_HIST;
    s_list_title = "Recently played";
    s_list_back  = 0;
    list_page_refresh();
    goto_page(6);
}

/* ★★ 首页那两张小卡的下行文字。单独抽出来，因为有三处要刷：
 *   ① 建首页时  ② 切台后（历史变了）  ③ 插卡/拔卡后（音频数变了）
 *   以前收藏数那行是散在 on_grid_fav 和首页建页里各写一遍的 ——
 *   两处写法迟早会不一致（和「顶栏时钟死 label」同一个毛病）。*/
static void home_cards_refresh(void)
{
    if (s_lbl_histcnt) {
        int top[1];
        int n = app_hist_top(top, 1);
        if (n > 0) {
            /* 显示【台名】而不是「N 个」：开机第一眼想知道的是「在听什么」，
             * 不是「有几条记录」。台名最长 14 字，11px 下约 150 px，
             * 卡片可用宽 152-24 = 128 px ⇒ 按字符截到 9 个字。*/
            char nm[64];
            utf8_copy_n(nm, sizeof(nm), app_st_get(top[0])->name, 9);
            lv_label_set_text_fmt(s_lbl_histcnt, "%s", nm);
        } else {
            lv_label_set_text(s_lbl_histcnt, "Nothing yet");
        }
    }
    if (s_lbl_sdinfo2) {
        if (app_sd_is_mounted()) {
            /* ★ 必须传 rescan 参数（app_sd_audio_total(bool)）——
             *   写成 app_sd_audio_total() 会被 gcc 拦（too few arguments）。
             * ★ 这里传 false：首页这张卡每秒可能被刷到（切台时会），
             *   传 true 就是每刷一次递归 stat 几千条目录项。
             *   真正需要重扫的地方是 SD 页切目录（那边已有 s_sd_count_dirty）。*/
            int cnt = app_sd_audio_total(false);
            if (cnt > 0) {
                lv_label_set_text_fmt(s_lbl_sdinfo2, "%d items >", cnt);
            } else {
                lv_label_set_text(s_lbl_sdinfo2, "No audio >");
            }
        } else {
            lv_label_set_text(s_lbl_sdinfo2, "No SD card");
        }
    }
}

static void page_list_build(lv_obj_t *p)
{
    /* 标题行 + 返回键 */
    s_btn_back = tick(p, 6, 4, 62, 30, C_HILITE, 15, NULL);
    lv_obj_add_event_cb(s_btn_back, ev_list_back, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bt = label(s_btn_back, 0, 0, F_BODY, C_TEXT, "< Back");
    lv_obj_center(bt);

    /* ★ 10-05 兰兰反馈「地区列表的标题字体太大，长地区名，上下滑的滑会换行
     *   挡住电台名」。
     *   根因同上：F_H2(24px)。但这里【已经】设了 LONG_DOT，理论上不该换行——
     *   实测换行说明两点之一：① 24px 下 LVGL 判定长文本的时机与 12px 不同；
     *   ② 更要紧的是 y=10 处 24px 字高 24，已占掉到 y=34，
     *   而网格从 y=40 开始，只剩 6px；字一换行第二行就压到网格第一行。
     *   修法：F_H2(24px) → F_MID(16px)。16px 下字高 16，10~26，
     *   网格 40 起 ⇒ 中间留 14px 安全带。长地区名仍走 LONG_DOT 出省略号。*/
    s_lbl_list_title2 = label(p, 76, 12, F_MID, C_TEXT, "Stations");
    lv_obj_set_width(s_lbl_list_title2, 340);
    lv_label_set_long_mode(s_lbl_list_title2, LV_LABEL_LONG_DOT);

    /* ---- 网格：y=40 起，高 236 → ★ 10-05 三列后 8 行 × 29px = 24 台/屏
     *   （原来双列 6 行 = 12 台。行高 236/8 = 29 px，扣掉格间距 4 是 25 px，
     *   放 12px 字 + 上下各 6 px 余量，够；台名字体走 F_ST12。
     *   ⚠️ 若以后把 vis_rows 提到 9，行高掉到 26 px 以下就会显得挤，
     *      届时应先把容器高度 h 从 236 加大，而不是压字号。）
     *
     * ★★★★★★ 10-05 v1.39 修【最右一列被挡字】
     *   兰兰原话：「三列最右一列基本都会挡住几个字」。
     *
     *   根因不是台名太长，是**容器比它所在的页面还宽**：
     *     本页父对象 p 的宽 = CONT_W = LCD_H_RES(480) − RAIL_W(56) = **424**
     *     （左边那条 56px 竖导航栏就占在这儿），
     *     而这里给网格 x=6、w=468 ⇒ 右缘 6+468 = **474**，比页面宽出 50px。
     *     cell_w = (468−6)/3 = 154，三列落在 0~154 / 160~314 / 320~474；
     *     第三列有 50px（424~474）落在页面之外，看不见。
     *     8 字台名 12px 实测最宽 80px，居中在 154 里 ⇒ 占 357~437，
     *     其中 424~437 那 13px 被切掉 ⇒ **每行的后 1~2 个字消失**。
     *     兰兰说「基本都会挡住几个字」，就是这个 —— 位置固定 ⇒ 是宽度算错，
     *     不是随机截断。
     *
     *   修法：横向一律以【父容器宽】为基准，不用屏幕宽。
     *     可用 = CONT_W(424) − x(6) − 右边距(6) = **412**
     *     cell_w = (412 − 6×2) / 3 = **133**
     *     三列占 133×3 + 6×2 = 411 ≤ 412 ✓，列右缘 133 / 272 / 411 全在页内
     *     8 字台名 80px 居中在 133 里 ⇒ 占 302~382，右侧余 29px ✓
     *
     *   ★★★ 教训（与 v1.38「上界用错量纲」同源，必须一起记）：
     *     **横向坐标以父容器宽为基准，纵向以父容器高为基准；
     *       拿屏幕宽（480）去算放在 424 宽父级里的坐标，必然溢出。**
     *     屏幕 480 里有 56px 是导航栏，页面里没有。
     *   ★ 顺带把上面 s_lbl_list_title2 的宽度也一起核过：
     *     x=76 w=340 ⇒ 右缘 416 ≤ 424 ✓（这行原来是对的，没动）。*/
    /* ★ v1.40：网格本体搬走，改由 grid_pages_ensure(6) 建。详见那段注释。*/

    /* 底部提示（收藏页空时这行有用）*/
    label(p, 6, 266, F_TINY, C_MUTED,
          "Tap = play. Long-press = favorite. Back = previous page");
}

/* ============================================================
 *  页面三：本地播放（进度环 + 曲名 + 传输控制）
 * ============================================================
 *  10-03 第七次改造。原来这页是「假 FM 调谐盘」：环上写死 105.8 MHz，
 *  两个键只弹一行日志，跟真正在放的东西没有任何关系 ——
 *  所以兰兰点完卡里的 mp3 会觉得「没有播放界面」。
 *  现在改成真界面：环 = 进度、环心 = 曲名与时间、右键 = 换曲与暂停。
 *  以后接上网络电台，同一页换个数据源即可。*/
static lv_obj_t *s_lbl_vol;

static void ev_vol_changed(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    int v = (int)lv_slider_get_value(sl);
    app_audio_set_volume(v);
    if (s_lbl_vol) lv_label_set_text_fmt(s_lbl_vol, "Vol %d%%", v);
}

/* mm:ss */
static void fmt_mmss(int sec, char *out, size_t n)
{
    if (sec < 0) sec = 0;
    snprintf(out, n, "%02d:%02d", sec / 60, sec % 60);
}

/* 把播放器的真实状态刷到界面上。
 * 由 1 秒定时器在「第 3 页可见时」调用；点文件 / 换台 / 暂停时也手动调一次。
 *
 * ★ 两种音源在同一页里长得不一样（10-03 第八次）：
 *   本地文件 —— 有总时长 ⇒ 底部细进度条显示进度，时长行 "12:03 / 45:10"
 *   网络直播 —— MP3 直播流没有总时长 ⇒ 进度条退化成底线，
 *               时长行显示 "LIVE 03:21"（已听多久）。
 * ★ 10-03 第十一次：原来那个 200x200 的进度环删了（兰兰「圆圈在，频谱就展不开」），
 *   换成底部一条 6 px 的细条，省下的空间全给频谱。*/
static void player_refresh(void)
{
    const char *name = app_radio_now_playing();
    const char *err  = app_radio_last_error();
    bool playing     = app_radio_is_playing();

    if (!playing || name == NULL || name[0] == '\0') {
        if (s_prog_play)     lv_obj_set_width(s_prog_play, 0);
        if (s_np_name)      lv_label_set_text(s_np_name, "Not playing");
        if (s_np_time)      lv_label_set_text(s_np_time, "--:--");
        if (s_np_hint)      lv_label_set_text(s_np_hint, "Tap a station on Home >");
        if (s_lbl_playname) lv_label_set_text(s_lbl_playname, "XianDial");
        if (s_np_meta)      lv_label_set_text(s_np_meta,
                                              "Internet radio / Local audio\\nHome or SD audio");
        if (s_np_state)     lv_label_set_text(s_np_state,
                                              (err && err[0]) ? err : "Idle");
        if (s_lbl_playbtn)  lv_label_set_text(s_lbl_playbtn, "Playing");
        if (s_lbl_favbtn) {
            /* ★★ 这里原来写 "\2606 收藏"，屏幕上显示的是「6 收藏」——
             *   C 的八进制转义 \ddd 最多吃 3 位数字，\2606 被解析成
             *   \260(=U+00B0，非法 UTF-8) + 字面字符 '6'，那个 6 就漏出来了。
             *   U+2606 的八进制是 2606（4 位），八进制压根表达不了
             *   ⇒ 只能用 \uXXXX / \xXX / 或（本工程 .c 是 UTF-8）直接写字符。
             *   见 firmware/_oct_escape_check.py（门禁，能自检）。*/
            lv_label_set_text(s_lbl_favbtn, "☆ Fav");
            lv_obj_set_style_text_color(s_lbl_favbtn, lv_color_hex(0x3A424C), 0);
        }
        spec_refresh();          /* 停播时频谱也要塌回底线 */
        return;
    }

    bool live = app_radio_is_stream();
    int  el   = app_radio_elapsed_s();
    int  tot  = app_radio_total_s();
    bool pz   = app_radio_is_paused();
    int  kHz  = app_radio_stream_rate();
    int  ch   = app_radio_stream_channel();

    char es[12], ts[12], line[80];
    fmt_mmss(el, es, sizeof(es));

    if (live) {
        /* 直播没有总时长 ⇒ 进度条就是一条底线（宽度 0），
           声音的起伏由上面那排真频谱表达。
           ★ 直播不能 seek：ev_seek_press 里 app_radio_can_seek() 为 false 会直接
             提示「直播不能拖进度」并返回，不会进入拖动态。*/
        if (s_prog_play && !s_seek_drag) lv_obj_set_width(s_prog_play, 0);
        /* 手柄也藏起来：不能拖的东西不该给一个「抓得住」的暗示 */
        if (s_seek_knob) lv_obj_set_style_bg_opa(s_seek_knob, LV_OPA_TRANSP, 0);
        /*★★ 10-04：直播时间行后面挂上「延迟 Ns」。
         *  兰兰两次报「听着像延迟」，但两次原因完全不同：
         *    修复前是重播老片（文件名是几分钟前的），
         *    现在是源本身 + 十几秒的正常延迟。
         *  光听分不出来，屏上打出数字就不用猜、也不用去对比电视。
         *  只有 HLS 直播能算（分片文件名里有 unix 时间戳）；算不出就照旧。*/
        long lag = app_radio_hls_lag_s();
        if (s_np_time) {
            if (lag > 0) lv_label_set_text_fmt(s_np_time, "LIVE  %s  delay%lds", es, lag);
            else         lv_label_set_text_fmt(s_np_time, "LIVE  %s", es);
        }
    } else {
        if (tot > 0) fmt_mmss(tot, ts, sizeof(ts));
        else         snprintf(ts, sizeof(ts), "--:--");
        /* ★ 正在拖动时不要抢：让前景宽度与时间行跟着手指走，
         *   否则 player_refresh 每秒刷一次会把拖动预览「拽回去」。*/
        if (s_prog_play && !s_seek_drag) {
            int w = 0;
            if (tot > 0) {
                int64_t d = (int64_t)el * s_seek_bar_w / tot;
                if (d > s_seek_bar_w) d = s_seek_bar_w;
                if (d < 0)   d = 0;
                w = (int)d;
            }
            lv_obj_set_width(s_prog_play, w);
        }
        /* ★★ 第二十一次：手柄【常显】并跟着播放进度走。
         *   这是兰兰说的「拖动条可以设置个拖动手柄，这样或许有效」——
         *   之前把手柄设成 LV_OPA_TRANSP，等于「能拖的东西看不见」，
         *   手感上根本不知道有条能拖。*/
        if (s_seek_knob && !s_seek_drag) {
            int d = 0;
            if (tot > 0) {
                int64_t k = (int64_t)el * s_seek_bar_w / tot;
                if (k < 0) k = 0;
                if (k > s_seek_bar_w) k = s_seek_bar_w;
                d = (int)k;
            }
            seek_knob_show(SEEK_BAR_X + d, app_radio_can_seek());
        }
        if (s_np_time && !s_seek_drag) {
            lv_label_set_text_fmt(s_np_time, "%s / %s", es, ts);
        }
    }

    /* 进度条下面那行提示：让兰兰一眼知道现在能不能拖
     *   （电平表撤掉后，这一行是唯一告诉他「条能拖」的界面信息）*/
    if (s_lbl_level && !s_seek_drag) {
        if (s_seek_drag)              lv_label_set_text(s_lbl_level, "Release to seek");
        else if (live)                 lv_label_set_text(s_lbl_level, "Live stream - progress is read-only");
        else if (tot > 0)              lv_label_set_text(s_lbl_level, "Drag the bar to seek");
        else                          lv_label_set_text(s_lbl_level, "No duration - use the 4 keys");
    }

    /* 曲名：按【字符】截断，按字节截会把汉字切半个（见 utf8_copy_n 注释）*/
    char nm[80];
    utf8_copy_n(nm, sizeof(nm), name, 22);
    if (s_np_name) lv_label_set_text(s_np_name, nm);

    if (s_np_hint) {
        if (pz)           snprintf(line, sizeof(line), "Paused");
        else if (live)    snprintf(line, sizeof(line), "Internet live - 64 kbps");
        else if (kHz > 0) snprintf(line, sizeof(line), "%d.%01d kHz · %s",
                                   kHz / 1000, (kHz % 1000) / 100,
                                   ch == 1 ? "Mono" : "Stereo");
        else              snprintf(line, sizeof(line), "Parsing format...");
        lv_label_set_text(s_np_hint, line);
    }

    /* 右卡：名称 + 来源 + 状态（比环心多一行信息）*/
    char nm2[96];
    utf8_copy_n(nm2, sizeof(nm2), name, 16);
    if (s_lbl_playname) lv_label_set_text(s_lbl_playname, nm2);

    if (s_np_meta) {
        if (live) {
            int cur = app_radio_station_current();
            const char *cat = (cur >= 0)
                              ? xs_cat_name_en[app_st_get(cur)->cat % NET_CAT_N]
                              : "Net stream";
            /* ★ 10-05 第二行文案缩短：原来「交通台 · 第 12/1246 台」14 字符 ≈ 168px
     *   > 框宽 140px ⇒ 第二行自己又换行 ⇒ 三行压到状态行（见上方注释）。
     *   改成「交通台 12/1246」9 字符 ≈ 108px，放得下。
     *   台单现在是 1246 台，五位数，改成只显示末位也不好看 ⇒ 保留全量，
     *   靠缩短栏目名那一段腾空间。*/
            lv_label_set_text_fmt(s_np_meta, "Internet radio - live\\n%s %d/%d",
                                  cat, cur + 1, app_radio_station_count());
        } else if (kHz > 0) {
            /* 同理：「128.0 kHz · 16 bit · 立体声」17 字符会换行
             * ⇒ 「128.0kHz 立体声」11 字符 ≈ 132px，放得下。*/
            lv_label_set_text_fmt(s_np_meta, "Local audio - SD card\\n%d.%01dkHz %s",
                                  kHz / 1000, (kHz % 1000) / 100,
                                  ch == 1 ? "Mono" : "Stereo");
        } else {
            lv_label_set_text(s_np_meta, "Local audio - SD card\\nParsing format...");
        }
    }
    if (s_np_state) {
        if (err && err[0]) lv_label_set_text_fmt(s_np_state, "× %s", err);
        else if (pz)       lv_label_set_text(s_np_state, "|| Paused");
        else if (live)     lv_label_set_text(s_np_state, "* Now playing (live)");
        else               lv_label_set_text(s_np_state, "* Now playing");
    }
    if (s_lbl_playbtn) lv_label_set_text(s_lbl_playbtn, pz ? "Resume" : "Pause");

    /* 收藏键：★ = 已收藏，☆ = 未收藏；不是电台就灰掉（本地文件没法收藏）
     * ★★ 符号必须【直接写 UTF-8 字符】，不许写 "\2606"——
     *   八进制 \ddd 上限 3 位，\2606 = \260 + '6'，那个 6 会漏到屏幕上，
     *   兰兰 10-05 实机看到的就是「6 收藏」/「5 已收藏」。*/
    int fcur = app_radio_station_current();
    if (s_lbl_favbtn) {
        if (fcur < 0) {
            lv_label_set_text(s_lbl_favbtn, "☆ Fav");
            lv_obj_set_style_text_color(s_lbl_favbtn, lv_color_hex(0x3A424C), 0);
        } else if (app_fav_has(fcur)) {
            lv_label_set_text(s_lbl_favbtn, "* Favorited");
            lv_obj_set_style_text_color(s_lbl_favbtn, C_GREEN, 0);
        } else {
            lv_label_set_text(s_lbl_favbtn, "☆ Fav");
            lv_obj_set_style_text_color(s_lbl_favbtn, C_TEXT, 0);
        }
    }

    /* ★ 返回键上的字跟着来源页变：用户按之前就知道会去哪一页。
     *   （s_lbl_playback 在第 3 页建好之前是 NULL，所以要判空）*/
    if (s_lbl_playback) {
        char want[24];
        snprintf(want, sizeof(want), "‹ %s", play_from_name());
        if (strcmp(lv_label_get_text(s_lbl_playback), want) != 0) {
            lv_label_set_text(s_lbl_playback, want);
        }
    }

    spec_refresh();
}

/* 上一个 / 下一个 —— 同一个键，两种音源（10-03 第八次）：
 *   在放电台 → 在整份内置台单里换台（119 台循环，换台后首页大卡同步）
 *   在放本地 → 在同一目录里换曲，到末尾回卷 */
static void local_step(int step)
{
    if (!app_radio_is_playing()) {
        if (s_np_state) lv_label_set_text(s_np_state, "Nothing playing");
        tap_note("Nothing playing");
        return;
    }

    if (app_radio_station_current() >= 0) {
        int nx = app_radio_station_step(step);
        if (nx < 0) {
            if (s_np_state) lv_label_set_text(s_np_state, "x Tuning failed");
            ESP_LOGW(TAG, "station_step(%d) 失败", step);
            return;
        }
        s_hr_sel = nx;
        home_update_card(nx);
        if (s_grid_home) grid_fill(s_grid_home);
        if (s_grid_list) grid_fill(s_grid_list);
        player_refresh();
        return;
    }

    char nm[96];
    esp_err_t r = app_radio_play_next(s_local_dir, step, nm, sizeof(nm));
    if (r != ESP_OK) {
        /* 换不了就把原因留在状态行上（大多是「这一目录只有一首」）*/
        if (s_np_state) lv_label_set_text(s_np_state, "x No other audio here");
        ESP_LOGW(TAG, "play_next(%d) -> %s", step, esp_err_to_name(r));
        return;
    }
    player_refresh();
}

static void ev_prev_station(lv_event_t *e)
{
    (void)e;
    tap_note("Prev");
    local_step(-1);
}

static void ev_next_station(lv_event_t *e)
{
    (void)e;
    tap_note("Next");
    local_step(1);
}

static void ev_play_toggle(lv_event_t *e)
{
    (void)e;

    /* 正在放（或暂停中）→ 真的暂停/继续 */
    if (app_radio_is_playing()) {
        app_radio_toggle_pause();
        tap_note(app_radio_is_paused() ? "Pause" : "Resume");
        player_refresh();
        return;
    }

    /* 没在放：首页选过台就先试着重起那一台（比如刚开机、或上一台流断了）*/
    if (s_hr_sel >= 0 && app_radio_play_station(s_hr_sel) == ESP_OK) {
        tap_note(app_st_get(s_hr_sel)->name);
        home_update_card(s_hr_sel);
        player_refresh();
        return;
    }

    /* 连台都没选：这一键当「试听提示音」，至少能验证喇叭是通的 */
    s_playing = !s_playing;
    tap_note("Chime");
    if (s_playbtn_lbl)   lv_label_set_text(s_playbtn_lbl, s_playing ? "Pause" : "Playing");
    if (s_lbl_playstate) lv_label_set_text(s_lbl_playstate, s_playing ? "Playing" : "Paused");
    app_audio_beep_async();
}

/* 收藏/取消收藏【正在播的这一台】。
 * ★ 兰兰 10-03：「如果能在硬件版也能做收藏设置最好了」——
 *   收藏不能只靠长按网格（有些界面触不到），播放页这个键是最直接的入口。*/
static void ev_fav_current(lv_event_t *e)
{
    (void)e;
    int cur = app_radio_station_current();
    if (cur < 0) {
        tap_note("Not a station (local files cannot be favorited)");
        return;
    }
    on_grid_fav(cur);
    player_refresh();
}

/* 回 SD 卡列表挑下一首（导航栏里没有「本地音频」入口，所以这页要留个门）*/
static void ev_open_sd_list(lv_event_t *e)
{
    (void)e;
    tap_note("Local files");
    s_play_from = 5;              /* 手动跳过去的，回也回 SD 页 */
    ui_sd_page_refresh();
    goto_page(5);
}

/* ============================================================
 *  ★★★ 播放页的「本列表」键（2026-10-04 第二十一次新增）
 * ============================================================
 * 兰兰原话：「播放页还是需要一个正在播放电台或者本地音乐的
 *   所在栏目或者文件夹的列表按键比较好，提示音在状态页已经有了，无需重复」。
 *
 * 做什么：把「我现在这一首是从哪儿来的」那一栏直接摊开。
 *   · 在播网络台 → 打开【它所属的栏目】的全屏台单（第 7 页）
 *     （若它属于某个省，就打开那个省 —— 从发现页选来的台这样更合理）
 *   · 在播本地歌 → 回到 SD 当前目录（第 6 页），那就是它所在的文件夹
 *   · 空闲        → 提示「还没在播」
 *
 * ★ 为什么这个键比提示音有用：提示音只是自测扬声器的手段，
 *   状态页已经有一个；播放页的三个小键里，收藏和「本地」都是
 *   「跳到某个固定页」，缺一个「跳到【当前】上下文」的入口。
 */
static void ev_open_own_list(lv_event_t *e)
{
    (void)e;
    if (!app_radio_is_playing()) { tap_note("Not playing"); return; }

    if (app_radio_is_stream()) {
        int cur = app_radio_station_current();
        if (cur < 0 || cur >= app_radio_station_count()) { tap_note("Station list changed"); return; }
        unsigned char pv = app_st_get(cur)->prov;
        if (pv != NET_PROV_NONE && pv < NET_PROV_N) {
            /* 有地区归属的台：打开该地区（发现页选来的台走这条）*/
            s_list_mode  = LIST_BY_PROV;
            s_list_prov  = pv;
            s_list_title = xs_prov_long_en[pv];
        } else {
            /* 全国台：按栏目打开 */
            s_list_mode  = LIST_BY_CAT;
            s_list_cat   = app_st_get(cur)->cat % NET_CAT_N;
            s_list_title = xs_cat_name_en[s_list_cat];
        }
        s_list_back  = 2;              /* 返回键回播放页 */
        s_play_from  = 6;              /* 播放页返回键也回这一页 */
        tap_note(s_list_title);
        list_page_refresh();
        goto_page(6);
    } else {
        /* 本地音频 ⇒ 它的「所在文件夹」就是 SD 页当前目录 */
        s_sd_count_dirty = true;
        s_play_from = 5;
        tap_note("Folder");
        ui_sd_page_refresh();
        goto_page(5);
    }
}

/* ============================================================
 *  播放页的返回键（兰兰 10-03）
 * ============================================================
 *  「在发现页选中要听的电台自动进入播放页但是缺少一个返回发现页的按键，
 *    要返回刚刚在发现页选择的地区栏；如果是首页选中的电台自动进入播放页，
 *    那也可以返回键返回首页，做到从哪里来能够回到哪里」
 *
 *  ★ 为什么不写死「回首页」：那从发现页进来就回错地方了。
 *    每层网格建的时候把 from_page 写进 st_grid_t，点格子时记进 s_play_from，
 *    本键就 goto_page(s_play_from)。从全屏分类页进来时回到第 6 页，
 *    第 6 页自己的「‹ 返回」再回发现/首页 —— 两层构成完整的一条回退链。
 *  ★ 按键上的字也跟着变（首页/发现/本地/电台列表），不然用户不知道会去哪。*/
static lv_obj_t *s_btn_playback;

static const char *play_from_name(void)
{
    switch (s_play_from) {
    case 0:  return "Home";
    case 1:  return "Browse";
    case 5:  return "Local";
    case 6:  return "Stations";
    default: return "Home";
    }
}

static void ev_play_back(lv_event_t *e)
{
    (void)e;
    int to = s_play_from;
    if (to < 0 || to >= PAGES_N) to = 0;
    /* ★ 从全屏分类页回来时，那个页面要重新按当前筛选刷一次 ——
     *   收藏在播放页被改过时（比如按了 ☆ 收藏），列表要立刻反映出来。*/
    if (to == 6) list_page_refresh();
    if (to == 5) ui_sd_page_refresh();
    tap_note("Back");
    goto_page(to);
}

/* ---- 动态频谱（12 根柱子）----
 * ★ 这不是装饰动画：柱高来自 app_radio_spectrum()，那是 app_radio 里
 *   对解码后 PCM 做 Hann 窗 + Goertzel 算出来的真实频段能量。
 *   （首页那张大卡上原来的 spectrum() 是写死的静态示意，这次一并换成真的。）
 *
 * ★★★★★★ 10-05 v1.39：加【三种显示模式可切换】（兰兰：「播放页出来频谱还有什么
 *   动态音频展示，可以切换吗，不占用过多资源的情况下更加灵动」）
 *
 *   三种模式（数据源完全相同，只是摆法不同）：
 *     0 柱  从底部长起来的柱状图（原来那版）
 *     1 镜  以中线为轴上下对称，像示波器波形 —— 比柱状更「有生命」
 *     2 环  12 根柱摆成一个圆环，随能量向外/向内伸缩
 *
 *   ★★★ 资源铁律：三种模式**共用同一批 12 个 box**，切换只改 pos/size/bg_color，
 *     一个新 lv_obj 都不建。
 *     池只剩 15 KB；为三种模式各建一套 = 36 个 box ≈ 12 KB ⇒ 直接抽干。
 *     现在 0 额外开销。切换动作只是改几个整数，CPU 开销可以忽略。
 *
 *   ★ 为什么不做「更多模式」：每种新摆法都要一份坐标表，
 *     而 480×320 上 12 个元素能摆出的、既好看又看得清形状的排列就这几种。
 *     再加就是凑数了。*/
#define SPEC_SHOW 12                  /* 与 app_radio 的 SPEC_BINS 对齐 */
#define SPEC_BAR_W 16
#define SPEC_BAR_GAP 4
#define SPEC_MODE_N 3                 /* 柱 / 镜 / 环 */
static lv_obj_t *s_spec_bar[SPEC_SHOW];
static int       s_spec_x0 = 4;       /* 频谱区左边 */
static int       s_spec_w   = SEEK_BAR_W;    /* 频谱区宽（与进度条同宽，同一个宏）*/
static int       s_spec_mode = 0;            /* 当前模式，见上面的枚举说明 */
static lv_obj_t *s_btn_spec;                  /* 切换键 */
static lv_obj_t *s_lbl_spec;                  /* 切换键上的文字 */

static void spec_build(lv_obj_t *p)
{
    /* 12 根 × (16+4) - 4 = 236，居中放在 244 里 ⇒ 左边留 4 px */
    s_spec_x0 = (s_spec_w - (SPEC_SHOW * (SPEC_BAR_W + SPEC_BAR_GAP) - SPEC_BAR_GAP)) / 2;
    for (int i = 0; i < SPEC_SHOW; i++) {
        /* 初始给 3px 底线：有东西在屏幕上，不是一片空白 */
        s_spec_bar[i] = box(p, s_spec_x0 + i * (SPEC_BAR_W + SPEC_BAR_GAP),
                            SPEC_Y + SPEC_H - 3, SPEC_BAR_W, 3, C_GREEN, 2);
    }
}

/* ---- 切模式：柱 → 镜 → 环 → 柱 ---- */
static void spec_mode_next(void)
{
    s_spec_mode = (s_spec_mode + 1) % SPEC_MODE_N;
    if (s_lbl_spec) {
        static const char *const nm[SPEC_MODE_N] = { "Bars", "Mirror", "Ring" };
        lv_label_set_text(s_lbl_spec, nm[s_spec_mode]);
    }
    spec_refresh();          /* 立刻重画，别等下一个 100ms 周期 */
}

static void ev_spec_mode(lv_event_t *e)
{
    (void)e;
    spec_mode_next();
    tap_note("Spectrum");
}

/* 按真实频谱重画。淡出到底（没在放就全塌成 3px 底线）。
 * ★ 三种模式共用这一份数据，只有「算 pos/size」那段分叉。*/
static void spec_refresh(void)
{
    int n = app_radio_spectrum_bins();
    for (int i = 0; i < SPEC_SHOW; i++) {
        int v = (i < n) ? app_radio_spectrum(i) : 0;
        if (!app_radio_is_playing() || app_radio_is_paused()) v = 0;

        int bx, by, bw, bh;
        /* 低段偏青、高段偏绿，一眼能看出高低频分布 */
        lv_color_t c = (i < 4) ? C_CYAN : ((i < 8) ? C_GREEN : lv_color_hex(0xA7F3D0));

        if (s_spec_mode == 0) {
            /* ---- 模式 0：柱状，从底部长 ---- */
            bh = SPEC_H * v / 100;
            if (bh < 3) bh = 3;
            if (bh > SPEC_H) bh = SPEC_H;
            bw = SPEC_BAR_W;
            bx = s_spec_x0 + i * (SPEC_BAR_W + SPEC_BAR_GAP);
            by = SPEC_Y + SPEC_H - bh;
        } else if (s_spec_mode == 1) {
            /* ---- 模式 1：镜像，以中线为轴上下对称（示波器波形）----
             * 半高最大只能到 SPEC_H/2，否则会超出频谱区压到上下的行。
             * 中线留 2px 基准线：v=0 时上下各 1px，看起来像一条平线。*/
            int half = (SPEC_H / 2) * v / 100;
            if (half < 1) half = 1;
            if (half > SPEC_H / 2) half = SPEC_H / 2;
            bw = SPEC_BAR_W;
            bx = s_spec_x0 + i * (SPEC_BAR_W + SPEC_BAR_GAP);
            bh = half * 2;
            by = SPEC_Y + SPEC_H / 2 - half;
        } else {
            /* ---- 模式 2：圆环，12 根柱围一圈 ----
             * 圆心放在频谱区正中，R=52，半径随能量在 34~62 之间伸缩。
             * ★ 角度用整数查表（sin/cos × 1000），不引 libm：
             *   12 个点每 30° 一个，写死 12 组常数即可，flash 省 8 KB。
             * ★ 柱宽 8（不是 16）：环上一周 2πR ≈ 390px 分 12 段，
             *   每段约 32px，16px 宽会连成一片看不出是「环」。*/
            static const int SIN12[SPEC_SHOW] = {
                0,  500,  866,  1000,  866,  500,
                0, -500, -866, -1000, -866, -500
            };
            static const int COS12[SPEC_SHOW] = {
                1000, 866, 500, 0, -500, -866,
                -1000, -866, -500, 0, 500, 866
            };
            int r = 34 + (28 * v) / 100;
            int cx = s_spec_x0 + s_spec_w / 2;
            int cy = SPEC_Y + SPEC_H / 2;
            bw = 8;
            bh = 8 + (14 * v) / 100;      /* 能量大时点更大更亮 */
            bx = cx + (COS12[i] * r) / 1000 - bw / 2;
            by = cy + (SIN12[i] * r) / 1000 - bh / 2;
            /* 环上低频在下、高频在上：按角度染色，比按序染色有立体感 */
            c = (SIN12[i] > 0) ? C_CYAN : ((SIN12[i] < 0) ? C_GREEN
                                                      : lv_color_hex(0xA7F3D0));
        }
        lv_obj_set_size(s_spec_bar[i], bw, bh);
        lv_obj_set_pos(s_spec_bar[i], bx, by);
        lv_obj_set_style_bg_color(s_spec_bar[i], c, 0);
        /* 环形时把圆角拉满变成「点」，柱状/镜像保持小圆角 */
        lv_obj_set_style_radius(s_spec_bar[i], (s_spec_mode == 2) ? 4 : 2, 0);
    }
}

/* ============================================================
 *  电平表（12 格）
 *  ============================================================
 *  频谱告诉你「声音的音色如何」，电平告诉你「到底有没有在响」。
 *  后者更原始但更可靠 —— 兰兰 10-03 血泪里两次「日志全绿但喇叭没声」，
 *  如果当时屏幕上有一条电平条，1 秒就能定位。
 *
 * ★ 10-03 第十五次撤掉了屏幕上的 12 格电平表（兰兰：「电平幅度可以不要，
 *   换成加粗点进度条，这样更容易操作」）。理由与数据都写在 page_play_build
 *   的版面账里。电平信息没丢：状态页仍打 `level=NN`，autoplay 探针也还在。
 *⇒ 下面是「顶部频谱 + 粗进度条 + 一行提示」的实现，lmeter 相关函数已删。*/

static void page_play_build(lv_obj_t *p)
{
    /* ★ 版面账（CONT 424 x 282，写死坐标前先算一遍，别叠）：
     *     左半 x=4..248（244 宽）
     *        2..22    曲名（F_SD 大字符集，保证网络台名 + SD 歌名都不出方块）
     *       24..46    时长行（18px 琥珀色，直播是 LIVE mm:ss）+ 右侧提示
     *       58..196   频谱 12 根柱（138 高）
     *       204..224  进度条（244 宽 ·20 高 ·加粗好按）
     *       228..244  一行提示（可拖 / 直播只读）
     *     右半 x=256..420（164 宽）
     *        2..28    返回键（84 宽）
     *       34..128   卡片 164x94（曲名 / 来源 / 状态）
     *       136..180  三个操作键（50+6+50+6+50 = 162 ≤ 164）
     *       188       音量文字
     *       202..218  音量滑块
     *       226..266  三个小键
     *       272..282  底部提示
     *
     * ★ 第十五次（兰兰：「电平幅度可以不要，换成加粗点进度条，这样更容易操作」）：
     *   12 格电平表撤掉了。它 12 个 box ≈ 5 KB LVGL 池（池只剩 21 KB），
     *   换来的却是一根手指按不准的 6 px 细条 —— 兰兰试了两次都「拖不动」。
     *   ⇒ 同一个内存换成 20 px 粗条 + 44 px 透明热区，一举两得。
     *   电平信息没丢：状态页仍有 `app_radio_level()` 的串口读数（level=NN）。
     *
     * ★ 为什么把 200x200 的进度环删掉（兰兰 10-03 第十一次）：
     *   「圆圈在，频谱就展不开」—— 环占掉左半 200x200，频谱只剩 58 px 高，
     *   12 根柱子挤在一条窄带里，看着像条装饰线。
     *   进度不能因此丢掉：改成底部那条 6 px 细条（本地文件显示进度，
     *   直播时它就是一条底线）。顺带省下一个 lv_arc（大控件自带样式）。
     *
     * ★ 第十二次（兰兰：「播放页的频谱太大了，看着不舒服，
     *   比首页频谱大一点幅度就够了」）：
     *   186 px 那版是「圆圈删掉之后剩下的空间全给频谱」的做法，
     *   结果 12 根柱子几乎顶满整个左半（186 高 vs 244 宽），
     *   像一片柱状图墙，压过了台名 —— 而台名才是这页的主角。
     *
     * ★ 第十三次（兰兰真机：「播放页左边电台显示有方块，
     *   比如苏州方块方块广播，右边能正常显示苏州儿童广播」）：
     *   这是**我自己上一版引入的 bug**，而且有【两个】独立原因：
     *   ① 我把左边曲名从 F_SD(14px, cjk14) 换成了 F_H1(26px) ——
     *      那是【主字库】，只收 466 个界面文案字，而台名有 717 个字。
     *      「苏州儿童广播」的「儿」「童」正好不在主字库里 ⇒ 两块方块。
     *      （右边 s_lbl_playname 用 F_SD，所以正常 —— 同一个台名两种字体。）
     *   ② 后来改用 F_ST26（台名专用 26px）也不够：它只有【台单里的 716 字】，
     *      而播放页也会显示【SD 卡里的歌名】——那是运行时才知道的名字，
     *      「苏打绿」「五月天」「第七期」这些字一个都不在台单里 ⇒ 照样方块。
     *   ⇒ **结论：播放页的曲名是「网络台名 + 任意 SD 文件名」的混合集合，
     *      只有 F_SD（cjk14 = GB2312 全集 4440 字）能全覆盖。**
     *      所以左右两边【统一用 F_SD】，不为左边另做字库。
     *      「醒目」改成靠【位置 + 上下两行留白 + 一行来源标签】实现，
     *      不再靠更大字号 —— 反正 26px 台名库也是刚踩过坑的东西。
     *   腾出的 46 px 全部给频谱：112 → 158。
     *
     * ★ 「为什么不给左边做个 26px 的 GB2312 全集字库」：
     *   cjk14(4440 字) 已经 1722 KB；按 (26/14)^2 放大到 26px 就是 4.7 MB，
     *   分区总共才 7 MB —— 放不下。
     *   而台名专用 26px 只要 741 KB，可它只覆盖台单那 716 字（本页第二种来源盖不住）。
     *   ⇒ 这不是「省事」，是【只有 F_SD 这一个可行解】。
     *   ⇒ 连带后果：xs_font_st26（741 KB 台名 26px）生成出来后发现【无人使用】，
     *     已从 gen_fonts_st.py 的 JOBS 里撤掉。台名 26px 这条路走不通。*/
    s_np_name = label(p, 4, 2, F_SD, C_TEXT, "Not playing");
    lv_obj_set_width(s_np_name, 244);
    lv_obj_set_height(s_np_name, 20);
    lv_label_set_long_mode(s_np_name, LV_LABEL_LONG_DOT);

    /* ★ 醒目靠这一行：台名是 F_SD(14px)，下面这行 18px 琥珀色写「网络电台 · LIVE」，
     *   尺寸与颜色形成主次，比单纯放大字号更耐看。*/
    /* ★★★ 10-05 兰兰真机反馈：「点电台进入播放页 LIVE 标签和播放时间挡住
     *   复标签：网络直播的网络 2 个字」。
     *   解开来看：不是「LIVE」和「网络直播」互相压，而是【时间那一行
     *   没有宽度约束】—— s_np_time 用 F_H2(24px) 且从不set_width，
     *   而 player_refresh 里直播时会写 "LIVE  00:32  延迟5s"（最长约 17 字符），
     *   24px 下 17×12 ≈ 204px，从 x=4 伸到 208 ⇒ 越过了 x=96 的 s_np_hint，
     *   把「网络直播 · 64 kbps」的前两个字盖掉了。
     *   v1.35 时这里是 18px（≈153px），刚好还在 96 右边不越界，
     *   换成 24px 就暴露了。⇒ 又是一次「砍字号档」引出的连带问题。
     *   修法：给它固定宽 88 + LONG_DOT，并把 hint 右移到 96 之后不变，
     *   超长文案（如延迟秒数变两位数）被省略号截断，绝不越界。*/
    /* ★★ 10-05 兰兰复验：「延迟时间标签挡住了下方直播流三个字」——
     *   上一版把 s_np_time 限宽到 88px 就以为万事大吉，
     *   可 16px 下「LIVE  00:32  延迟5s」实测【151px】（hmtx 折算，非估算），
     *   88px 装不下 ⇒ LONG_DOT 之前先 WRAP 换行，第二行 42~59 正好压住
     *   y=44 的 s_lbl_level（「直播流 · 进度条只读」的前三个字）。
     *
     *   修法（宽度全部改实测值）：左半区 x=4~256 共 252px，单行排得下：
     *     「LIVE  00:32  延迟5s」  16px = 151px  ⇒ 限宽 152
     *     「网络直播 · 64 kbps」   12px = 114px  ⇒ 限宽 148（不变，够）
     *     「直播流 · 进度条只读」 12px = 120px  ⇒ 限宽 244（不变，够）
     *   ★ 纵向账（行高是从字库 .c 的 .line_height 读回来的真实值，不是猜的）：
     *     2 ~ 15   s_np_name  12px 行高 13
     *     24 ~ 42  s_np_time  16px 行高 18 ← y 由 25 改 24，见下
     *     44 ~ 57  s_lbl_level 12px 行高 13 ⇒ 顶边 44
     *     58 ~ 频谱 SPEC_Y=58
     *   ⇒ 间隙 2px。
     *   ⚠️ 顺带纠正一个我自己抄错的数：16px 的 .line_height 是【18】不是 17。
     *   按 17 算会以为「25+17=42，间隙 2px 够宽」，
     *   按真实 18 算则是 25+18=43，只剩 1px —— 卡在临界上。
     *   ⇒ **行高必须查文件，不要凭印象写进注释**（本工具 --lineheight 就是干这个的）。*/
    s_np_time = label(p, 4, 24, F_MID, C_AMBER, "--:--");
    lv_obj_set_width(s_np_time, 152);
    lv_label_set_long_mode(s_np_time, LV_LABEL_LONG_DOT);

    /* ★ 10-05：hint 从 (96,26) 挪到 (128,44)，与 s_lbl_level 同一行并排。
     *   起因：s_np_time 放宽到 152 后占 4~156，与原来的 hint(96~244) 重叠 60px。
     *   而 y=44 那行左半区还有 124px 空位（lbl_level 只要 120px）⇒ 换到那行最省。
     *   实测宽度：12px「直播流 · 进度条只读」= 120px，12px「网络直播 · 64 kbps」= 114px。
     *   128+114 = 242 < 244（与 s_lbl_level 同右边界），两行都不换行。*/
    s_np_hint = label(p, 128, 44, F_TINY, C_MUTED, "Pick a track in SD audio");
    lv_obj_set_width(s_np_hint, 116);
    lv_label_set_long_mode(s_np_hint, LV_LABEL_LONG_DOT);

    /* ---- 频谱（12 根柱，数据来自 app_radio 的 Goertzel 分析）---- */
    spec_build(p);

    /* ---- 进度条：本地可拖动，直播只显示 ----
     *   ★ 兰兰 10-03 第十四次：「细条看到了，但不起作用，无法拖拉」，
     *     第十五次：「本地播放进度条还是没有」。
     *   ⇒ 真因：6 px 高的 box 当触摸目标，按不准。
     *     现在加粗到 20 px，另罩一个 44 px 高的透明热区，事件全挂热区。*/
    s_seek_bar_w = SEEK_BAR_W;
    s_arc_play = box(p, SEEK_BAR_X, SEEK_BAR_Y, SEEK_BAR_W, SEEK_BAR_H, C_HILITE, 4);
    s_prog_play = box(p, SEEK_BAR_X, SEEK_BAR_Y, 0, SEEK_BAR_H, C_GREEN, 4); /* 宽度由 player_refresh 改 */
    /* ---- 拖动手柄（第二十一次：18×34 的圆角把手，常显）----
     * ★ 之前是 4 px 宽、且只在拖动时显形，看着就是「绿线上的一道白线」，
     *   兰兰不知道能拖。现在做成一个明显的小把手，一直停在当前进度上。*/
    s_seek_knob = box(p, SEEK_BAR_X, SEEK_BAR_Y - 4, SEEK_KNOB_W, SEEK_KNOB_H,
                      lv_color_hex(0xF2F5F7), 6);
    /* 把手只显示，不收事件：命中全交给下面的透明热区，
     * 否则它会把热区挡掉一块（透明对象照样吃触摸，10-04 踩过）。*/
    lv_obj_clear_flag(s_seek_knob, LV_OBJ_FLAG_CLICKABLE);
    /* 透明热区：视觉上什么都没有，但 34 px 好按 */
    s_seek_hit = box(p, SEEK_BAR_X, SEEK_HIT_Y, SEEK_BAR_W, SEEK_HIT_H,
                     lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_seek_hit, LV_OPA_TRANSP, 0);
    lv_obj_set_flag(s_seek_hit, LV_OBJ_FLAG_CLICKABLE, true);
    /* ★★ 热区必须【不可滚】：LV_DIR_NONE 让 LVGL 不把它当滚动对象，
     *   这一条对 PRESSED/PRESSING 能不能收到是决定性的
     *   （兰兰连报三次「本地拖动还是不行」= 只有 PRESSING 收不到）。
     *   ⚠️ 但它同时意味着 LVGL 【不会】把拖动派发成 PRESSING ——
     *   所以拖动完全不依赖事件，改由 spec_timer_cb → seek_drag_poll()
     *   每 100 ms 主动读一次 indev 当前点。这是唯一稳的做法：
     *   要自己发事件就得靠 scroll 抢手势，一抢就把 PRESSING 也弄丢。*/
    lv_obj_set_scroll_dir(s_seek_hit, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(s_seek_hit, LV_SCROLLBAR_MODE_OFF);
    /* 只挂「按下」与「松手」两个边界事件，中间的移动完全自管。
     * ⚠️ 上一版还挂了 LV_EVENT_PRESSING，它必然收不到，留着是误导。*/
    lv_obj_add_event_cb(s_seek_hit, ev_seek_press,    LV_EVENT_PRESSED,     NULL);
    lv_obj_add_event_cb(s_seek_hit, ev_seek_release,   LV_EVENT_RELEASED,    NULL);
    lv_obj_add_event_cb(s_seek_hit, ev_seek_release,   LV_EVENT_PRESS_LOST,  NULL);
    ESP_LOGI(TAG, "seek bar: x=%d w=%d y=%d h=%d, hit area y=%d h=%d, total=%d can_seek=%d",
             SEEK_BAR_X, SEEK_BAR_W, SEEK_BAR_Y, SEEK_BAR_H, SEEK_HIT_Y, SEEK_HIT_H,
             app_radio_total_s(), (int)app_radio_can_seek());

    /* ---- 快退 2 / 退 10 / 进 10 / 进 2（见版面宏的注释）----
     * 兰兰原话：「快进10有了，但是10秒太短，需要分钟级别的，2分钟，10分钟」。
     * ★ 文案用汉字「退2分/进2分」：« » 这两个符号 SimHei/Deng 都没有 ⇒ 方块。
     *
     * ★★ 2026-10-04 第二十次：兰兰报「快进10分钟有效，但经常要触碰几次才有效，
     *   2 分钟无反应可能不灵敏」。
     *   真因不是「不灵敏」，是【点击被 LVGL 判成滚动吃掉了】：
     *     tick() 给按钮挂了 ev_press(LV_EVENT_PRESSED) 去改透明度 opa，
     *     而「改样式 → 重排 → 对象坐标变了」，LVGL 在同一帧里
     *     发现指针下的对象变了，就把它转成 scroll 并放弃 CLICKED。
     *     手指按得越重越容易触发 ⇒ 表现为「要碰几次」。
     *   ⇒ 修法：跳转键【不挂 ev_press/ev_release】，改挂 LV_EVENT_PRESSED 自己做
     *     一次性动作（按下即跳，不等松手）。按下即响应 = 不可能被判成滚动。
     *     视觉反馈用 tap_note 的文字提示，不靠改按钮样式。*/
    {
        static const int      secs[4] = { -120, -600, 600, 120 };
        static const char *const txts[4] = { "-2m", "-10m", "+10m", "+2m" };
        for (int i = 0; i < 4; i++) {
            int x = SEEK_BAR_X + i * (SEEK_STEP_W + SEEK_STEP_GAP);
            /* ★ 高度从 20 加到 30（SEEK_STEP_H），手指目标大一圈 */
            lv_obj_t *b = box(p, x, SEEK_ROW1_Y, SEEK_STEP_W, SEEK_STEP_H,
                              C_HILITE, 8);
            lv_obj_add_flag(b, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_scroll_dir(b, LV_DIR_NONE);
            /* 按下即跳：不等 RELEASED ⇒ 不会有「按住不动算滚动」的窗口 */
            lv_obj_add_event_cb(b, ev_seek_step, LV_EVENT_PRESSED,
                                (void *)(intptr_t)secs[i]);
            /* ★ 第二十二次：退用青、进用绿（兰兰：「关键按键的字体最好是彩色的」）。
             *   方向靠颜色分，比靠文字顺序记更省心：左手边一排青=往回，
             *   右手边一排绿=往前，中间那条分界就是「现在」。*/
            lv_obj_t *t = label(b, 0, 0, F_BODY, (i < 2) ? C_CYAN : C_GREEN, txts[i]);
            lv_obj_center(t);
        }
    }

    /* ---- 电平表（12 格条）：频谱看「音色」，这条看「有没有声音」 ----
     * ★ 为什么自己拿小方块拼而不用 lv_linemeter：那个控件自带一整套
     *   style（渐变/边框/指示器每个都是 lv_obj），一个就吃掉几 KB。
     *   池只剩 21 KB，得省。12 个 box ≈ 5 KB。*/
    /* 电平表已撤（10-03 第十五次）。这一行改成拖动提示，
     * 每次 player_refresh 会按能不能拖更新文案。
     * ★ 第十八次挪到 y=228 之下没空间了（进度条现在 210~238），
     *   改放在【频谱区上方那行】—— 也就是 y=44，紧贴时间行下面。
     *   反正 58 才是频谱顶，44~56 这一行本来就空着。*/
    /* ★ 10-05：宽度 244 → 120（实测 12px「直播流 · 进度条只读」= 120px）。
     *   右半 124px 让给 s_np_hint，两段边界明确，避免再次叠字。
     *   另两处文案实测：12px「拖粗条跳位置 · 上面四键跳分钟」= 156px、
     *   12px「读不出总长 · 只能用四个跳转键」= 168px —— 都超过 120px，
     *   会出省略号。这是【故意接受的取舍】：兰兰要求优先保住左半区不叠字，
     *   且这行是操作提示，截断成「拖粗条跳位置 · 上面四键跳…」仍能看懂。*/
    s_lbl_level = label(p, 4, 44, F_TINY, C_MUTED, "Drag the thin bar");
    lv_obj_set_width(s_lbl_level, 120);

    /* ---- 右半：返回键（回来源页）+ 卡片 + 控制 ---- */
    int x = 256;

    s_btn_playback = tick(p, x, 2, 84, 26, C_HILITE, 13, NULL);
    lv_obj_add_event_cb(s_btn_playback, ev_play_back, LV_EVENT_CLICKED, NULL);
    s_lbl_playback = label(s_btn_playback, 0, 0, F_TINY, C_TEXT, "< Home");
    lv_obj_center(s_lbl_playback);

    label(p, x + 90, 6, F_TINY, C_MUTED, "Now Playing");

    lv_obj_t *c = box(p, x, 32, 164, 94, C_PANEL, 14);
    lv_obj_set_style_border_width(c, 1, 0);
    lv_obj_set_style_border_color(c, C_LINE, 0);

    /* ★ 曲名必须用大字符集（F_SD）：卡里的中文名源码里从没出现过，
     *   用扫源码生成的小字库只会画成方块（10-03 兰兰反馈过）。*/
    s_lbl_playname = label(c, 12, 6, F_SD, C_TEXT, "Playing locally");
    lv_obj_set_width(s_lbl_playname, 140);
    lv_obj_set_height(s_lbl_playname, 36);
    lv_label_set_long_mode(s_lbl_playname, LV_LABEL_LONG_DOT);

    /*★ 10-05 兰兰真机反馈：「下方直播流 进度条只读挡住一半」。
     *   解开来看：原文是【两行】文案（"\n" 分隔），行距 3 px。
     *   12px 两行 + 行距 ≈ 12+3+12 = 27px，从 y=44 起 ⇒ 占到 y=71；
     *   而 s_np_state（● 正在播放（直播））固定在 y=72 —— 只差 1px。
     *   ★ 但 v1.36 把这一页的F_BODY/F_TINY 都换成 12px 后，
     *   「网络电台 · 直播\n交通台 · 第 12/1246 台」这类文案里
     *   数字与汉字混排的行宽变宽，第二行「交通台 · 第 12/1246 台」
     *   在 140px 宽的框里【放不下又换了一次行】⇒ 变成三行，
     *   27px 变41px ⇒ 压到 y=72 的状态行，这就是「挡住一半」。
     *   修法：宽度由 140 放宽到容器全宽 140→144 不够，本质是行数；
     *   改成【固定两行、禁止再换行】：每行 12px + 行距 2 = 26px，
     *   y=44~70，与 y=72 留2px 间隙。同时把行距 3→2 收紧。
     *   ⚠️ s_np_meta 用 LONG_WRAP 是【故意的】，它文案天生两行；
     *     要防的是第二行自己也换行 ⇒ 缩短第二行文案长度更稳妥。*/
    s_np_meta = label(c, 12, 44, F_TINY, C_MUTED, "Local audio - SD card");
    lv_obj_set_width(s_np_meta, 140);
    lv_obj_set_style_text_line_space(s_np_meta, 2, 0);

    s_np_state = label(c, 12, 74, F_TINY, C_AMBER, "Idle");
    lv_obj_set_width(s_np_state, 140);
    lv_label_set_long_mode(s_np_state, LV_LABEL_LONG_DOT);

    /* 三个操作键
     * ★ 第二十二次：字体改彩色（兰兰：「所有关键按键的按键字体最好是彩色的」）。
     *   原来三个都是 C_TEXT 白 ⇒ 跟旁边那些不能按的文字长得一样，
     *   扫一眼分不出哪些能按。改成：
     *     上一个 / 下一个 → C_CYAN（电波青，与绿主色区分）
     *     暂停 / 播放    → C_ONDARK（深字压在绿底上，对比最强，它是主角）*/
    lv_obj_t *prev = tick(p, x, 132, 50, 44, C_HILITE, 14, NULL);
    lv_obj_add_event_cb(prev, ev_prev_station, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pt = label(prev, 0, 0, F_TINY, C_CYAN, "Prev");
    lv_obj_center(pt);

    lv_obj_t *pp = tick(p, x + 56, 132, 50, 44, C_GREEN, 14, NULL);
    lv_obj_add_event_cb(pp, ev_play_toggle, LV_EVENT_CLICKED, NULL);
    s_lbl_playbtn = label(pp, 0, 0, F_TINY, C_ONDARK, "Pause");
    lv_obj_center(s_lbl_playbtn);

    lv_obj_t *nx = tick(p, x + 112, 132, 50, 44, C_HILITE, 14, NULL);
    lv_obj_add_event_cb(nx, ev_next_station, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pt3 = label(nx, 0, 0, F_TINY, C_CYAN, "Next");
    lv_obj_center(pt3);

    /* ---- 音量 ---- */
    s_lbl_vol = label(p, x, 182, F_TINY, C_MUTED, "Vol 60%");

    lv_obj_t *sl = lv_slider_create(p);
    lv_obj_set_pos(sl, x, 198);
    lv_obj_set_size(sl, 164, 16);
    lv_slider_set_range(sl, 0, 100);
    lv_slider_set_value(sl, app_audio_get_volume(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, C_HILITE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sl, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(sl, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, C_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sl, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(sl, 8, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, C_TEXT, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sl, 3, LV_PART_KNOB);
    lv_obj_add_event_cb(sl, ev_vol_changed, LV_EVENT_VALUE_CHANGED, NULL);
    if (s_lbl_vol) lv_label_set_text_fmt(s_lbl_vol, "Vol %d%%", app_audio_get_volume());

    /* 三个小键：收藏 / 本地 / 本列表
     * ★ 第二十一次：第三个键从「提示音」换成「本列表」。
     *   兰兰：「播放页还是需要一个正在播放电台或者本地音乐的所在栏目
     *   或者文件夹的列表按键比较好，提示音在状态页已经有了，无需重复」。
     *
     * ★ 第二十二次：高度 40 → 34（220..254），腾出 258..280 给【倍速键】。
     *   横向账（右半 164 宽）：3×50 + 2×6 = 162 ≤ 164 ✓（没变）*/
    lv_obj_t *fb = tick(p, x, 220, 50, 34, C_HILITE, 12, NULL);
    lv_obj_add_event_cb(fb, ev_fav_current, LV_EVENT_CLICKED, NULL);
    s_lbl_favbtn = label(fb, 0, 0, F_TINY, C_AMBER, "☆ Fav");
    lv_obj_center(s_lbl_favbtn);

    lv_obj_t *lb = tick(p, x + 56, 220, 50, 34, C_HILITE, 12, NULL);
    lv_obj_add_event_cb(lb, ev_open_sd_list, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb1 = label(lb, 0, 0, F_TINY, C_CYAN, "Local");
    lv_obj_center(lb1);

    lv_obj_t *bt = tick(p, x + 112, 220, 50, 34, lv_color_hex(0x1B3A31), 12, NULL);
    lv_obj_add_event_cb(bt, ev_open_own_list, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btl = label(bt, 0, 0, F_TINY, C_GREEN, "This list");
    lv_obj_center(btl);

    /* ---- 倍速键（第二十二次新增）----
     * ★★ 兰兰：「如果可以，增加倍速播放按键1.2X 或1.4X，位置放不下就算了」。
     *   ⇒ 放得下，所以做了。位置：258..280（22 高），78 宽。
     *
     * ★ 只有一个键而不是三个（1.0X/1.2X/1.4X 各一个）：
     *   右半 164 宽塞三个键每个只剩 ~50，而 50 宽放「1.4X」够、
     *   放「常速」就挤；更要紧的是三个键会把 258 那一行挤到 282 以下。
     *   ⇒ 改成【循环切换】：按一下 1.0X → 1.2X → 1.4X → 1.0X。
     *   键上的字始终显示当前档位，一眼知道现在是多少倍。
     *
     * ★ 为什么用 22 高的小键而不是再抢一格 34 高的：
     *   34 高的键在这块屏上手感明显好（跳转键 30 高就比 20 高好用得多）。
     *   而 258..280 之上还剩 254..258 的缝，22 高是「能按准但不占地方」的折中。
     *   真嫌小，下一轮可以把三个小键也缩到 30 高再给倍速 30 高。*/
    s_btn_speed = tick(p, x, 258, 78, 22, lv_color_hex(0x1B3A31), 10, NULL);
    lv_obj_add_event_cb(s_btn_speed, ev_speed_cycle, LV_EVENT_PRESSED, NULL);
    s_lbl_speed = label(s_btn_speed, 0, 0, F_TINY, C_GREEN, "Speed 1.0x");
    lv_obj_center(s_lbl_speed);

    /* ---- 频谱模式切换键（v1.39 新增）----
     * 兰兰：「播放页出来频谱还有什么动态音频展示，可以切换吗，
     *   不占用过多资源的情况下更加灵动」
     *
     * ★ 位置：把原来那行**只读**的静态说明「柱=真实频谱 条=进度」
     *   换成一个可点的循环切换键 —— 不新增版面，而是把一处只占地方不干活的
     *   文案变成控件。原先 12px 实测 93.3px 宽（x+84=340 起 ⇒ 433），
     *   其实已经超出右半区（CONT_W=424）9px，是一处**旧的溢出**，
     *   换成 80px 宽的键正好落在 x+84=340~420（页面右缘 424）内。
     *
     * ★ 事件用 LV_EVENT_PRESSED（按下即切）而不是 CLICKED：
     *   tick() 已经挂了 ev_press 改透明度 → 触发重排 → 坐标变了 →
     *   LVGL 同一帧会把指针下的对象判成滚动并放弃 CLICKED。
     *   这正是 10-04「快进要碰几次」的同一个坑（见上面 ev_seek_step 注释）。*/
    s_btn_spec = tick(p, x + 84, 258, 80, 22, lv_color_hex(0x1B3A31), 10, NULL);
    lv_obj_add_event_cb(s_btn_spec, ev_spec_mode, LV_EVENT_PRESSED, NULL);
    s_lbl_spec = label(s_btn_spec, 0, 0, F_TINY, C_CYAN, "Bars");
    lv_obj_center(s_lbl_spec);
}

/* ============================================================
 *  页面四：夜间模式（含亮度调节）
 * ============================================================ */
static lv_obj_t *s_lbl_bright;
static lv_obj_t *s_lbl_sleep_left;   /* 睡眠定时剩余时间（10-04 新增）*/

static void ev_bright_changed(lv_event_t *e)
{
    lv_obj_t *sl = lv_event_get_target(e);
    int v = (int)lv_slider_get_value(sl);
    app_sys_set_brightness(v);
    if (s_lbl_bright) lv_label_set_text_fmt(s_lbl_bright, "Brightness %d%%", v);
}

/* ---- 睡眠定时：只用固定档位 ----
 * ★★ 10-04 第二十七次（兰兰原话「定时自由按键还是取消了比较好，
 *   把时间固定按键改好了多几个选择即可」）：
 *   上一版的 －/数值/＋ 三颗【删掉了】。三条理由，都不是审美问题：
 *   ① 数值框太宽（248 px，占整行 2/3），而它显示的「45 分 (步长 5)」
 *      是全页信息密度最低的一块 —— 看得懂「步长 5」的人比看得懂
 *      「还有 47 分 12 秒」的人少得多，那一行才是真正要盯的。
 *   ② 它挡住了剩余时间那行（兰兰原话「输出的还有几分几秒自动停播
 *      被自由定时按键和框挡住了」）。
 *   ③ 固定档已经覆盖绝大多数场景（兰兰两次精简：先去掉 15，再去掉 45；
 *      10-04 15:20 又要求补上 240 = 4 小时，最终
 *      关闭/30/60/90/120/150/180/240）。
 *   ⇒ 结论：固定档 + 一个「关闭」，8 颗，分两行（4 + 4）。*/
#define SLEEP_N 8
static const int s_sleep_mins[SLEEP_N] = {0, 30, 60, 90, 120, 150, 180, 240};

/* 前向声明：page_night_build 在这些之后才用得到它们 */
static void sleep_left_cb(lv_timer_t *t);
static void ev_sleep_pick(int idx);

static void ev_sleep_pick(int idx)
{
    if (idx < 0 || idx >= SLEEP_N) return;
    app_sleep_set_minutes(s_sleep_mins[idx]);
}

/* 剩余时间那一行。每秒刷。*/
static void sleep_left_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_lbl_sleep_left) return;
    if (!app_sleep_is_on()) {
        /* 已到点：说清楚「停了」，别让用户以为定时还在跑 */
        lv_label_set_text(s_lbl_sleep_left,
                          app_sleep_fired() ? "Timer reached - playback stopped" : "");
        return;
    }
    int left = app_sleep_remaining_s();
    int m = left / 60, sec = left % 60;
    lv_label_set_text_fmt(s_lbl_sleep_left, "%d:%02d left before auto-stop", m, sec);
}

/* 睡眠定时的 1 秒心跳：先判到点（要在任何页面都生效），
 * 再刷夜间页那行字（只在夜间页刷，避免别的页白白重绘）。*/
static void sleep_timer_cb(lv_timer_t *t)
{
    (void)t;
    bool fired = app_sleep_tick();
    if (s_cur_page == 3 || fired) sleep_left_cb(t);
}

static void page_night_build(lv_obj_t *p)
{
    /* ★ 10-04 改：原来写死 "23:47" / "10 月 3 日 · 星期六" / "播放中 · 中国之声"，
     *   三行全是假的（兰兰看到方块就是这里）。现在初值给占位，由定时器刷真值。*/
    s_lbl_clock = label(p, 0, 10, F_BIG, C_WARM, "--:--");
    lv_obj_set_width(s_lbl_clock, CONT_W);
    lv_obj_set_style_text_align(s_lbl_clock, LV_TEXT_ALIGN_CENTER, 0);

    /* ★ 10-05 兰兰反馈「夜间页内时钟和当前日期字体可放大」，
     *   但补一句「夜间页时钟保持原状」⇒ 只放大【日期】，时钟维持 F_BIG(24px)。
     *   日期：F_TINY(12px) → F_MID(16px)。它整宽居中、上下都有留白
     *   （时钟在 y=10，状态行在 y=74），16px 字高 16，54+16=70 < 74 ⇒ 不碰。
     *   ⚠️ 时钟为什么不能放大（我查过，别再反复试）：
     *     要 32px 就得新增一档。实测体积（全字库 630 字）= 1.63 MB；
     *     而时钟只显示「23:47」⇒ 只有 0-9 和冒号 11 个字符，
     *     单独生成 32px 纯数字字库只要 13.9 KB —— 技术上可行。
     *     但 24→32 是 1.333 倍非整数缩放，像素风会被拉糊，
     *     而夜间页是这台机唯一「躺下来听」的场景，糊字最毁氛围。
     *     兰兰最终决定：保持 24px。留档说明以免以后重复评估。*/
    s_lbl_date = label(p, 0, 54, F_MID, lv_color_hex(0x8C8471), "Date pending network sync");
    lv_obj_set_width(s_lbl_date, CONT_W);
    lv_obj_set_style_text_align(s_lbl_date, LV_TEXT_ALIGN_CENTER, 0);

    s_lbl_night_state = label(p, 0, 74, F_BODY, C_WARM, "Not playing");
    lv_obj_set_width(s_lbl_night_state, CONT_W);
    lv_obj_set_style_text_align(s_lbl_night_state, LV_TEXT_ALIGN_CENTER, 0);

    /* 细波浪
     * ★★ v1.35 砍掉：原来这里 for (i<40) 建了 40 个 box 当「波浪竖条」。
     *   夜间页进页面就崩的元凶之一 —— 每个 box 约 250~400 B，
     *   40 根就是 **10~16 KB**，而池只剩 13.7 KB。
     *   ⚠️ 纯装饰的东西永远不值 40 个对象。一条线足够表达「这里分隔一下」。
     *   （真要波浪感，用一张小图或改字体符号，不要用对象堆。）*/
    box(p, 0, 94, CONT_W, 1, lv_color_hex(0x2E2A20), 0);

        label(p, 20, 108, F_TINY, lv_color_hex(0x8C8471),
          "Sleep timer (minutes)");
    /*★★★ 10-04：这排胶囊以前是【死的】——建了、写了「15/30/60/90 分」，
     *   点了会变色（grp_apply）但【不做任何事】，因为 on_change 没接。
     *   兰兰要的就是这个功能。现在接上 app_sleep。*/
    grp_init(&s_grp_sleep, 1, lv_color_hex(0x2E2A20), lv_color_hex(0x1B1811),
             C_AMBER, lv_color_hex(0x8C8471), "Timer");
    /* ★ 「关闭」放第一个：它是「回到默认」的动作，放首位最符合直觉，
     *   也免得在最右边（靠近边缘、容易误触）。*/
    /* ★★ 文案只写数字，单位「分钟」提到小标题里 ——
     *   「180 分」比「180」宽 20 px，9 颗挤两行时这 20 px 就是余量。
     *   少两个字换回一整行的呼吸空间，划算。*/
    /* ★★★ 兰兰 10-04 报「点睡眠页就重启」的真正原因（v1.35 修）：
     *   这个数组声明成 mins[SLEEP_N]（= 8）却只给了 7 个初值，
     *   ⇒ 第 8 个元素编译器填 **NULL**。
     *   第 8 颗胶囊 grp_chip(..., mins[7]) ⇒ lv_label_set_text(标签, NULL)
     *   ⇒ 进夜间页时 LVGL v9 走到 strlen/lv_text_get_size(NULL)
     *   ⇒ 立刻崩。而开机建页时不崩，因为那段代码不碰文本。
     *
     *   ⚠️ 教训（并且我【自己又犯了一遍】，写完立刻被编译器抓住）：
     *      「数组长度不要直接写元素个数」这条建议本身有坑 ——
     *      改成 mins[SLEEP_N - 1] 后长度变成 7，可我照旧给了 8 个初值，
     *      编译器立刻报 excess elements（这正是我们想要的：编译期挡住）。
     *      ★★ 所以正确写法是：**长度仍然写元素个数，用「少一个」当护栏是不对的**，
     *         真正的护栏是下面那句 ——【改了 SLEEP_N 就必须同步核对这个数组的项数】，
     *         而 _arrlen_check.py 会自动检查声明长度与实际项数是否一致。
     *
     *   8 颗 = 关闭/30/60/90/120/150/180/240（最后 4 小时）。*/
    static const char *mins[SLEEP_N] = {
        "Off", "30", "60", "90", "120", "150", "180", "240"
    };
    /* ★★★ 10-04 第二十七次：8 颗分两行（4 + 4）。
     *   ★ 兰兰原话「夜间页定时可以做60分钟，90分钟，120分钟，150分钟固定按键」
     *     —— 15 分和 45 分都去掉，只留下 30/60/90/120/150/180。
     *
     *   横向账（CONT_W = LCD_H_RES(480) － RAIL_W(56) = **424**，可用 384）：
     *     每行 4 颗 ⇒ step = 384 / 4 = 96、宽 91
     *     末颗右缘 = 20 + 3*96 + 91 = 399 ≤ 424 ✓（右缘余 25 px）
     *     最宽文案「180」= 3 位数字 ≈ 24 px，加内边距 24 ⇒ 需 48 ≤ 91 ✓（余 43 px，很宽裕）
     *   ⚠️ 6 颗一行是 NG 的：step=64 ⇒ 宽 59，而「180 分」要 68。
     *     上一版 5 颗时余量只剩 3 px —— 【文案去单位】就是为了把这个
     *     3 px 变成 23 px，不要再往回加「分」字。     *   ⚠️ 算横向账一律用 CONT_W，不是 480 —— 再上一版就是这里出的错
     *     （按 480 算末颗落在 452，被导航栏裁掉 28 px）。*/
#define SLEEP_X0    20
#define SLEEP_AVAIL (CONT_W - 2 * SLEEP_X0)      /* 384 */
#define SLEEP_PER_ROW 4
#define SLEEP_STEP  (SLEEP_AVAIL / SLEEP_PER_ROW) /* 76 */
#define SLEEP_CW    (SLEEP_STEP - 5)              /* 91 */
#define SLEEP_CH    38
    for (int i = 0; i < SLEEP_N; i++) {
        /* 行 1 = i 0~4（y 126），行 2 = i 5~8（y 170）。
         * ★ 用取余/整除算行号，不要写死两段循环 ——
         *   以后加第 10 档（210 分）只需把 SLEEP_N 改 10、这里不用动。*/
        int col = i % SLEEP_PER_ROW;
        int row = i / SLEEP_PER_ROW;
        grp_chip(&s_grp_sleep, p, SLEEP_X0 + col * SLEEP_STEP,
                 126 + row * (SLEEP_CH + 6), SLEEP_CW, SLEEP_CH,
                 12, F_BODY, mins[i]);
    }
    s_grp_sleep.on_change = ev_sleep_pick;
    /* 恢复上次设定：NVS 里存的是分钟数，要映射回胶囊下标。
     * ★ 用 s_sleep_mins 而不是 atoi_minutes(mins[i])：
     *   文案里已经没有数字了（只有「关闭」有中文），反查不回来；
     *   直接按下标查表，方向单一、不易错。*/
    {
        int sv = app_sleep_minutes();
        int sel = 0;                       /* 默认「关闭」 */
        for (int i = 1; i < SLEEP_N; i++) {
            if (sv == s_sleep_mins[i]) { sel = i; break; }
        }
        s_grp_sleep.sel = sel;
    }
    grp_apply(&s_grp_sleep);

    /* 剩余时间：给一个独立 label，每秒刷。
     * ★ 为什么单独一行而不是塞进胶囊里：胶囊是「设定值」，
     *   这一行是「还剩多久」，两件事混在一颗胶囊里会看不清。*/
    s_lbl_sleep_left = label(p, 20, 214, F_BODY, C_AMBER, "");

    /* ---- 亮度 ----
     * ★ 10-04 第二十七次整页重排：两行胶囊（126~208）占掉了上一版
     *   「自定义三颗」那排的位置，剩余时间和亮度区随之整体下移。
     *
     *   ★★ 纵向账（CONT_H = 320 － TOPBAR_H 38 = **282**，可用 y 0~282）：
     *       10  时钟（F_BIG 38px → 10~48）
     *       54  日期（F_TINY → 54~68）
     *       74  状态（F_BODY → 74~92）
     *       94  波浪分隔线（v1.35：只剩 1 条线，40 根竖条已砍）
     *      108  「睡眠定时」小标题（F_TINY → 108~120）
     *      126  胶囊行一 关闭/30/60/90（高 38 → 126~164）
     *      170  胶囊行二 120/150/180/240（高 38 → 170~208）
     *      214  剩余时间（F_BODY → 214~232）
     *      238  亮度小标题（F_TINY → 238~252）
     *      256  滑块（高 18 → 256~274）
     *       底部余量 8 px
     *   ⚠️ 这一页到底了（两版前余量还有 36 px）。以后想加任何一行之前，
     *      先回来看这张表；「先挤一挤」最多一次，第二次就得砍东西或分页。
     *   ⚠️ 横向账一律用 CONT_W(424) 不是 480，见上面 SLEEP_X0 那段。
     *   ⚠️ 底部那行「暖白去蓝光 · 触摸屏幕恢复 · 拖动滑块调亮度」早就删了：
     *      纵向没位置，而它说的三件事里
     *      「拖动滑块」正在滑块上方看得见、
     *      「暖白去蓝光」是硬件特性（背光本身就没有蓝光）、
     *      「触摸屏幕恢复」指的是息屏唤醒，那是全局行为不是本页功能。*/
    s_lbl_bright = label(p, 20, 238, F_TINY, lv_color_hex(0x8C8471),
                         "Brightness 85%");

    lv_obj_t *sl = lv_slider_create(p);
    lv_obj_set_pos(sl, 20, 256);
    lv_obj_set_size(sl, 384, 18);
    lv_slider_set_range(sl, 5, 100);
    lv_slider_set_value(sl, app_sys_get_brightness(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, lv_color_hex(0x1B1811), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sl, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(sl, 9, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, C_AMBER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sl, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(sl, 9, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, C_WARM, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sl, 4, LV_PART_KNOB);
lv_obj_add_event_cb(sl, ev_bright_changed, LV_EVENT_VALUE_CHANGED, NULL);
}

/* ---- 夜间页的按需建 / 离开就删（v1.35）----
 * 与第 8 页 WiFi 设置同一套机制，理由也一样：这一页最重，
 * 常驻会把 LVGL 池抽干，进页面时崩在 lv_draw_add_task。
 *
 * ⚠️ 销毁时【必须把句柄全清 NULL】，否则三个 1 秒定时器
 *   （clock_cb / clock_date_cb / night_state_cb / sleep_timer_cb）
 *   下一拍还会往已释放的句柄上写 —— 那是 use-after-free，
 *   症状是「切走几秒后随机崩」，比现在难查十倍。
 *   这些定时器本来就有 if (s_lbl_xxx) 判空（见各自定义），
 *   清 NULL 就自动变成「不刷」。*/
static void night_page_destroy(void)
{
    if (!s_night_built) return;
    s_night_built = false;
    s_lbl_clock = NULL;
    s_lbl_date = NULL;
    s_lbl_night_state = NULL;
    s_lbl_sleep_left = NULL;
    s_lbl_bright = NULL;
    memset(&s_grp_sleep, 0, sizeof(s_grp_sleep));
    lv_obj_clean(s_pages[3]);
    /* 清理 grp 的点击参数池（grp_clicked 的 user_data 指向它，
     * 页面删了这些参数也作废，重建时由 arg_of() 重新分配）。*/
    s_arg_n = 0;
}

static void night_page_ensure(void)
{
    if (s_night_built) return;
    page_night_build(s_pages[3]);
    s_night_built = true;
    /* 建完立刻刷一次：不然要等下一个整秒才显示真实时间 */
    clock_cb(NULL);
    clock_date_cb(NULL);
    sleep_left_cb(NULL);
}

/* ============================================================
 *  页面五：系统状态（关于 + 体检 + WiFi）
 * ============================================================ */
#define STAT_ROWS 7      /* ★ 10-04：6 → 7，新增「电池」行。纵向账见 stat_key 上方 */

static lv_obj_t *s_lbl_stat_key[STAT_ROWS];
static lv_obj_t *s_lbl_stat_val[STAT_ROWS];
static lv_obj_t *s_lbl_scan;

static const char *stat_key[STAT_ROWS] = {
    "Firmware", "Uptime", "Memory", "Audio", "Storage", "Net", "Battery"
};

/* ---- 状态页纵向账（10-04 加了第 7 行「电池」后重算）----
 *   标题 18px → 38；「重新读取」那行到 40；
 *   7 行键值：y = 40 + i*24，行高 22 ⇒ 最后一行到 40+144+22 = 206；
 *   三个动作按钮从 210 起 ⇒ 还剩 4 px 余量。
 *   ⚠️ 原来是 6 行 / y=42 + i*27 / 行高 24（到 201）。加第 7 行时
 *   试过把行距压到 24：40+144+24 = 208 > 206（会压到按钮），
 *   压到 23 又让 22 高的行几乎贴在一起 ⇒ 最后取【行高 22 / 行距 24】。
 *   ⚠️ 行高 22 配 F_TINY(11px) 时上下各留 5.5 px，视觉上比原来紧一点，
 *   但不裁字。若以后再加第 8 行，必须重算这一段而不是接着往下堆。*/

/* 顶栏最右的 WiFi 标志 + 内存卡标志。
 * ★ 必须每页都刷、每秒都刷 —— 它不在「状态页」里，而是全局可见的指示灯。
 *   所以不能用 stat_refresh()（那个只在第 5 页跑），单独一个轻量函数。
 *
 * ★ 10-03 第九次：只改颜色，不改文案（兰兰要的是「绿色=已连，没标志=没连」，
 *   绿色 WiFi 图标本身就够表意，再写「已连」两个字是冗余）。*/
static void wifi_indicator_refresh(void)
{
    if (s_ic_wifi) {
        switch (app_net_link_state()) {
        case APP_NET_OK:
            lv_obj_set_style_text_color(s_ic_wifi, C_GREEN, 0);
            break;
        case APP_NET_CONNECTING:
            lv_obj_set_style_text_color(s_ic_wifi, C_AMBER, 0);
            break;
        case APP_NET_FAIL:
            lv_obj_set_style_text_color(s_ic_wifi, lv_color_hex(0xF87171), 0);
            break;
        default:
            lv_obj_set_style_text_color(s_ic_wifi, lv_color_hex(0x3A424C), 0);
            break;
        }
    }
    /* 内存卡：没挂载就整个变暗（图标 + 文字一起）*/
    bool has = app_sd_is_mounted();
    if (s_ic_sd) {
        lv_obj_set_style_text_color(s_ic_sd, has ? C_GREEN : lv_color_hex(0x3A424C), 0);
    }
    if (s_lbl_sdinfo) {
        if (has) {
            uint32_t cap_mb = 0;
            const char *fs = NULL;
            app_sd_get_info(&cap_mb, &fs);
            /* 顶栏只有 38 px 高，写不下「29.1 GB」+ 文件系统名，只写容量 */
            lv_label_set_text_fmt(s_lbl_sdinfo, "%uG", (unsigned)((cap_mb + 512) / 1024));
            /* ★ 跟 SD 图标同色（绿）：兰兰 10-03 第三次反馈
             *   「保留的那个距离内存卡标志太远，而且不是绿色字体」*/
            lv_obj_set_style_text_color(s_lbl_sdinfo, C_GREEN, 0);
        } else {
            lv_label_set_text(s_lbl_sdinfo, "No card");
            lv_obj_set_style_text_color(s_lbl_sdinfo, lv_color_hex(0x3A424C), 0);
        }
    }
}

/* 顶栏电池图标（10-04 新增）。
 * ★ 为什么只放图标不放数字：横账算过了，时钟右端 330、SD 图标 392、
 *   WiFi 450，中间只有 374~388 这一格 14 px —— 塞不下「85」两个字。
 *   ⇒ 顶栏用【颜色】表示档位，百分比数字放状态页与夜间页那两行整宽处。
 *
 * ★ 为什么不能用 s_batt_shown_pct 之外的方式去重：
 *   app_pwr_tick() 5 秒才真读一次 ADC，而本函数每秒都跑；
 *   不比对就每秒重设文字颜色 = 每秒标脏一块区域触发重绘。*/
static void batt_indicator_refresh(void)
{
    if (!s_ic_batt) return;

    int pct = app_pwr_batt_pct();
    bool present = app_pwr_batt_present();

    /* 档位（-1 = 无电池，0~3 = 满/中/低/极低）*/
    int step = -1;
    if (present) {
        if (pct > 50)      step = 0;
        else if (pct > 20) step = 1;
        else if (pct > 5)  step = 2;
        else               step = 3;
    }
    if (step == s_batt_shown_pct) return;
    s_batt_shown_pct = step;

    /* ⚠️ 符号名【必须去 lv_symbol_def.h 查】，别按"看起来该叫什么"拼 ——
     *   我写 LV_SYMBOL_BATTERY_3_BARS / _1_BAR 全是错的，真名是
     *   LV_SYMBOL_BATTERY_3 / _2 / _1（没有 _BARS / _BAR 后缀）。*/
    const char *icon;
    lv_color_t col;
    switch (step) {
    case 0:  icon = LV_SYMBOL_BATTERY_FULL;   col = C_GREEN;                break;
    case 1:  icon = LV_SYMBOL_BATTERY_3;      col = C_AMBER;                break;
    case 2:  icon = LV_SYMBOL_BATTERY_2;      col = lv_color_hex(0xF87171); break;
    case 3:  icon = LV_SYMBOL_BATTERY_1;      col = lv_color_hex(0xF87171); break;
    default: icon = LV_SYMBOL_BATTERY_EMPTY;  col = lv_color_hex(0x3A424C); break;
    }
    lv_label_set_text(s_ic_batt, icon);
    lv_obj_set_style_text_color(s_ic_batt, col, 0);

    /* 同步状态灯。★ 优先级：低电 > 播放中 > 连接中 > 已联网。
     *   低电必须压过其它状态 —— 电快没了的时候，
     *   不管在放什么，蓝绿色的「一切正常」都是误导。
     *   这里是本文件里【唯一】写 app_led_set_mode 的地方，
     *   别在别的函数里也写，否则两处打架、最后谁赢看调用顺序。*/
    if (step == 3) {
        app_led_set_mode(APP_LED_LOW_BATT);
    } else if (app_radio_is_playing() && !app_radio_is_paused()) {
        app_led_set_mode(APP_LED_PLAYING);
    } else {
        switch (app_net_link_state()) {
        case APP_NET_OK:         app_led_set_mode(APP_LED_WIFI);    break;
        case APP_NET_CONNECTING: app_led_set_mode(APP_LED_LINKING); break;
        default:                 app_led_set_mode(APP_LED_OFF);     break;
        }
    }
}

static void stat_refresh(void)
{
    app_status_t st;
    app_sys_get(&st);

    uint32_t up = st.uptime_s;

    if (s_lbl_stat_val[0]) {
        lv_label_set_text_fmt(s_lbl_stat_val[0], "%s · %s", st.fw_ver, st.build_date);
    }
    if (s_lbl_stat_val[1]) {
        lv_label_set_text_fmt(s_lbl_stat_val[1], "%02u:%02u:%02u",
                              (unsigned)(up / 3600), (unsigned)((up / 60) % 60),
                              (unsigned)(up % 60));
    }
    if (s_lbl_stat_val[2]) {
        lv_label_set_text_fmt(s_lbl_stat_val[2], "Internal %u KB / PSRAM %u MB",
                              (unsigned)st.heap_internal_kb,
                              (unsigned)(st.heap_psram_kb / 1024));
    }
    if (s_lbl_stat_val[3]) {
        lv_label_set_text_fmt(s_lbl_stat_val[3], "ES8311 %s - vol %d%%",
                              st.audio_ready ? "Ready" : "Not ready", st.volume);
    }
    if (s_lbl_stat_val[4]) {
        if (st.sd_mounted) {
            lv_label_set_text_fmt(s_lbl_stat_val[4], "TF %u MB - %d tracks",
                                  (unsigned)st.sd_cap_mb, st.sd_audio_count);
        } else {
            lv_label_set_text(s_lbl_stat_val[4], "No SD card");
        }
    }
    /* ★★ 10-04：电量行。顶栏只放得下 14 px 的图标（横账见 topbar_build），
     *   百分比数字只能放这里这一行整宽的位置。
     *   「无电池」和「0%」是两件事，必须分开写：
     *     无电池 = 现在插着 USB 供电；
     *     0%     = 插了电池但真的没电了。*/
    /* ★★ 10-04 第二次改：电量行做成【可点】，用来声明电池在不在。
     *   为什么必须让人点：原理图确认 TP4054 的 CE 脚硬接 GND = 常使能，
     *   没插电池时它照样往 BAT 线灌电流，把读数顶到 4.2V 附近
     *   ⇒ 兰兰实测「没插电池却显示 94% · 4.15 V」。
     *   而满电真电池 4.20V 与空载 4.15V 差距比 ADC 噪声还小，
     *   CHRG1 又没引到 MCU ⇒ 硬件上没有判据，只能让人自己声明。
     *   点一下就切「已接/未接」，不写 NVS（电池是插拔动作）。
     *   「未接」时这一行提示怎么点回来，别让兰兰以为自己点坏了。*/
    if (s_lbl_stat_val[6]) {
        if (app_pwr_batt_present()) {
            int mv = app_pwr_batt_mv();
            lv_label_set_text_fmt(s_lbl_stat_val[6], "Battery %d%% - %d.%02d V",
                                  app_pwr_batt_pct(), mv / 1000, (mv % 1000) / 10);
        } else {
            lv_label_set_text(s_lbl_stat_val[6], "No battery");
        }
        lv_obj_set_style_text_color(s_lbl_stat_val[6], C_TEXT, 0);
    }
    if (s_lbl_stat_val[5]) {
        if (app_net_scan_running()) {
            lv_label_set_text_fmt(s_lbl_stat_val[5], "Scanning... (%d last time)", st.ap_count);
        } else {
            /* ★ 按「链路状态」分支，不要只判断字符串。
             *   「连接中」只在真的还在重试时出现；重试用完必须变成「连不上 + 原因」，
             *   否则会永远停在一句「连接中」，看着像卡住。*/
            switch (app_net_link_state()) {
            case APP_NET_OK:
                lv_label_set_text_fmt(s_lbl_stat_val[5], "%s · %s · %d dBm",
                                      st.net_ip, st.net_ssid, st.rssi);
                break;
            case APP_NET_CONNECTING:
                lv_label_set_text_fmt(s_lbl_stat_val[5], "Connecting to %s", st.net_ssid);
                break;
            case APP_NET_FAIL:
                lv_label_set_text_fmt(s_lbl_stat_val[5], "Cannot connect - %s - %s",
                                      st.net_ssid,
                                      app_net_reason_short(app_net_last_reason()));
                break;
            case APP_NET_NO_CRED:
                lv_label_set_text(s_lbl_stat_val[5], "Not configured - put wifi.txt on the SD card");
                break;
            default:
                lv_label_set_text(s_lbl_stat_val[5], "Not initialized");
                break;
            }
        }
    }

    /* ⚠️ 顶栏时钟【不在这里刷】—— 10-04 兰兰报「首页标签栏时间没校准，差 4 分钟」。
     *   真因不是没对上时：是这个函数只在状态页被调用
     *   （stat_timer_cb 里 `if (s_cur_page == 4) stat_refresh();`），
     *   顶栏那个 label 在别的页上从建好起就没再被写过，
     *   一直停在上次进状态页那一刻的值 ⇒ 在状态页待过之后玩 4 分钟，就正好差 4 分钟。
     *   ★ 教训（和速查卡铁律 20 同源）：【定时器回调里写 label，
     *     必须确认那个回调在所有页面都会跑】，否则就是一个「建好就死」的死 label。
     *   现在统一搬到 clock_cb：1 秒一次、全页面生效，那里是唯一真相源。*/

    /* 扫描结束后把结果打到串口（屏幕放不下，串口可查） */
    static int last_aps = -1;
    if (!app_net_scan_running() && st.ap_count > 0 && st.ap_count != last_aps) {
        last_aps = st.ap_count;
        for (int i = 0; i < st.ap_count; i++) {
            char nm[40];
            int rssi = 0;
            if (app_net_scan_get(i, nm, sizeof(nm), &rssi)) {
                ESP_LOGI(TAG, "AP[%d] %s  %d dBm", i, nm, rssi);
            }
        }
    }
}

static void ev_scan_wifi(lv_event_t *e)
{
    (void)e;
    tap_note("Scan WiFi");
    app_net_scan_start();
    if (s_lbl_scan) lv_label_set_text(s_lbl_scan, "Scanning...");
}

/* ★★ 10-04 第二次改：点「电池」那一行 = 声明「我插了 / 没插电池」。
 *
 *  为什么必须有这个（兰兰实测「没插电池却显示 94% · 4.15 V」）：
 *    原理图确认 **TP4054 的 CE 脚硬接 GND = 充电常使能**，
 *    没插电池时它照样往 BAT 线灌电流、把读数顶到 4.2V 附近，
 *    而满电真电池也是 4.20V —— **靠电压区分不了**，
 *    CHRG1 脚又没引到 MCU ⇒ 硬件上没有判据，只能让人自己说。
 *
 *  ★ 为什么不写 NVS：电池是插拔动作。固化到 flash 会出现
 *    「换了电池还记着上一块」的怪事，重启一次就恢复更符合直觉。
 *
 *  ⚠️ 纵向账：这一行不加任何新对象，复用已有的 label 当点击热区
 *     （池只剩 13.7 KB，used 87%，每个带样式对象 ≈ 350 B）。*/
static void ev_batt_toggle(lv_event_t *e)
{
    (void)e;
    bool now = !app_pwr_batt_present();
    app_pwr_batt_set_present(now);
    tap_note(now ? "Battery connected" : "No battery");
    /* 顶栏图标也走同一个判据，这里主动刷一次，别等下一秒的定时器 */
    s_batt_shown_pct = -2;
    batt_indicator_refresh();
    stat_refresh();
}

/* 从 SD 卡的 wifi.txt 重新连接。
 * ★ 有了这个按钮，换 WiFi / 首次往卡里放文件之后都不用重启机器。*/
static void ev_connect_wifi(lv_event_t *e)
{
    (void)e;
    tap_note("Connect wifi.txt");
    esp_err_t r = app_net_connect_from_file("/sdcard/wifi.txt");

    if (s_lbl_stat_val[5]) {
        if (r == ESP_OK) {
            lv_label_set_text(s_lbl_stat_val[5], "Connection started...");
        } else if (r == ESP_ERR_NOT_FOUND) {
            lv_label_set_text(s_lbl_stat_val[5], "No wifi.txt in the SD card root");
        } else if (r == ESP_ERR_INVALID_ARG) {
            lv_label_set_text(s_lbl_stat_val[5], "First line of wifi.txt is empty");
        } else {
            lv_label_set_text_fmt(s_lbl_stat_val[5], "Connection failed (%s)", esp_err_to_name(r));
        }
    }
    ESP_LOGI(TAG, "connect_from_file -> %s", esp_err_to_name(r));
}

static void ev_refresh_stat(lv_event_t *e)
{
    (void)e;
    tap_note("Refresh");
    stat_refresh();
}

static void page_about_build(lv_obj_t *p)
{
    label(p, 8, 6, F_H2, C_TEXT, "System status");

    lv_obj_t *rf = tick(p, 328, 4, 88, 28, C_HILITE, 14, NULL);
    lv_obj_add_event_cb(rf, ev_refresh_stat, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rfl = label(rf, 0, 0, F_TINY, C_TEXT, "Reload");
    lv_obj_center(rfl);

    /* 7 行键值。★ 10-04：y 从 42+i*27 改为 40+i*24、行高 24→22，
     *   否则加第 7 行会一路顶到 228，直接压到 y=210 的按钮上。
     *   完整纵向账见 stat_key 上面那段注释。
     *
     * ★★ 10-04 烧录后崩过一次（EXCVADDR=0x08 @ lv_draw_add_task，
     *   池 free=13636 B used=87%），原因就是这一圈加了 3 个带样式的对象。
     *   ⇒ **只有前 6 行画底 box，第 7 行（电池）不画**：
     *     省下一个 box（含 local style ≈ 350 B），
     *     而这一行是电池（状态页最不常看的一行），视觉上少个底框无感。
     *   ★★ 这是速查卡铁律「加 UI 前先算 lv_obj 数」的第四次应验：
     *     基线 already 87%，**每加一个带样式的对象都在吃崩溃额度**。
     *     下次再加行，请先想「哪一行可以不画底框」。*/
    for (int i = 0; i < STAT_ROWS; i++) {
        int y = 40 + i * 24;
        /* 第 7 行（i==6，电池）不画底 box —— 见上面说明 */
        if (i < 6) box(p, 8, y, 408, 22, C_PANEL, 6);
        s_lbl_stat_key[i] = label(p, 18, y + 4, F_TINY,
                                  i < 6 ? C_MUTED : C_AMBER, stat_key[i]);
        s_lbl_stat_val[i] = label(p, 76, y + 4, F_TINY, C_TEXT, "…");
        lv_obj_set_width(s_lbl_stat_val[i], 332);
        /* ★ 第 7 行（电池）挂点击 = 声明电池在不在。
         *   硬件判据不存在（TP4054 的 CE 常使能，空载也 4.15V），
         *   只能让人点。热区就用这个 label，**不加新对象**。*/
        if (i == 6) {
            lv_obj_add_flag(s_lbl_stat_val[i], LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(s_lbl_stat_val[i], ev_batt_toggle,
                                LV_EVENT_CLICKED, NULL);
        }
    }

    /* 三个动作按钮 */
    lv_obj_t *bs = tick(p, 8, 210, 130, 40, C_HILITE, 12, NULL);
    lv_obj_add_event_cb(bs, ev_scan_wifi, LV_EVENT_CLICKED, NULL);
    s_lbl_scan = label(bs, 0, 0, F_TINY, C_TEXT, "Scan nearby WiFi");
    lv_obj_center(s_lbl_scan);

    /* 连 wifi.txt：验证阶段的连网入口（发行版会换成扫描列表 + 屏幕键盘）*/
    lv_obj_t *bc = tick(p, 146, 210, 130, 40, lv_color_hex(0x1B2A3A), 12, NULL);
    lv_obj_add_event_cb(bc, ev_connect_wifi, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bcl = label(bc, 0, 0, F_TINY, C_CYAN, "Connect wifi.txt");
    lv_obj_center(bcl);

    lv_obj_t *bb = tick(p, 284, 210, 132, 40, lv_color_hex(0x1B3A31), 12, NULL);
    lv_obj_add_event_cb(bb, ev_beep, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bbl = label(bb, 0, 0, F_TINY, C_GREEN, "Chime");
    lv_obj_center(bbl);

    /* ★★ 第二十二次新增「WiFi 设置」入口（第 8 页），第二十四次改了它的用途：
     *   兰兰否决了屏幕键盘（「又要键盘操作也不方便」），改成手机网页配网。
     *   所以这个键现在通到「开热点 → 手机连 → 自动弹网页」那一页。*/
    lv_obj_t *bw = tick(p, 8, 254, 130, 26, lv_color_hex(0x1B3A31), 10, NULL);
    lv_obj_add_event_cb(bw, ev_open_wifi_page, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bwl = label(bw, 0, 0, F_TINY, C_GREEN, "Setup via phone >");
    lv_obj_center(bwl);

    /* ★ 版本行改用 XS_FW_VERSION（app_version.h）。
     *   ★ 兰兰：「系统状态页固件的版本号要建立」——
     *     原来这里写死了「v0.1」，跟实际迭代完全脱节，排查时无法判断
     *     板上跑的是哪一版。现在与 app_sys.h 里那份同源。
     *   ★★ 10-04 兰兰：「状态页底部署名不用」⇒ **删掉「· © 永远的兰兰」**。
     *     署名在开机页和顶栏都有（那两处才是推广位），状态页是排查用的，
     *     只需要版本号 + 日期。删掉后文字从 ~154 px 缩到 ~70 px。*/
    /* ⚠️ label() 只收 6 个参数、不做格式化，要带 %s 得自己先建 label 再
     *   lv_label_set_text_fmt —— 直接 label(p, x,y,font,color,"v%s…",a,b)
     *   会被判「too many arguments」。*/
    lv_obj_t *vrow = label(p, 148, 258, F_TINY, lv_color_hex(0x5A6672), "");
    lv_label_set_text_fmt(vrow, "v%s %s", XS_FW_VERSION + 1, XS_FW_DATE);
    lv_obj_set_width(vrow, 128);
    /* CLIP 而不是默认的 WRAP：这行只有 26 px 高，放不下第二行，
     * WRAP 会把字折下去、跑到按钮那行上。*/
    lv_label_set_long_mode(vrow, LV_LABEL_LONG_MODE_CLIP);

    /* ---- 关机键（10-04 新增，同日第二次修）----
     * ★★ 兰兰报「关机键挡住了下面的字，露出部分的兰兰」。
     *   **根因不是坐标算错半格，是加按钮时没把这一行已有的元素列进来算账**：
     *   底部 y=254 那行原本已经有「手机配网 →」(x=8) 和版本/署名 vrow (x=146)，
     *   我按「三键 8 / 146 / 284」的旧账把关机填进 146 ⇒ 正好压住 vrow，
     *   只剩右尾几个字露在按钮外面。
     *   ⇒ 修法：删掉状态页的署名（兰兰：「状态页底部署名不用」），
     *     版本行从 ~154 px 缩到 ~70 px，关机键放到 284 那格。
     *
     * ★★ 底部横向账（CONT_W = 424，写死坐标前必须把这一行【所有】元素列全）：
     *     8   + 130   手机配网 →            到 138
     *     148 + 128   版本号 v1.33 10-04    到 276（文字 ~70 px，居左）
     *     284 + 132   关机                  到 416
     *   间距 10，两端各留 8，右缘余 8 ✓
     *   ⚠️ **先量文字实际宽度再定 x，别照旧账往格子里填** ——
     *      教训：照旧账填 146 就压住了同一行的 vrow。
     *
     * ⚠️ 这里【必须】是 tick(按钮) + 独立 label 两个对象，不能省掉 label：
     *   我一度想「把文字直接挂按钮上省一个对象」（池只剩 13.6 KB，很想省），
     *   但 tick() 建的是 **box**（见它的实现：box() + 挂三个事件回调），
     *   给 box 调 lv_label_set_text() 会按 label 的布局结构去解释一个 box
     *   的指针 ⇒ LoadProhibited 崩在 set_text_internal → lv_free_core
     *   （EXCVADDR=0x08）。**这就是速查卡那条「别靠对象类型猜」的教训，
     *   只不过这次是我自己差点犯。** 省 350 B 不值得换一次必崩。*/
    s_btn_power = tick(p, 284, 254, 132, 26, lv_color_hex(0x2A1416), 10, NULL);
    lv_obj_add_event_cb(s_btn_power, ev_power, LV_EVENT_CLICKED, NULL);
    s_lbl_power = label(s_btn_power, 0, 0, F_TINY, lv_color_hex(0xF87171), "Power off");
    lv_obj_center(s_lbl_power);
}

/* ============================================================
 *  第 8 页（索引 7）：WiFi 配网
 * ============================================================
 *  ★★★ 2026-10-04 第二十四次：整页重做，屏幕自建键盘【全部删掉】。
 *
 *  兰兰原话：「不用手动输入 WiFi 又要键盘操作也不方便，
 *            还是市场上最主流的方式吧，比如扫码链接或者你觉得什么连接最好」
 *
 *  方案（原理见 app_prov.h）：设备开热点 → 手机连 → 系统自动弹网页
 *  → 网页里下拉选 SSID + 填密码 → 设备连上并存 NVS。
 *  屏幕这一页只管三件事：
 *     ① 告诉你「连哪个热点」
 *     ② 告诉你「网页没自动弹就手动开这个地址」
 *     ③ 显示连接结果
 *
 *  ★ 顺带把「点附近热点会重启」那条路径整个删掉（AP 列表不再存在）。
 *    那个崩溃我到最后也没抓到现场（两轮修复无效、串口监听 600 秒 0 字节），
 *    与其继续赌机制，不如把会崩的路拿掉 —— 现在这一页只有 9 个静态
 *    label + 3 个按钮，没有列表、没有滚动、没有动态建删。
 * ============================================================ */

/* 三个动作键的横向账：CONT_W = 424
 *   8 + 136*3 + 4*2 = 424  ⇒  x = 8 / 148 / 288，宽 136，最后一个到 424 ✓ */
#define PW_BTN_W     136
#define PW_BTN_H      30
#define PW_BTN_Y     214
#define PW_BTN_X0       8
#define PW_BTN_GAP      4

/* 纵向账（页内坐标，页高 282）：
 *   2   标题（18px）           → 20
 *   30  状态行（14px）         → 48
 *   76  步骤 1（11px）         → 90
 *   96  热点名（38px）         → 134
 *   136 步骤 2（11px）         → 150
 *   152 步骤 3（11px）         → 166
 *   178 提示（11px）           → 192
 *   142 → 这一行是「手动地址」，上面 136/152 两行挪不开，改用它当提示行
 *   实际排布见下面每行的 y。*/
#define PW_STEP1_Y    76
#define PW_SSID_Y     96
#define PW_STEP2_Y   138
#define PW_STEP3_Y   154
#define PW_URL_Y     174
#define PW_TIP_Y     200

static lv_obj_t *s_pw_stat;        /* 状态行（五态 + 配网进展）*/
static lv_obj_t *s_pw_step1;       /* 第 1 步：连热点 */
static lv_obj_t *s_pw_ssid;        /* 热点名（大字，绿色）*/
static lv_obj_t *s_pw_step2;       /* 第 2 步 */
static lv_obj_t *s_pw_step3;       /* 第 3 步 / 结果 */
static lv_obj_t *s_pw_url;         /* 192.168.4.1（大字，青色）*/
static lv_obj_t *s_pw_tip;         /* 兜底提示 */
static lv_obj_t *s_pw_b1;          /* 开始配网 / 已开则变「关掉」 */
static lv_obj_t *s_pw_b2;          /* 重新扫描 */
static lv_obj_t *s_pw_b3;          /* 连 wifi.txt */

/* 前向声明：handler 的定义在挂上去之后（这一段整体替换自文件后段）*/
static void wifi_page_refresh(void);
static void ev_pw_start(lv_event_t *e);
static void ev_pw_scan(lv_event_t *e);
static void ev_pw_back(lv_event_t *e);

/* 内存账：这一页对象少，但仍然每步打一次 —— 万一以后又往这页加东西，
 * 有这行日志才知道该不该加（iron rule 20：别猜，看数字）。*/
static void wifi_mem_note(const char *when)
{
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    ESP_LOGI(TAG, "wifi mem @%-10s free=%6u B  used=%2u%%  peak=%u B",
             when, (unsigned)m.free_size, (unsigned)m.used_pct,
             (unsigned)m.max_used);
}

/* 三个动作键 + 返回键共 4 份共享样式（第二十三次的教训：绝不用
 * lv_obj_set_style_*，那会给每个对象各 malloc 一份 local style）。*/
typedef struct {
    lv_style_t st;
    bool       inited;
} pw_style_t;
static pw_style_t s_pws_btn[3];    /* 绿 / 青 / 灰 */
static pw_style_t s_pws_back;      /* 返回键：灰字 */

static void pw_style_init(pw_style_t *s, lv_color_t fg)
{
    if (s->inited) return;
    s->inited = true;
    lv_style_init(&s->st);
    lv_style_set_bg_color(&s->st, C_HILITE);
    lv_style_set_bg_opa(&s->st, LV_OPA_COVER);
    lv_style_set_radius(&s->st, 5);
    lv_style_set_pad_ver(&s->st, 0);
    lv_style_set_text_font(&s->st, F_TINY);
    lv_style_set_text_color(&s->st, fg);
    lv_style_set_text_align(&s->st, LV_TEXT_ALIGN_CENTER);
}

static void pw_styles_init(void)
{
    /* ⚠️ 颜色表不能写成 `static const lv_color_t t[] = { C_GREEN, ... }`：
     *   C_* 是 lv_color_hex(0x…) 宏，展开成函数调用（对结构体逐字段赋值），
     *   而 C 的静态初始化要求常量表达式 ⇒ gcc 报
     *   `initializer element is not constant`，且报错行指向宏定义那几行，
     *   离真正出错点几百行（第二十三次踩过）。所以逐个赋值。*/
    pw_style_init(&s_pws_btn[0], C_GREEN);
    pw_style_init(&s_pws_btn[1], C_CYAN);
    pw_style_init(&s_pws_btn[2], C_MUTED);
    pw_style_init(&s_pws_back,  C_MUTED);
}

/* 一个可点的 label 当按钮（1 个对象顶 box+label 的 2 个）*/
static lv_obj_t *pw_btn(lv_obj_t *parent, int x, int y, int w, int h,
                        const char *txt, const lv_style_t *st,
                        lv_event_cb_t cb)
{
    if (!parent) return NULL;
    lv_obj_t *l = lv_label_create(parent);
    if (!l) {
        ESP_LOGE(TAG, "pw_btn: 池满，建不出按钮");
        return NULL;
    }
    if (st) lv_obj_add_style(l, st, 0);
    lv_label_set_text(l, txt ? txt : "");
    lv_obj_set_pos(l, x, y);
    lv_obj_set_size(l, w, h);
    lv_obj_add_flag(l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(l, LV_DIR_NONE);
    lv_obj_add_event_cb(l, cb, LV_EVENT_PRESSED, NULL);
    return l;
}

/* ---- 三个动作键 ---- */
static void ev_pw_start(lv_event_t *e)
{
    (void)e;
    if (app_prov_is_active()) {
        app_prov_clear_result();
        app_prov_stop();
        tap_note("Stop setup");
    } else {
        esp_err_t r = app_prov_start();
        if (r == ESP_OK) {
            app_net_scan_start();   /* 网页里的下拉列表要用 */
            tap_note("Start setup");
        } else {
            char m[56];
            snprintf(m, sizeof(m), "Hotspot failed: %.36s", esp_err_to_name(r));
            tap_note(m);
        }
    }
    wifi_page_refresh();
    wifi_mem_note("after prov");
}

static void ev_pw_scan(lv_event_t *e)
{
    (void)e;
    tap_note("Rescan");
    app_net_scan_start();
    wifi_page_refresh();
}

static void ev_pw_back(lv_event_t *e)
{
    (void)e;
    tap_note("Back to status");
    goto_page(4);
}

static void wifi_page_refresh(void)
{
    if (!s_pw_stat) return;

    /* 扫描完按信号排序 + 同名去重（app_sys 里做，UI 只读）。
     * ⚠️ last_running 之前是函数级 static —— 重进页面时不重置，
     *   会拿上一轮的残留判「扫描是不是刚结束」，排序不执行、顺序是乱的。
     *   现在它是本页的 static，wifi_page_destroy 负责归零。*/
    static int last_running = 1;
    bool running = app_net_scan_running();
    if (last_running && !running) {
        app_net_scan_sort_by_rssi();
        last_running = 0;
    } else if (running) {
        last_running = 1;
    }

    app_status_t st;
    app_sys_get(&st);
    bool prov = app_prov_is_active();
    int  pr = app_prov_result();

    /* ---- 状态行：配网结果优先，其次五态 ---- */
    if (pr == 2) {
        lv_label_set_text_fmt(s_pw_stat, "Connected to %s - %s", st.net_ssid, st.net_ip);
    } else if (pr == 1) {
        lv_label_set_text(s_pw_stat, "Connecting to the network you chose...");
    } else if (pr == 3) {
        lv_label_set_text_fmt(s_pw_stat, "Not connected: %s",
                              app_net_reason_short(app_net_last_reason()));
    } else if (pr == 4) {
        lv_label_set_text(s_pw_stat, "Bad input from the page, please retry");
    } else if (running) {
        lv_label_set_text(s_pw_stat, "Scanning nearby networks... (about 3s)");
    } else {
        switch (app_net_link_state()) {
        case APP_NET_OK:
            lv_label_set_text_fmt(s_pw_stat, "Connected to %s - %s - %d dBm",
                                  st.net_ssid, st.net_ip, st.rssi);
            break;
        case APP_NET_CONNECTING:
            lv_label_set_text_fmt(s_pw_stat, "Connecting to %s...", st.net_ssid);
            break;
        case APP_NET_FAIL:
            lv_label_set_text_fmt(s_pw_stat, "Cannot connect to %s - %s",
                                  st.net_ssid,
                                  app_net_reason_short(app_net_last_reason()));
            break;
        case APP_NET_NO_CRED:
            lv_label_set_text(s_pw_stat, "Never connected - tap Start setup");
            break;
        default:
            lv_label_set_text(s_pw_stat, "Network not initialized");
            break;
        }
    }

    /* ---- 三个步骤行 ---- */
    if (s_pw_step1) {
        if (prov) {
            int c = app_prov_clients();
            if (c > 0) lv_label_set_text_fmt(s_pw_step1, "1. Phone connected to hotspot (%d devices)", c);
            else       lv_label_set_text(s_pw_step1, "1. Connect your phone to the hotspot below");
        } else {
            lv_label_set_text(s_pw_step1, "1. Tap Start setup, then connect your phone");
        }
    }
    if (s_pw_ssid) {
        if (prov) lv_label_set_text(s_pw_ssid, app_prov_ssid());
        else      lv_label_set_text(s_pw_ssid, "(hotspot not started)");
    }
    if (s_pw_step2) {
        lv_label_set_text(s_pw_step2, "2. A setup page will pop up in your browser");
    }
    if (s_pw_step3) {
        if      (pr == 2) lv_label_set_text(s_pw_step3, "3. Connected - you can close the page");
        else if (pr == 1) lv_label_set_text(s_pw_step3, "3. Connecting, please wait...");
        else if (pr == 3) lv_label_set_text(s_pw_step3, "3. Wrong password? Try another network");
        else              lv_label_set_text(s_pw_step3, "3. Pick WiFi, enter password, tap Connect");
    }
    if (s_pw_url) lv_label_set_text(s_pw_url, APP_PROV_IP_STR);
    if (s_pw_tip) {
        int n = st.ap_count;
        if (prov && n > 0)      lv_label_set_text_fmt(s_pw_tip, "%d networks found - pick in the page", n);
        else if (n > 0)         lv_label_set_text_fmt(s_pw_tip, "%d networks nearby", n);
        else                    lv_label_set_text(s_pw_tip, "Can still set up - type the name manually");
    }
    if (s_pw_b1) lv_label_set_text(s_pw_b1, prov ? "Stop setup" : "Start setup");
    if (s_pw_b2) lv_label_set_text(s_pw_b2, running ? "Scanning..." : "Rescan");
}

static void wifi_page_ensure(void)
{
    if (s_pw_stat) return;                    /* 已在页内 */
    lv_obj_t *p = s_pages[7];
    if (!p) return;
    pw_styles_init();

    /* 标题 + 返回 */
    label(p, 8, 2, F_H2, C_TEXT, "WiFi setup");
    (void)pw_btn(p, 336, 2, 80, 24, "< Back", &s_pws_back.st, ev_pw_back);

    /* 状态行 30..48 */
    s_pw_stat = label(p, 8, 30, F_BODY, C_GREEN, "…");
    lv_obj_set_width(s_pw_stat, 408);
    lv_label_set_long_mode(s_pw_stat, LV_LABEL_LONG_DOT);

    s_pw_step1 = label(p, 8, PW_STEP1_Y, F_TINY, C_MUTED, "");
    lv_obj_set_width(s_pw_step1, 408);
    lv_label_set_long_mode(s_pw_step1, LV_LABEL_LONG_DOT);

    /* 热点名用 38px 大字 —— 这是用户唯一需要照着打的东西，必须醒目 */
    s_pw_ssid  = label(p, 8, PW_SSID_Y, F_BIG, C_GREEN, "");
    lv_obj_set_width(s_pw_ssid, 408);
    lv_label_set_long_mode(s_pw_ssid, LV_LABEL_LONG_DOT);

    s_pw_step2 = label(p, 8, PW_STEP2_Y, F_TINY, C_MUTED, "");
    lv_obj_set_width(s_pw_step2, 408);
    s_pw_step3 = label(p, 8, PW_STEP3_Y, F_TINY, C_MUTED, "");
    lv_obj_set_width(s_pw_step3, 408);

    s_pw_url   = label(p, 8, PW_URL_Y, F_H2, C_CYAN, APP_PROV_IP_STR);
    lv_obj_set_width(s_pw_url, 220);

    s_pw_tip   = label(p, 8, PW_TIP_Y, F_TINY, C_MUTED, "");
    lv_obj_set_width(s_pw_tip, 408);
    lv_label_set_long_mode(s_pw_tip, LV_LABEL_LONG_DOT);

    /* 三个动作键（横向账见上面 PW_BTN_* 的注释） */
    s_pw_b1 = pw_btn(p, PW_BTN_X0, PW_BTN_Y, PW_BTN_W, PW_BTN_H,
                     "Start setup", &s_pws_btn[0].st, ev_pw_start);
    s_pw_b2 = pw_btn(p, PW_BTN_X0 + PW_BTN_W + PW_BTN_GAP, PW_BTN_Y,
                     PW_BTN_W, PW_BTN_H, "Rescan", &s_pws_btn[1].st, ev_pw_scan);
    s_pw_b3 = pw_btn(p, PW_BTN_X0 + (PW_BTN_W + PW_BTN_GAP) * 2, PW_BTN_Y,
                     PW_BTN_W, PW_BTN_H, "Connect wifi.txt", &s_pws_btn[2].st,
                     ev_connect_wifi);
    if (!s_pw_b1 || !s_pw_b2 || !s_pw_b3) {
        ESP_LOGE(TAG, "wifi 页按钮没建全（池满）");
    }
    wifi_mem_note("page built");
}

/* ★ 离开第 8 页就把整页对象删掉，把 LVGL 池还回去。
 *   挂在 goto_page 里而不是返回键里 —— 万一是从左边导航栏切走的，
 *   那样就漏掉了（第二十二次真机崩在这：常驻时池从 26.9 KB 掉到 9.5 KB）。*/
static void wifi_page_destroy(void)
{
    s_pw_stat = NULL;
    s_pw_step1 = s_pw_step2 = s_pw_step3 = NULL;
    s_pw_url = s_pw_tip = NULL;
    s_pw_ssid = NULL;
    s_pw_b1 = s_pw_b2 = s_pw_b3 = NULL;
    lv_obj_clean(s_pages[7]);
    wifi_mem_note("page gone");
}

/* 从状态页那个 WiFi 键进来 */
static void ev_open_wifi_page(lv_event_t *e)
{
    (void)e;
    tap_note("WiFi setup");
    /* ★ 这一页是【按需建、离开就删】的 */
    wifi_page_ensure();
    app_net_scan_start();      /* 进页就扫一次：网页里的下拉列表要用 */
    wifi_page_refresh();
    goto_page(7);
}


/* ============================================================
 *  定时器
 * ============================================================
 * ★★★ 10-04 第二十九次：splash 只能【撤一次】，而且必须把动画定时器一起带走。
 *
 * 症状（兰兰真机报）：「板子不停地发出启动的三声音符」，
 *   串口日志里 `splash done -> main` 每 3.5 秒重复一次。
 *
 * 真因是【同一个回调被两个定时器各建了一次】：
 *   splash_build() 里建了 3500 ms 的 s_splash_timer，
 *   ui_init() 后面又建了一个 2600 ms 的 s_splash_timer  ← 历史遗留
 * 后者把前者的【句柄覆盖】了，前者再也删不掉。
 * 而 splash_done_cb 里是 `lv_timer_del(s_splash_timer)`：
 *   第 2.6 秒那次删掉的是 2.6 s 那个（= 自己，正好收工）；
 *   3.5 s 那个再触发时 s_splash_timer 已是 NULL，`lv_timer_del(NULL)`
 *   在 v9 里是 no-op ⇒ 【它自己永远删不掉自己】⇒ 每 3.5 秒重复一次，
 *   每次都重新 lv_screen_load(主屏) + app_audio_beep_async()。
 *
 * ⇒ 修法三条：① ui_init 里那行多余的 timer 建法删掉（splash_build 已建）
 *             ② 回调加幂等标志 s_splash_done，第二次进来直接返回
 *             ③ 顺手把 60 ms 动画定时器也删掉 —— 它从来没被删过，
 *               而 splash 的子对象在 lv_screen_load 之后已经被释放，
 *               悬空指针每 60 ms 写一次样式就是「海报页卡顿」的来源。*/
static void splash_done_cb(lv_timer_t *t)
{
    (void)t;
    if (s_splash_done) return;          /* 幂等：谁来都不再重放一遍 */
    s_splash_done = true;

    if (s_splash_timer) { lv_timer_del(s_splash_timer); s_splash_timer = NULL; }
    /* ★ 60 ms 动画推送定时器：以前只在注释里说「splash_done_cb 会 del 它」，
     *   实际根本没删。splash 一撤，lv_screen_load(s_main_scr) 会连带释放
     *   splash 的子对象，s_spl_ring[] / s_spl_bar[] 立刻变悬空指针 ——
     *   动画回调还在跑，每 60 ms 往已释放的对象里写样式。*/
    if (s_spl_timer) { lv_timer_del(s_spl_timer); s_spl_timer = NULL; }
    for (int i = 0; i < SPL_RING_N; i++) s_spl_ring[i] = NULL;
    for (int i = 0; i < 5; i++)           s_spl_bar[i]  = NULL;
    s_spl_greet = NULL;

    lv_screen_load(s_main_scr);
    app_backlight_set(app_sys_get_brightness());
    ESP_LOGI(TAG, "splash done -> main");

    /* 开机提示音：不接音源也能证明「喇叭真的会响」*/
    app_audio_beep_async();
}

/* 状态刷新：只在对应页面可见时做，别的时候完全静音。
 * 例外：WiFi 标志是全局指示灯，每页每秒都要刷。*/

static void stat_timer_cb(lv_timer_t *t)
{
    (void)t;
    wifi_indicator_refresh();
    /* ★★ 10-04：电池图标。放在这里而不是某个页面的 refresh 里 ——
     *   顶栏是全局指示区，任何页面都要有。
     *   （同一个坑的第四次：顶栏时钟死 label、首页音乐库「未插卡」、
     *     夜间页假日期，都是「建好之后没人刷」。）*/
    batt_indicator_refresh();
    /* ★ 低电自动关机必须挂在【全页面每秒都跑】的这个回调里。
     *   app_pwr_tick() 内部自己按 5 秒节流，这里只是入口。*/
    app_pwr_tick();
    if (s_cur_page == 4) stat_refresh();
    /* ★ 第 7 页 WiFi 设置：每秒刷一次。
     *   扫描与连接都是异步的（app_sys 里跑独立任务），
     *   所以这一页靠「每秒读一次真实状态」来更新，
     *   与状态页那一行是同一套机制，只是页面不同。*/
    if (s_cur_page == 7) wifi_page_refresh();
    if (s_cur_page == 2) player_refresh();   /* 播放页：进度环 / 电平表按秒走 */

    /* 首页：跟着【真实播放状态】走，别停在旧数据上。
     * 两种会「不同步」的情形：
     *   ① 在播放页按了「下一台」→ 首页大卡还写着上一台；
     *   ② 电台流断了 / 起播失败 → 圆钮还写着「暂停」。
     * 只在真的变了才写 label，否则每秒重绘一次首页是白费。*/
    if (s_cur_page == 0) {
        int cur = app_radio_station_current();
        if (cur >= 0 && cur != s_hr_sel) {
            s_hr_sel = cur;
            home_update_card(cur);
        }
        if (s_playbtn_lbl) {
            bool run = app_radio_is_playing() && !app_radio_is_paused();
            const char *want = run ? "Pause" : "Playing";
            const char *have = lv_label_get_text(s_playbtn_lbl);
            if (have == NULL || strcmp(have, want) != 0) {
                lv_label_set_text(s_playbtn_lbl, want);
            }
        }
        /* ★★ 10-04 第二十九次：首页「音乐库」卡那行字要跟着 SD 挂载状态走。
         *  症状：兰兰真机报「首页音乐库显示未插内存卡」，可日志里明明是
         *    `XSSD: SD mounted at /sdcard  cap=29819 MB` + 音频数 337 —— 卡好好的。
         *  真因是【时序】：app_main() 里 ui_init() 在第 3 步、app_sd_mount() 在第 4 步，
         *    建首页那张卡时 SD 还没挂 ⇒ label 写下「未插内存卡」，
         *    而这个 label 之后谁也不刷（首页分支只刷播放按钮和台名）。
         *  ⇒ 挂载状态一变就刷一次那张卡。顺带把「拔卡」也管了。*/
        bool sd_now = app_sd_is_mounted();
        if (sd_now != s_last_sd_mounted) {
            s_last_sd_mounted = sd_now;
            home_cards_refresh();
        }
    }
}

/* ============================================================
 *  频谱专用快速定时器（100 ms）
 * ============================================================
 *  ★ 为什么不能挂在 stat_timer_cb（1 秒）里：频谱一秒才动一次，
 *    看着就是「一格一格跳」，不像频谱。100 ms 一次是流畅与 CPU 的平衡点
 *    —— 一次只是改 12~13 个 box 的 pos/size，不做布局计算，开销可以忽略。
 *  ★ 只在【频谱可见的页】才动手：不在播放页/首页时直接返回，
 *    省得给看不见的控件白改样式（LVGL 改样式会标脏区域，触发重绘）。*/
static void spec_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_cur_page == 2) {
        spec_refresh();
    } else if (s_cur_page == 0) {
        home_spec_refresh();
    }
}

/* ============================================================
 *  拖动专用定时器（20 ms）—— ★ 第二十二次从频谱定时器里拆出来
 * ============================================================
 * ★ 为什么必须独立：
 *   之前 seek_drag_poll() 挂在 100 ms 的 spec_timer_cb 上，
 *   而那个定时器只在播放页/首页干活 —— 于是在别的页面上拖动
 *   依然是 100 ms 一拍（虽然坐标本身已是 33 ms 粒度）。
 *   兰兰 10-04 报「拖动有作用了，但不灵敏」，这一拍就是主因。
 *
 * ★ 20 ms 而不是 10 ms：indev 读周期已经是 10 ms，
 *   轮询再快也只是重复读同一个坐标，纯烧 CPU。
 *   20 ms ⇒ 一秒 50 次，手指划过 244 px 的条每秒更新 50 px，够跟手。
 *
 * ★ 空闲时几乎零成本：第一行就 return，
 *   只有「手指真的按在条上」或「正在拖」才做后面的事。*/
static void seek_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (s_cur_page != 2) return;      /* 不在播放页就不管 */
    if (!app_radio_is_playing() && !s_seek_drag) return;
    seek_drag_poll();
}

/* ---- 取当前时刻（分钟）----
 * ★ 顶栏与夜间页共用一份：以前夜间页是从 23:47 起每秒自己 +1 的假时钟，
 *   顶栏也没时间。现在有 SNTP 就用真时间；还没校上时退回「开机后的小时数」，
 *   至少两个页面显示的是同一个东西，不会一个真一个假。*/
/* ---- 取当前时刻（分钟）；没对上时返回 -1 ----
 * ★ 为什么要 -1 而不是「退回开机后的小时数」：
 *   10-03 兰兰真机看到顶栏是「00:00」—— 那个 0 点其实是「开机 0 分钟」，
 *   长得跟真的凌晨一模一样，纯误导。宁可显示「--:--」，
 *   一眼就知道「时间还没对上，等 WiFi 连上」。*/
static int now_minutes(void)
{
    time_t t = time(NULL);
    if (t > 1700000000) {                 /* 2023-11 之后 = 真的对上了时 */
        struct tm tmv;
        localtime_r(&t, &tmv);
        return tmv.tm_hour * 60 + tmv.tm_min;
    }
return -1;
}

/* ---- 时钟：顶栏 + 夜间页大钟（10-04 合并）----
 * ★★★ 兰兰报「首页标签栏时间没校准，查 4 分钟，夜间页时间是正确的」。
 *   两个数用同一个 now_minutes()、同一份系统时间，函数上不可能差 4 分钟 ——
 *   差别在【谁在刷】：
 *     夜间页的钟在 clock_cb 里每秒刷（10-04 新加的），所以它对；
 *     顶栏的钟写在 stat_refresh() 里，而那个函数只有状态页会调
 *     （stat_timer_cb: `if (s_cur_page == 4)`），
 *     所以在首页/发现/播放/夜间页上它是【建好之后再没被写过】的死 label，
 *     显示的是上次进状态页那一刻的时间 ⇒ 「差 4 分钟」＝ 他在状态页之后玩了 4 分钟。
 *   ⇒ 现在顶栏也归 clock_cb 管，周期 1 秒（原来 60 秒，顶栏跳分钟看着发闷）。
 *   改口径必须同时改注释，否则下一个人会把它搬回去。*/
static void clock_cb(lv_timer_t *t)
{
    (void)t;
    int m = now_minutes();
    /* ⚠️ 缓冲给 16 而不是刚好 5+1：gcc 对 `%02d` 会按「int 全量范围」算最坏长度，
     *   声明 8 字节会被 -Werror=format-truncation 判可能截断（速查卡老坑）。
     *   这里 m 逻辑上最大 23*60+59，给足就不会误报。*/
    char buf[16];
    if (m < 0) snprintf(buf, sizeof(buf), "--:--");
    else       snprintf(buf, sizeof(buf), "%02d:%02d", m / 60, m % 60);

    if (s_lbl_clock)    lv_label_set_text(s_lbl_clock, buf);      /* 夜间页大钟 */
    if (s_lbl_topstat)  lv_label_set_text(s_lbl_topstat, buf);    /* 顶栏      */
}

/* ---- 夜间页：真日期 + 星期（10-04 新增）----
 * ★★★ 为什么加这个：兰兰报「夜间页面有方块」。
 *   真因不是字库缺字 —— 是【日期被写死成 "10 月 3 日 · 星期六"】，
 *   那是昨天，而且"月/日"里的空格与"·"是半角，跟周围字体对不上，
 *   在 480×320 上看着就像方块。改成都从系统时间算。
 * ⚠️ 与顶栏共用 now_minutes 那份判断：时间没对上时宁可显示占位，
 *   也不要显示一个「看起来很像真的」的错日期。*/
static void clock_date_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_lbl_date) return;
    time_t now = time(NULL);
    if (now <= 1700000000) {              /* 还没校上时 */
        lv_label_set_text(s_lbl_date, "Date pending network sync");
        return;
    }
    struct tm tmv;
    localtime_r(&now, &tmv);
    static const char *wd[7] = {"Sun","Mon","Tue","Wed",
                                "Thu","Fri","Sat"};
    int w = tmv.tm_wday; if (w < 0 || w > 6) w = 0;
    lv_label_set_text_fmt(s_lbl_date, "%b %d, %s",
                          tmv.tm_mon + 1, tmv.tm_mday, wd[w]);
}

/* ---- 夜间页：状态行（10-04 改）----
 * ★★★ 兰兰报「夜间页面还有当前播放中的显示」。
 *   现在这行改成【只显示播放状态，不显示台名】：
 *     理由 —— 台名要在夜间页显示，就得把【台单里所有台名的字】都塞进字库，
 *     而字库是我们自己扫源码生成的，只扫 UI 里出现的字。
 *     一旦这里显示台名，699 个台名（含繁体「上海戲曲廣播」）的字都要进主字库，
 *     52px 档会多几百 KB（速查卡铁律 13 踩过这个坑）。
 *   现在改成：状态行只说「在播 / 暂停 / 空闲」，台名去播放页看。
 *   ⇒ 顺便也解决了兰兰说的「上海戏曲广播的曲」（那个「曲」是繁体，
 *     在主字库里没有 ⇒ 方块）。*/
static void night_state_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_lbl_night_state) return;
    if (app_radio_is_playing()) {
        lv_label_set_text(s_lbl_night_state, "Listening");
    } else if (app_radio_is_paused()) {
        lv_label_set_text(s_lbl_night_state, "Paused");
    } else {
        lv_label_set_text(s_lbl_night_state, "Not playing");
    }
}

/* ============================================================
 *  入口
 * ============================================================ */
void ui_init(void)
{
    splash_build();

    s_main_scr = lv_obj_create(NULL);
    lv_obj_remove_style_all(s_main_scr);
    lv_obj_set_style_bg_color(s_main_scr, C_BG, 0);
    lv_obj_set_style_bg_opa(s_main_scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s_main_scr, LV_OBJ_FLAG_SCROLLABLE);

    topbar_build(s_main_scr);
    nav_build(s_main_scr);

    for (int i = 0; i < PAGES_N; i++) {
        lv_obj_t *p = lv_obj_create(s_main_scr);
        lv_obj_remove_style_all(p);
        lv_obj_set_pos(p, CONT_X, CONT_Y);
        lv_obj_set_size(p, CONT_W, CONT_H);
        lv_obj_set_style_bg_color(p, lv_color_hex(s_page_bg[i]), 0);
        lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
        lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
        s_pages[i] = p;
    }
    page_home_build(s_pages[0]);
    page_find_build(s_pages[1]);
    page_play_build(s_pages[2]);
    /* ★ v1.35：夜间页（第 4 页）也【不在这里建】—— 和第 8 页 WiFi 一样，
     *   「进页才建、离开就删」（钩子在 goto_page 里）。
     *   兰兰真机报「点击睡眠页就重启」：这一页最重（时钟+8 颗胶囊+滑块），
     *   常驻占掉池的一大块，等它第一次显示时 draw task 申请不到就崩。*/
    page_about_build(s_pages[4]);
    page_sd_build(s_pages[5]);
    page_list_build(s_pages[6]);   /* 全屏电台列表：栏目 / 省份 / 收藏 共用 */
    /* ★ 第 8 页 WiFi 设置【不在这里建】—— 它是「进页才建、离开就删」。
     *   常驻的开销是 17 KB，而 LVGL 池只有 ~100 KB，used 直接从 75% 冲到 91%，
     *   splash 一撤就崩在 lv_draw_add_task。详见 goto_page 里的注释。*/

    for (int i = 1; i < PAGES_N; i++) lv_obj_add_flag(s_pages[i], LV_OBJ_FLAG_HIDDEN);

    /* ★★★ v1.40：开机就要把【首页】那一套建出来。
     *   为什么不能靠 goto_page(0) 顺带建：s_cur_page 初值本来就是 0，
     *   而 goto_page 开头有 `idx == s_cur_page` 就 return 的守卫
     *   ⇒ 开机时 goto_page(0) 会被直接挡掉，首页网格永远建不出来，
     *      兰兰一开机看到的会是一个没有电台名的空网格。
     *   ⇒ 这里显式建一次。其余两套（第 1 页发现、第 6 页分类）
     *      天然由第一次切过去时建，不急。*/
    grid_pages_ensure(0);

    /* ★★★ 10-04 第二十九次：这里原来还有一行
     *      s_splash_timer = lv_timer_create(splash_done_cb, 2600, NULL);
     *   已删除。splash_build() 里已经建过 3500 ms 的那一个，
     *   这一行把它的【句柄覆盖】掉 ⇒ 旧那个再也删不掉 ⇒
     *   splash_done_cb 每 3.5 秒自己触发一次（重复撤屏 + 重复开机音）。
     *   留着这行的教训：splash 的计时器【只有 splash_build 一处】能建。*/
    s_stat_timer   = lv_timer_create(stat_timer_cb, 1000, NULL);
    s_clock_timer  = lv_timer_create(clock_cb, 1000, NULL);
    /* ★★ 10-04：夜间页的日期与状态行各挂一个定时器。
     *   为什么要单独挂：原来这两行是建页时写死的假值，从来没被刷新过
     *   （兰兰看到的方块 + 假日期就是它）。现在都改成从系统时间/播放状态实时取。
     *   日期 60 秒足够（跨零点时顶栏那一跳会带着它一起变），
     *   状态 1 秒 —— 播放状态变化要立刻反映。*/
    s_date_timer   = lv_timer_create(clock_date_cb, 60000, NULL);
    s_nstate_timer = lv_timer_create(night_state_cb, 1000, NULL);
    /* ★★ 10-04 新增：睡眠定时。
     *   这个回调【在所有页面都会跑】（没有 if (s_cur_page == N)）——
     *   千万别加页判断！到点停播必须在你停在任何一页时都生效
     *   （兰兰可能听完歌切到首页才发现声音停了）。
     *   ★ 这正是第二十五次「顶栏时钟死 label」的同一个坑，反面教材。*/
    s_sleep_timer  = lv_timer_create(sleep_timer_cb, 1000, NULL);
    s_spec_timer   = lv_timer_create(spec_timer_cb, 100, NULL);
    /* ★ 拖动轮询独立 20 ms（第二十二次）。见 seek_timer_cb 的注释：
     *   之前挂在 100 ms 频谱定时器上，是「不灵敏」的主因。*/
    s_seek_timer   = lv_timer_create(seek_timer_cb, 20, NULL);
    /* ★ 第二十四次：WiFi 页的 60 ms 待办定时器已删。屏幕键盘去掉之后，
     *   这一页不再需要在事件回调与对象增删之间绕圈子。*/

    /* 内存体检：LVGL 用它自己的内置池（sdkconfig 里 192 KB、不自动扩容）。
     * 池见底时 lv_draw_add_task() 的 lv_malloc_zeroed() 返回 NULL，
     * 而 release 构建下 LV_ASSERT_MALLOC 是空操作 → 往 NULL 写 → 无限重启。
     * 以后再加 UI，先看这行日志的 used%/peak。*/
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    ESP_LOGI(TAG, "LVGL pool: size=%u B  free=%u B  used=%u%%  frag=%u%%  peak=%u B",
             (unsigned)mon.total_size, (unsigned)mon.free_size,
             (unsigned)mon.used_pct, (unsigned)mon.frag_pct, (unsigned)mon.max_used);

    /* ★★★ 10-04 加的「崩溃预警」（本轮真的崩了一次才补的）
     *  v1.31 首次烧录就 Guru：EXCVADDR=0x08 @ lv_draw_add_task，池 free=13636B。
     *  根因就是「基线 already 87%，我又加了 8 个带样式的对象」。
     *  ⇒ 光在注释里写「加 UI 前先算」是靠不住的 —— 这次还是漏了。
     *     改成一个【会在串口说话的自证开关】：
     *       free < 16 KB  ⇒ 黄色警告（还能跑，但已在悬崖边）
     *       free < 10 KB  ⇒ 红色错误（几乎必崩，此时该减对象而不是继续加）
     *  下一次再崩，串口里第一眼就能看到这行，
     *  而不是花半小时去反推「这轮到底加了几个对象」。
     *  ⚠️ 阈值是按 112 KB 池的经验值：
     *     崩的那版 free=13636，peak=88768；正常跑 free 稳定在 26 KB 上下。*/
    if (mon.free_size < 10240) {
        ESP_LOGE(TAG, "★★ LVGL 池危险！free=%u B（<10 KB）—— 立刻要减 lv_obj，"
                      "再加 UI 必崩（EXCVADDR=0x08 @ lv_draw_add_task）",
                 (unsigned)mon.free_size);
    } else if (mon.free_size < 16384) {
        ESP_LOGW(TAG, "★ LVGL 池偏紧 free=%u B（<16 KB）—— 加 UI 前先减对象",
                 (unsigned)mon.free_size);
    }

    stat_refresh();
    player_refresh();          /* 播放页先按「空闲」态画一遍，别停在假数据上 */
    wifi_indicator_refresh();  /* 顶栏 WiFi 标志立刻有值，别停在占位符 */
    /* 首页大卡：开机先按「正在播的那台 / 台单第 1 台」画一遍，
     * 别停在「正在载入台单…」*/
    if (app_radio_station_count() > 0) {
        s_hr_sel = 0;
        home_update_card(0);
    }
    if (s_lbl_favcnt) {
        lv_label_set_text_fmt(s_lbl_favcnt, "%d favorites", app_fav_count());
    }
    /* 第 7 页先按「收藏」建一次，开机就是有内容的那一屏 */
    s_list_mode = LIST_BY_FAV;
    s_list_title = "My favorites";
    list_page_refresh();

    ESP_LOGI(TAG, "UI built (7 pages, %d stations, %d cats, %d provs, %d favs)",
             app_radio_station_count(), NET_CAT_N, NET_PROV_N, app_fav_count());
}
