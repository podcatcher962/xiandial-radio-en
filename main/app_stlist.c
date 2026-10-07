/*
 * 拾声 (XianDial) 硬件版 —— SD 卡台单导入（stations.tsv）
 * 见app_stlist.h 的契约说明。
 *
 * 【为什么这样写】
 *  ① 回落优先：任何异常都回落到内置台单。依据 v1.40 的教训 ——
 *     LVGL 在 release 下分配失败是静默崩溃，UI 侧任何"数据可能没有"
 *     的路径都必须在这里兜住，界面上只看到"0 台"这种合法状态，
 *     看不到野指针。
 *  ② 内存：8MB PSRAM（OCT）。2000 台的字符串池约 200 KB，
 *     指针数组 8 KB，总约 208 KB，一次 malloc 常驻。
 *  ③ 不在 UI 回调里分配：app_st_load() 由开机流程调用一次。
 *  ④ 解析严格但不苛刻：多一个 TAB、少一个字段都跳过这一行并计数，
 *     不因为一行坏就丢掉整份文件 —— 使用者导出的 M3U 千奇百怪。
 */
#include "app_stlist.h"
#include "xs_version_mode.h"   /* XS_FAV_PUBLISH：发布版形态开关，见 xs_version_mode.h */
#include "app_sd.h"
#include "app_radio.h"
#include "ui_xiandial.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <esp_timer.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
/* ★★ 10-06 编译失败踩的坑：这里原来写的是 <driver/uart.h>，报
 *   "fatal error: driver/uart.h: No such file or directory"。
 *   根因不是路径写错，而是【这块板根本没有硬件 UART】：
 *   sdkconfig 里 CONFIG_ESP_CONSOLE_UART_NUM=-1、
 *   CONFIG_ESP_CONSOLE_UART_DEFAULT 未选、控制台走 USB Serial/JTAG。
 *   也就是说 uart_read_bytes() 压根没有设备可读。
 *   正确通道：esp_driver_usb_serial_jtag 组件的 usb_serial_jtag_read_bytes()。
 *   IDF 6.x 已把老 driver 组件拆成 esp_driver_*，但【头文件路径没变】，
 *   所以 app_display.c 里的 driver/i2c_std.h 之类照样能编过 ——
 *   别看到 driver/xxx.h 报缺文件就以为是改名问题，先查有没有这个外设。*/
#include <driver/usb_serial_jtag.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sdkconfig.h>

static const char *TAG = "XSTLIST";

/* ---- 常驻状态 ---- */
static net_station_t *s_pool = NULL;      /* 指针数组，PSRAM */
static char         *s_str  = NULL;       /* 字符串池，PSRAM */
static size_t        s_str_used = 0;
static size_t        s_str_cap  = 0;
static int           s_count    = 0;
static app_st_src_t  s_src      = APP_ST_SRC_BUILTIN;
static bool          s_loaded   = false;

/* 名字 → 下标 的小缓存（仅加速收藏/历史查找，不影响正确性） */
#define NAME_CACHE 64
static struct { char name[72]; int idx; } s_ncache[NAME_CACHE];
static int s_ncache_n = 0;

/* ---------- 字符串池 ---------- */
static bool str_reserve(size_t need)
{
    if (s_str_used + need <= s_str_cap) return true;
    if (!s_str) {
        /* 首次：按上限一次性给足，避免反复 realloc 造成碎片 */
        size_t cap = 384 * 1024;                 /* 384 KB，2000 台够用 */
        s_str = heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
        if (!s_str) {
            /* 没有 PSRAM 就退到内部 RAM（会紧张，但比不工作强）*/
            ESP_LOGW(TAG, "PSRAM 分配失败，退回内部 RAM %u B", (unsigned)cap);
            cap = 160 * 1024;
            s_str = heap_caps_malloc(cap, MALLOC_CAP_8BIT);
        }
        if (!s_str) { ESP_LOGE(TAG, "字符串池分配失败"); return false; }
        s_str_cap = cap;
        s_str_used = 0;
        return s_str_used + need <= s_str_cap;
    }
    /* 池子不够：扩到 2 倍。old 块留在 PSRAM 里没释放，偶发一次可接受。 */
    size_t ncap = s_str_cap * 2;
    char *n = heap_caps_malloc(ncap, MALLOC_CAP_SPIRAM);
    if (!n) n = heap_caps_malloc(ncap, MALLOC_CAP_8BIT);
    if (!n) { ESP_LOGE(TAG, "字符串池扩容失败 %u->%u", (unsigned)s_str_cap, (unsigned)ncap); return false; }
    memcpy(n, s_str, s_str_used);
    s_str = n; s_str_cap = ncap;
    return s_str_used + need <= s_str_cap;
}

static const char *str_put(const char *src, size_t n)
{
    if (n == 0) return "";
    if (!str_reserve(n + 1)) return NULL;
    char *p = s_str + s_str_used;
    memcpy(p, src, n);
    p[n] = '\0';
    s_str_used += n + 1;
    return p;
}

/* ---------- 名字 → 下标 映射 ---------- */
/* 栏目/地区对不上时的兜底下标。名字必须与 net_stations.h 的
 * NET_CAT_N=13 顺序一致；找不到就用 0（第一个，界面仍能显示）。*/
static unsigned char cat_index(const char *s)
{
    if (!s || !*s) return 0;
    for (int i = 0; i < NET_CAT_N; i++)
        if (strcmp(s, g_cat_name[i]) == 0) return (unsigned char)i;
    return 0;      /* 对不上→ 第一个（界面显示的是真名，不影响播放） */
}

static unsigned char prov_index(const char *s)
{
    if (!s || !*s) return NET_PROV_NONE;
    /* 「全国」「其他」按契约→ NET_PROV_NONE。
     * ⚠️ 这与g_prov_name 第 35 项「其他华语」不是一回事，别混。 */
    if (strcmp(s, "全国") == 0 || strcmp(s, "其他") == 0)
        return NET_PROV_NONE;
    for (int i = 0; i < NET_PROV_N; i++)
        if (strcmp(s, g_prov_name[i]) == 0) return (unsigned char)i;
    return NET_PROV_NONE;
}

/* 去掉首尾空白 + 去掉 Windows 风格尾部 CR（★ CRLF 会把URL 尾部挂 \r，
 * 症状是「别的台都好，就这几台不行」，极难查，所以这里必须剥）*/
static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\r' || s[n - 1] == '\n' ||
                     s[n - 1] == ' '  || s[n - 1] == '\t'))
        s[--n] = '\0';
    return s;
}

/* 把 TSV 的一行拆成 4 段，就地打 \0，返回段数（<4 视为坏行） */
static int split4(char *line, char *f[4])
{
    int k = 0;
    f[0] = line;
    for (char *p = line; *p; p++) {
        if (*p == '\t') {
            *p = '\0';
            if (k < 3) f[++k] = p + 1;
            else return 4;            /* 第 5 个 TAB 起，后面全部并入第 4 段 */
        }
    }
    return k + 1;
}

/* ---------- 内部台单也走缓存池，让上层代码只有一条路径 ----------
 * 只在「没找到 TSV」时调用：把 g_stations 复制到池里。
 * 好处：app_st_get() 永远是同一套逻辑，不会出现
 *「走了 TSV 路径」和「走了内置路径」两种行为。 */
static void load_builtin(void)
{
    int n = g_station_count;
    if (n <= 0) { s_count = 0; return; }
    if (n > APP_ST_MAX) n = APP_ST_MAX;

    s_pool = heap_caps_calloc(n, sizeof(net_station_t), MALLOC_CAP_SPIRAM);
    if (!s_pool) s_pool = calloc(n, sizeof(net_station_t));
    if (!s_pool) { ESP_LOGE(TAG, "内置台单 %d 条分配失败", n); s_count = 0; return; }

    for (int i = 0; i < n; i++) {
        const char *nm = g_stations[i].name ? g_stations[i].name : "";
        const char *ur = g_stations[i].url  ? g_stations[i].url  : "";
        const char *pn = str_put(nm, strlen(nm));
        const char *pu = str_put(ur, strlen(ur));
        if (!pn || !pu) {            /* 池子不够就到此为止，能放多少放多少 */
            s_count = i; break;
        }
        s_pool[i].name = pn;
        s_pool[i].url  = pu;
        s_pool[i].cat  = g_stations[i].cat;
        s_pool[i].prov = g_stations[i].prov;
    }
    if (s_count == 0) s_count = n;
    s_src = APP_ST_SRC_BUILTIN;
    ESP_LOGI(TAG, "内置台单 %d 条", s_count);
}

static void free_all(void)
{
    if (s_pool) { free(s_pool); s_pool = NULL; }
    /* s_str 刻意不释放：常驻，释放了立刻还要再分配，碎片更糟 */
    s_count = 0; s_ncache_n = 0;
}

static void ncache_add(const char *name, int idx)
{
    if (s_ncache_n >= NAME_CACHE) {
        /* 简单覆盖第0 项（收藏一般只有十几个，够用） */
        snprintf(s_ncache[0].name, sizeof(s_ncache[0].name), "%s", name);
        s_ncache[0].idx = idx;
        return;
    }
    snprintf(s_ncache[s_ncache_n].name, sizeof(s_ncache[s_ncache_n].name), "%s", name);
    s_ncache[s_ncache_n].idx = idx;
    s_ncache_n++;
}

/* ---------- 主流程：解析 stations.tsv ---------- */
static app_st_src_t load_tsv(void)
{
    if (!app_sd_is_mounted()) {
        ESP_LOGI(TAG, "SD 未挂载，用内置台单");
        load_builtin();
        return APP_ST_SRC_BUILTIN;
    }

    FILE *fp = fopen(APP_ST_FILE, "rb");
    if (!fp) {
        ESP_LOGI(TAG, "没有 %s，用内置台单", APP_ST_FILE);
        load_builtin();
        return APP_ST_SRC_BUILTIN;
    }

    /* 先数行，据此定池大小，避免反复扩容 */
    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (fsz <= 0) { fclose(fp); load_builtin(); return APP_ST_SRC_BUILTIN; }

    int cap = (int)(fsz / 64);          /* 估算：一条台单最短约 64 字节 */
    if (cap < 32)   cap = 32;
    if (cap > APP_ST_MAX) cap = APP_ST_MAX;

    s_pool = heap_caps_calloc(cap, sizeof(net_station_t), MALLOC_CAP_SPIRAM);
    if (!s_pool) s_pool = calloc(cap, sizeof(net_station_t));
    if (!s_pool) {
        ESP_LOGE(TAG, "台单数组分配失败（%d 条）", cap);
        fclose(fp); load_builtin(); return APP_ST_SRC_BUILTIN;
    }

    char line[APP_ST_MAX_LINE];
    int  bad = 0, over = 0;
    /* ① UTF-8 BOM：'\xEF\xBB\xBF'。带 BOM 会让第一个台名变成
     *    "□中央人民广播电台"，且收藏按名匹配永远存不上。 */
    int  first = 1;

    while (fgets(line, sizeof(line), fp)) {
        if (first) {
            first = 0;
            size_t l = strlen(line);
            if (l >= 3 && (unsigned char)line[0] == 0xEF &&
                (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF) {
                memmove(line, line + 3, l - 2);
                ESP_LOGW(TAG, "★ 文件带 BOM，已剥离（否则第一个台名会多一个方块）");
            }
        }
        /* 注释行与空行 */
        char *t = trim(line);
        if (*t == '\0' || *t == '#') continue;

        char *f[4];
        if (split4(t, f) < 4) { bad++; continue; }

        char *name = trim(f[0]);
        char *cat  = trim(f[1]);
        char *prov = trim(f[2]);
        char *url  = trim(f[3]);
        if (!*name || !*url) { bad++; continue; }

        if (s_count >= APP_ST_MAX) { over++; continue; }
        if (s_count >= cap) break;             /* 池满，停止 */

        const char *pn = str_put(name, strlen(name));
        const char *pu = str_put(url,  strlen(url));
        if (!pn || !pu) {
            ESP_LOGE(TAG, "字符串池在第 %d 行耗尽，截断到 %d 条", s_count + 1, s_count);
            break;
        }
        s_pool[s_count].name = pn;
        s_pool[s_count].url  = pu;
        s_pool[s_count].cat  = cat_index(cat);
        s_pool[s_count].prov = prov_index(prov);
        ncache_add(pn, s_count);
        s_count++;
    }
    fclose(fp);

    if (s_count <= 0) {
        ESP_LOGW(TAG, "TSV 解析到 0 条（坏行 %d），回落到内置台单", bad);
        free_all();
        load_builtin();
        return APP_ST_SRC_BUILTIN;
    }

    s_src = APP_ST_SRC_TSV;
    ESP_LOGI(TAG, "★ 台单来自 %s：%d 条（坏行 %d，超限丢弃 %d）",
             APP_ST_FILE, s_count, bad, over);
    return APP_ST_SRC_TSV;
}

/* ---------- 对外接口 ---------- */
app_st_src_t app_st_load(void)
{
    if (s_loaded) return s_src;             /* 幂等 */
    s_loaded = true;
    free_all();
    s_ncache_n = 0;
    app_st_src_t r = load_tsv();
    ESP_LOGI(TAG, "载入完成：来源=%s 台数=%d PSRAM free=%u",
             r == APP_ST_SRC_TSV ? "TSV" : "内置", s_count,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return r;
}

app_st_src_t app_st_source(void) { return s_src; }
int         app_st_count(void)   { return s_count; }

const net_station_t *app_st_get(int i)
{
    if (i < 0 || i >= s_count) return NULL;
    return &s_pool[i];
}

int app_st_find_by_name(const char *name)
{
    if (!name || !*name) return -1;
    for (int k = 0; k < s_ncache_n; k++)
        if (strcmp(s_ncache[k].name, name) == 0) return s_ncache[k].idx;
    for (int i = 0; i < s_count; i++)
        if (strcmp(s_pool[i].name, name) == 0) return i;
    return -1;
}

/* ---------- 串口命令通道（10-06）----------
 * 用途：插了卡、换了 stations.tsv 之后不用重启机器。
 * 串口敲 st_reload 即可重新读卡；st_dump 打印当前台单。
 *
 * ★ 为什么需要它：板载TF 卡座走SDIO，Windows 看不到这张卡
 *   （板子不是 USB 读卡器），所以开发时没法像 U 盘那样往里拷文件。
 *   有了这条命令，改完文件在串口敲一下就能验证，不必反复断电。
 * ★ 长期有用：使用者自己换了台单同样能热重载，不用重启。
 */
/* ---------- 卡上文件的读 / 写 / 删（10-06）----------
 * 存在的理由：板载 TF 走 SDIO，Windows 看不到这张卡；板载 Type-C 的
 * PHY 又被 Serial-JTAG 占着、板子不能当 USB 主机。⇒ 改卡上文件以前
 * 只能拔卡插读卡器。有了这一组命令，改台单/改 WiFi 账密都在串口里做。
 *
 * ★ 写协议刻意做成「一行一条命令」而不是发裸字节：串口是字符设备，
 *   裸字节协议要自己处理转义、半包、粘包，调试时肉眼看不出来。
 *   一行一条虽然慢（13139 字节的台单要发 144 行），但每次都能看见。
 */

#define ST_PATH_MAX 64

/* 正在写入的目标文件名（st_put 用）*/
static char s_wfp_name[ST_PATH_MAX];

/* 只允许操作 SD 卡根目录下的文件，且文件名里不许出现 '/' 或 ".."，
 * 免得一条 st_cat ../../../ 之类把别的分区也读了。
 * 只读卡是别人的数据，这层检查不能省。 */
static bool st_safe_name(const char *name)
{
    if (!name || !*name) return false;
    size_t n = strlen(name);
    if (n >= ST_PATH_MAX) return false;
    if (strchr(name, '/') || strchr(name, '\\')) return false;
    if (n >= 2 && name[0] == '.' && name[1] == '.') return false;
    return true;
}

static bool st_path(char *out, size_t cap, const char *name)
{
    if (!st_safe_name(name)) return false;
    snprintf(out, cap, "/sdcard/%s", name);
    return true;
}

/* ---- st_ls：列根目录 ---- */
static void st_ls(void)
{
    if (!app_sd_is_mounted()) { ESP_LOGW(TAG, "st_ls: SD 没挂载"); return; }
    DIR *d = opendir("/sdcard");
    if (!d) { ESP_LOGW(TAG, "st_ls: 打不开根目录"); return; }
    struct dirent *e;
    int n = 0;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        /* ★ d_name 最长 255 字节，路径缓冲必须留够 8+255+1，否则
         *   编译报 format-truncation（-Werror 直接挂）。曾经写 160 被抓出来。*/
        char p[300];
        snprintf(p, sizeof(p), "/sdcard/%s", e->d_name);
        struct stat stt;
        if (stat(p, &stt) == 0)
            ESP_LOGI(TAG, "  %-16s %8lld B", e->d_name, (long long)stt.st_size);
        else
            ESP_LOGI(TAG, "  %-16s <?>", e->d_name);
        n++;
    }
    closedir(d);
    ESP_LOGI(TAG, "st_ls: 共 %d 项", n);
}

/* ---- st_cat <file>：回读卡上文本文件 ----
 * 用途：写完立刻核对。写入成功返回码不代表内容对，
 * 只有把字节读回来看到才对（★ 铁律：自写 check 先用已知答案验证再信结论）。*/
static void st_cat(const char *name)
{
    char p[ST_PATH_MAX + 16];
    if (!st_path(p, sizeof(p), name)) { ESP_LOGW(TAG, "st_cat: 文件名不合法"); return; }
    FILE *fp = fopen(p, "rb");
    if (!fp) { ESP_LOGW(TAG, "st_cat: 打不开 %s", p); return; }
    char line[APP_ST_MAX_LINE];
    int n = 0;
    size_t total = 0;
    while (fgets(line, sizeof(line), fp)) {
        total += strlen(line);
        /* 只打前后各若干行，中间省掉，免得刷屏 */
        if (n < 5 || n >= 1000) ESP_LOGI(TAG, "  |%s", line);
        else if (n == 5) ESP_LOGI(TAG, "  ...（中间省略）...");
        n++;
    }
    fclose(fp);
    ESP_LOGI(TAG, "st_cat: %s 共 %d 行 / %u 字节", name, n, (unsigned)total);
}

/* ---- st_rm <file> ---- */
static void st_rm(const char *name)
{
    char p[ST_PATH_MAX + 16];
    if (!st_path(p, sizeof(p), name)) { ESP_LOGW(TAG, "st_rm: 文件名不合法"); return; }
    if (remove(p) == 0) ESP_LOGW(TAG, "st_rm: 已删除 %s", name);
    else            ESP_LOGW(TAG, "st_rm: 删不掉 %s", name);
}

/* ---- st_put <file>：进入写入模式 ----
 * 后续每行原样写入（自动补 LF），单独一行 "." 结束并回读校验。
 * 返回 true 表示已进写入模式，调用方要把后续行喂给 st_put_line()。 */
static bool st_put_begin(const char *name, FILE **out_fp)
{
    char p[ST_PATH_MAX + 16];
    if (!app_sd_is_mounted()) { ESP_LOGW(TAG, "st_put: SD 没挂载"); return false; }
    if (!st_path(p, sizeof(p), name)) { ESP_LOGW(TAG, "st_put: 文件名不合法"); return false; }

    /* ★ 先写 .tmp 再改名：中途断电/断线不会留下半个文件，
     *   而半个 stations.tsv 会被解析成「一堆坏行」甚至 0 台。 */
    char tmp[ST_PATH_MAX + 24];
    snprintf(tmp, sizeof(tmp), "%s.tmp", p);
    FILE *fp = fopen(tmp, "wb");
    if (!fp) { ESP_LOGW(TAG, "st_put: 建不了 %s", tmp); return false; }
    *out_fp = fp;
    ESP_LOGW(TAG, "st_put: 开始写 %s（写到 %s.tmp，收到单独一行 . 结束）", name, name);
    return true;
}

static void st_put_line(FILE *fp, const char *line)
{
    fputs(line, fp);
    fputc('\n', fp);
}

static void st_put_end(FILE *fp, const char *name)
{
    fflush(fp);
    fclose(fp);
    char p[ST_PATH_MAX + 16], tmp[ST_PATH_MAX + 24];
    st_path(p, sizeof(p), name);
    snprintf(tmp, sizeof(tmp), "%s.tmp", p);
    if (rename(tmp, p) != 0) {
        ESP_LOGE(TAG, "st_put:改名失败 %s -> %s", tmp, p);
        return;
    }
    ESP_LOGW(TAG, "st_put: 写完，已改名为 %s，下面回读核对", name);
    st_cat(name);
}

/* ---- st_net <url>[?insecure] ：分层测网络（10-06）----
 * 把「连不上」拆成四层，每层单独报，才能知道该修哪：
 *   ① DNS   —— getaddrinfo 解析出 IP
 *   ② TCP   —— socket + connect 裸握手，打印 errno / 目标 IP / 耗时
 *   ③ HTTP  —— esp_http_client_open + fetch_headers，打印状态码
 *   ④ 内容  —— 拉第一行（正常时应该是 #EXTM3U）
 *
 * ★ 实测结论（10-06 台单播不出声的现场）：
 *   https 源：① 通 ② 通（68 ms）③ 失败 TLS err=0x008D
 *   http  源：① 通 ② 通（17 ms）③ 200 ④ #EXTM3U
 *   ⇒ 断点在 TLS 这一层，DNS 与 TCP 都没问题，源也没死。
 *
 * ★ 加 ?insecure 后缀可关掉证书校验再试一次，这是区分
 *   「内存不够」与「证书链不认」的唯一办法 —— 两者都走 HTTPS、
 *   都吃内部 RAM，光看错误码分不出来。
 *
 * ★★★ 10-06 隐私审计：`?insecure` 在【发布版里编译期关掉】
 *   （XS_FAV_PUBLISH=1）。理由：
 *     它是全工程唯一一条能人为关闭 TLS 加密的代码路径。虽然要物理
 *     串口输入才触发、不是自动行为，但「发布版本里带一个关证书校验的
 *     开关」在任何合规审查里都是扣分项 —— 而它的唯一用途是【排障】，
 *     排障只需要自用版。这是不损失任何发布功能的纯减法。
 *   ⇒ 使用者拿到的是发布版时，`st_net <地址>?insecure` 会被忽略并打
 *     一行提示，要用这个开关请自编译（README 已说明）。
 */
static void st_net_probe(const char *url_in)
{
/*★★ 10-06 踩坑：这几个数组原本是函数里的局部变量，结果
 *   ***ERROR*** A stack overflow in task xs_stcmd has been detected.
 *   板子直接复位 —— 而且因为复位发生在 printf 中间，串口日志被截成半行
 *   （「=== 分层测网络: https://<某个流地址>」），看着像丢日志，
 *   实际是崩了。**教训：加了大的局部数组后，必须同步调大任务栈。**
     *   这里选【static】而不是把栈调大：串口任务只在有人敲命令时跑，
     *   320×2 字节常驻内部 RAM 无所谓；而调栈会影响常驻内存，
     *   在只剩 23~30 KB 内部 RAM 的机器上是更坏的选择。
     *   ⚠ 这几个变量绝不能递归/重入使用 —— 目前只在这一个任务里用，安全。*/
    static char url[320];
    static char clean[320];
    static char host[128];

    /* 先把 ?insecure 后缀摘掉，剩下才是真正的 URL */
    snprintf(url, sizeof(url), "%.300s", url_in ? url_in : "");
    char *qmark = strchr(url, '?');
    if (qmark) *qmark = '\0';

    ESP_LOGW(TAG, "=== 分层测网络: %s ===", url);

    /* ⚠ url 已经是上面的局部数组（不再是入参指针），所以不能判 !url，
     *   判了 gcc 会报 "address of 'url' will always evaluate as true"。
     *   入参为空的情形在 snprintf 那一步就把数组填成空串了。*/
    if (url[0] == '\0' || strncmp(url, "http", 4) != 0) {
        ESP_LOGE(TAG, "不是 http(s) 开头的地址");
        return;
    }
    int is_https = (strncmp(url, "https", 5) == 0);
    int port = is_https ? 443 : 80;

    /* 拆出 host 与 path */
    const char *hs = strstr(url, "://");
    hs = hs ? hs + 3 : url;
    const char *he = strpbrk(hs, "/?#");
    size_t hn = he ? (size_t)(he - hs) : strlen(hs);
    if (hn >= sizeof(host)) hn = sizeof(host) - 1;
    memcpy(host, hs, hn);
    host[hn] = '\0';
    /* 带端口的写法 a.b:80 */
    char *colon = strrchr(host, ':');
    if (colon) { *colon = '\0'; port = atoi(colon + 1); }

    ESP_LOGW(TAG, "① host = %s  port = %d (%s)", host, port, is_https ? "TLS" : "plain text");
    ESP_LOGW(TAG, "   内存起点：internal free=%u largest=%u",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

    /* ① DNS */
    struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
    struct addrinfo *res = NULL;
    int rc = getaddrinfo(host, NULL, &hints, &res);
    if (rc != 0 || !res) {
        /* ★ 不用 gai_strerror：新版 lwIP 的 netdb.h 不导出它，
         *   用了会报 implicit declaration（-Werror 直接挂）。
         *   打错误码 + 主机名就够了，读者要的是「解析不出」这件事。*/
        ESP_LOGE(TAG, "① DNS 失败 rc=%d —— 机器解析不了 %s", rc, host);
        return;
    }
    struct in_addr ip4;
    memcpy(&ip4, &((struct sockaddr_in *)res->ai_addr)->sin_addr, sizeof(ip4));
    ESP_LOGW(TAG, "① DNS OK → %d.%d.%d.%d",
             (int)((uint8_t *)&ip4)[0], (int)((uint8_t *)&ip4)[1],
             (int)((uint8_t *)&ip4)[2], (int)((uint8_t *)&ip4)[3]);
    freeaddrinfo(res);

    /* ② TCP：这一步是判定重点。esp_http_client 只会笼统报
     *   ESP_ERR_HTTP_CONNECT，分不出是解析失败还是握手被拒。*/
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) { ESP_LOGE(TAG, "② socket() 失败 errno=%d", errno); return; }
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(port);
    sa.sin_addr   = ip4;
    int64_t t0 = esp_timer_get_time();
    int cr = connect(s, (struct sockaddr *)&sa, sizeof(sa));
    int64_t dt = (esp_timer_get_time() - t0) / 1000;
    if (cr == 0) {
        ESP_LOGW(TAG, "② TCP OK —— 握手成功，%lld ms", (long long)dt);
    } else {
        ESP_LOGE(TAG, "② TCP 失败 errno=%d (%s) 目标 %s:%d 耗时 %lld ms",
                 errno, strerror(errno), host, port, (long long)dt);
        close(s);
        return;      /* 握手都过不去，后面几层不用试了 */
    }
    close(s);

    /* ③+④ HTTP：走 esp_http_client，状态码与 m3u8 首行一并打出来。
     * 故意用最简配置（带 CA 证书包），和 hls_fetch_list 保持一致，
     * 免得「这里通、那里不通」是配置差异造成的假象。
     *
     * ★★ st_net insecure：URL 带 ?insecure 就关掉证书校验再试一次。
     *   这是【区分「内存不够」与「证书链不认」的唯一办法】：
     *   ① 证书校验要额外分配 X509 解析缓冲，很吃内部 RAM；
     *   ② 校验失败返回的是 X509 段错误码（0x2xxx/0x3xxx）。
     *   两个都走 HTTPS、都吃 RAM，错误码不同，靠猜分不出来 ——
     *   只能让机器自己跑一遍两种配置对比。*/
    /* ★ url 顶层的 ?insecure 已经在函数开头摘掉了（连同 query 一起），
     *   所以这里只要判断标记还在不在原始入参里。
     *   ⚠ clean 必须开 320：url 是 320 字节数组，用 256 会被
     *   -Werror=format-truncation 拦下（319 > 256）。*/
    /* ★ 发布版（XS_FAV_PUBLISH=1）这个开关在【编译期就不存在】——
       隐私审计要求，见上面函数头注释。*/
#if XS_FAV_PUBLISH
    bool insecure = false;
    if (url_in && strstr(url_in, "?insecure"))
        ESP_LOGW(TAG, "   发布版不支持 ?insecure（关证书校验的开关只留自用版）");
#else
    bool insecure = (strstr(url_in ? url_in : "", "?insecure") != NULL);
#endif
    snprintf(clean, sizeof(clean), "%.300s", url);
    char *q = strchr(clean, '?');
    if (q) *q = '\0';

    esp_http_client_config_t cfg = {
        .url = clean,
        .timeout_ms = 8000,
        .buffer_size = 1024,
        .crt_bundle_attach = insecure ? NULL : esp_crt_bundle_attach,
    };
    /* insecure 时用 mbedtls 自带的 skip_verify（不挂证书包即可）*/
    if (insecure) {
        ESP_LOGW(TAG, "   ⚠ 已关掉证书校验（仅用于定位问题，不可用于发布）");
        cfg.crt_bundle_attach = NULL;
    }
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (!c) { ESP_LOGE(TAG, "③ http 句柄建不了（可能是内部 RAM 不够）"); return; }
    esp_http_client_set_header(c, "User-Agent", "Mozilla/5.0 (XianDial)");
    esp_err_t oe = esp_http_client_open(c, 0);
    if (oe != ESP_OK) {
        ESP_LOGE(TAG, "③ open 失败 err=%d (%s)", (int)oe, esp_err_to_name(oe));
        if (is_https) {
            int tc = 0, tf = 0;
            esp_http_client_get_and_clear_last_tls_error(c, &tc, &tf);
            ESP_LOGE(TAG, "   TLS err=0x%04X flags=0x%04X", (unsigned)tc, (unsigned)tf);
            ESP_LOGE(TAG, "   内部 RAM 此刻 free=%u largest=%u",
                     (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                     (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
            if (insecure)
                ESP_LOGE(TAG, "   ⇒ 不挂证书包也失败 ⇒ 不是证书链问题，是内存/协议问题");
            else
                ESP_LOGE(TAG, "   ⇒ 用 st_net <同一个地址>?insecure 再试一次即可定性");
        }
        esp_http_client_cleanup(c);
        return;
    }
    int total = esp_http_client_fetch_headers(c);
    int status = esp_http_client_get_status_code(c);
    ESP_LOGW(TAG, "③ HTTP 状态 = %d（fetch_headers 返回 %d）", status, total);
    if (status == 200 || status == 206) {
        char buf[256];
        int n = esp_http_client_read(c, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            char *nl = strpbrk(buf, "\r\n");
            if (nl) *nl = '\0';
            ESP_LOGW(TAG, "④ 首行 = %s", buf);
        }
    }
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    ESP_LOGW(TAG, "=== 测完 ===");
}

static void st_serial_task(void *arg)
{
    char    line[APP_ST_MAX_LINE];
    size_t  n = 0;
    uint8_t ch;
    FILE   *wfp = NULL;            /* 非空 = 正在写入模式 */

    ESP_LOGI(TAG, "串口通道就绪：st_dump / st_reload / st_ls / st_cat <f> / "
                  "st_put <f> (结束=单行.) / st_rm <f> / st_play <下标>");

    /* ★ 驱动已被控制台装好了（usb_serial_jtag_vfs_dev_port_init 在启动时
     *   调了 driver_install + vfs_use_driver），这里只管读。
     *   万一没装上就自己装一次，失败就退出任务，不能让机器重启。*/
    if (!usb_serial_jtag_is_driver_installed()) {
        usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
        cfg.rx_buffer_size = 1024;
        if (usb_serial_jtag_driver_install(&cfg) != ESP_OK) {
            ESP_LOGE(TAG, "USB-JTAG 驱动装不上，热重载通道关闭（不影响收音）");
            vTaskDelete(NULL);
            return;
        }
        ESP_LOGW(TAG, "USB-JTAG 驱动由本任务补装");
    }

    for (;;) {
        if (usb_serial_jtag_read_bytes(&ch, 1, portMAX_DELAY) != 1)
            continue;
        if (ch == '\r' || ch == '\n') {
            line[n] = '\0';
            char *t = trim(line);
            if (*t) {
                if (wfp) {                          /* 写入模式：吃掉一切 */
                    if (strcmp(t, ".") == 0) { st_put_end(wfp, s_wfp_name); wfp = NULL; }
                    else                            st_put_line(wfp, t);
                } else if (strcmp(t, "st_reload") == 0) {
                    ESP_LOGW(TAG, "收到 st_reload —— 重新读卡");
                    free_all();
                    s_ncache_n = 0;
                    app_st_src_t r = load_tsv();
                    ESP_LOGI(TAG, "重读完成：来源=%s 台数=%d",
                             r == APP_ST_SRC_TSV ? "TSV" : "内置", s_count);
                    app_st_dump_info();
                    /* UI 需要重建才会看到新台单 */
                    ui_reload_stations();
                } else if (strcmp(t, "st_dump") == 0) {
                    app_st_dump_info();
                } else if (strcmp(t, "st_ls") == 0) {
                    st_ls();
                } else if (strncmp(t, "st_play", 7) == 0 && t[7] == ' ') {
                    int idx = atoi(trim(t + 8));
                    if (idx < 0 || idx >= s_count) {
                        ESP_LOGW(TAG, "st_play: 下标 %d 越界（共 %d 台）", idx, s_count);
                    } else {
                        ESP_LOGW(TAG, "st_play: 播第 %d 台 = %s", idx, s_pool[idx].name);
                        ESP_LOGW(TAG, "st_play: url = %s", s_pool[idx].url);
                        esp_err_t r = app_radio_play_url(s_pool[idx].name, s_pool[idx].url);
                        ESP_LOGW(TAG, "st_play: 返回 %s，3 秒后看错误与状态",
                                 esp_err_to_name(r));
                    }
                } else if (strncmp(t, "st_cat", 6) == 0 && t[6] == ' ') {
                    st_cat(trim(t + 7));
                } else if (strncmp(t, "st_rm", 5) == 0 && t[5] == ' ') {
                    st_rm(trim(t + 6));
                } else if (strncmp(t, "st_put", 6) == 0 && t[6] == ' ') {
                    char *nm = trim(t + 7);
                    snprintf(s_wfp_name, sizeof(s_wfp_name), "%s", nm);
                    wfp = NULL;
                    if (st_put_begin(nm, &wfp)) { /* 已进入写入模式 */ }
                } else if (strncmp(t, "st_net", 6) == 0 && t[6] == ' ') {
                    /* ★ 10-06 加这条：分层测网络 —— DNS / TCP / HTTP 拆开。
                     *   起因：143 个台全部 ESP_ERR_HTTP_CONNECT（TCP 连不上），
                     *   而【同一台电脑、同一 WiFi、同一源全部 200】。
                     *   同一个源一边通一边不通 ⇒ 一定要知道断在哪一层，
                     *   否则只能猜。分层测完才知道是 DNS 解析、TCP 握手、
                     *   还是 TLS 证书 —— 三者的修法完全不同。*/
                    st_net_probe(trim(t + 7));
                } else {
                    ESP_LOGI(TAG, "未知命令「%s」", t);
                }
            }
            n = 0;
            continue;
        }
        if (ch == 0x08 || ch == 0x7F) { if (n) n--; continue; }   /* 退格 */
        if (n < sizeof(line) - 1) line[n++] = (char)ch;
    }
}

void app_st_start_serial_cmd(void)
{
    static TaskHandle_t t = NULL;
    if (t) return;
    /* ★★ 栈 3072 → 5120。10-06 实测 3072 会
     *   ***ERROR*** A stack overflow in task xs_stcmd has been detected***
     *   —— 触发者是 st_net 里 esp_http_client_open() 走 TLS 握手那条路
     *   （mbedTLS 自己还要用几百字节栈）。诊断命令把板子搞复位，
     *   一次白等 5 分钟。留足余量，别再省这点内存。*/
    xTaskCreate(st_serial_task, "xs_stcmd", 5120, NULL, 3, &t);
}

void app_st_dump_info(void)
{
    if (s_src == APP_ST_SRC_TSV)
        ESP_LOGI(TAG, "台单源=TSV(%s) 条数=%d", APP_ST_FILE, s_count);
    else
        ESP_LOGI(TAG, "台单源=内置 条数=%d", s_count);

    /* ★ 台单构成统计（10-06 从 app_radio.c 搬来）。
     *   必须分开报 http/https：10-04 那次把 281 个 https HLS 改写成 http
     *   （省 TLS 握手堆），但文案还写着「HLS https 353」，看日志会以为没生效。
     *   ⇒ 四个数都打：直连 http / 直连 https / HLS http / HLS https。
     *   排查「某个台连不上」时先看它落在哪一格。
     *   ★ 放在这里而不是 app_radio_init() 里，是因为那时台单还没载入。*/
    {
        int n_dir_http = 0, n_dir_https = 0, n_hls_http = 0, n_hls_https = 0;
        for (int i = 0; i < s_count; i++) {
            const char *u = s_pool[i].url;
            int https = (strncmp(u, "https://", 8) == 0);
            if (strstr(u, ".m3u8")) { if (https) n_hls_https++; else n_hls_http++; }
            else                      { if (https) n_dir_https++; else n_dir_http++; }
        }
        ESP_LOGI(TAG, "台单构成: %d 台 = 直连 %d(http %d + https %d) + HLS %d(http %d + https %d)",
                 s_count,
                 n_dir_http + n_dir_https, n_dir_http, n_dir_https,
                 n_hls_http + n_hls_https, n_hls_http, n_hls_https);
    }

    for (int i = 0; i < 3 && i < s_count; i++)
        ESP_LOGI(TAG, "  [%d] %s | cat=%u prov=%u | %s",
                 i, s_pool[i].name, s_pool[i].cat, s_pool[i].prov, s_pool[i].url);
}
