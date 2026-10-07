# XianDial — English firmware build

**The same firmware as the Chinese build, with an English interface. Zero built-in stations — you bring the stations.**

This is the **English edition** for international users. The playback engine, station import, SD-card handling, favourites, sleep timer and WiFi setup are **identical** to the Chinese build (same source, translated strings). What differs is the interface language and the fonts that back it.

- **Hardware:** ESP32-S3 + 3.5" IPS TFT touchscreen (480×320, ST77922 + FT5x06 capacitive touch)
- **Audio:** I2S DAC → amplifier. Plays MP3 / AAC / FLAC / OGG, plus HLS (`.m3u8`) live streams
- **Stations:** read **your own** station list from a TF card (**0 stations built in**)
- **Interface:** English. **Chinese version:** [`xiandial-radio`](https://github.com/podcatcher962/xiandial-radio) (README in both languages)

---

## What "English build" means here, precisely

This is worth being exact about, because it decides which text stays Chinese:

| Layer | Language | Why |
|---|---|---|
| Menus, buttons, labels, error messages, WiFi setup web page | **English** | This is what you read |
| Category / region identifiers in `stations.tsv` | **Simplified Chinese** | They are **parsing keys**, not display text — see below |
| Station names you supply | **Whatever your file contains** | Runtime data |

**The category/region tokens stay Chinese on purpose.** The firmware matches them with `strcmp`:

```c
/* app_stlist.c */
if (strcmp(field, g_cat_name[i]) == 0) return i;
if (strcmp(field, g_prov_name[i]) == 0) return i;
```

If they were translated to `"News"`, importing would still succeed, every field would still validate, the UI would still look fine, and **every format check would still pass** — but every station would fall through to the "other" bucket. So the protocol tables are untouched, and a **separate English display table** sits beside them (`xs_cat_name_en` / `xs_prov_short_en` / `xs_prov_long_en`).

**Practical consequence: `XianForge.en.html` produces a `stations.tsv` that works on both the English and Chinese builds, byte for byte.** The same file, same firmware behaviour, different interface language.

---

## Fonts: what this build ships

| Font | Size | Glyphs | Purpose |
|---|---|---|---|
| `xs_font_en12/16/24.c` | 12/16/24 px | ASCII + 37 symbols (183 KB total) | **UI chrome** — this build's English interface is tiny |
| `xs_font_st12/16.c` | 12/16 px | ASCII + **GB2312 level-1, 3755 chars** | **Station names** from your `stations.tsv` |
| `xs_font_cjk12.c` | 12 px, 2bpp | GB2312 full set | SD card **file names** |

Why the UI font shrank to 183 KB while the Chinese build's is 1.79 MB: English UI text is ASCII. A pixel font scaled to English needs almost nothing.

Why the station-name font **stays Chinese** even in the English build: station names arrive at runtime from your file, so their glyphs must be baked at build time. Chinese-language station lists are exactly what a lot of international users of this device will have — including Chinese-language stations carried on overseas services. The glyphs cover Simplified Chinese.

### ⚠️ Station names outside Latin + Simplified Chinese

The station-name font covers ASCII and the 3755 level-1 characters of GB2312.

- ✅ **English, Spanish, French, German, Italian, Portuguese, Dutch, Vietnamese, Indonesian, Malay, Turkish, Polish** — Latin letters are ASCII, these render correctly.
- ❌ **Cyrillic (Russian, Ukrainian, Bulgarian, Serbian), Greek, Arabic, Hebrew, Devanagari, Thai, Hangul, Kana** — these render as **boxes (□)**.

Not a code bug — a size tradeoff. A full Unicode font would multiply the firmware size several times over.

**If your stations have Cyrillic or kana names, rebuild the station-name font:**

```bash
# 1. Put your stations.tsv where the generator can read it
#    (edit ST_SRC in gen_fonts_st.py to point at your copy)
python gen_fonts_st.py --publish
# 2. Copy the two produced files into the build's font directory
cp main/fonts/xs_font_pub_st12.c main/fonts/xs_font_pub_st16.c en/main/fonts/
# 3. Rebuild
python _build_en.py
```

This also lets you pick up rare Simplified characters outside GB2312 level-1. Both fonts are generated from **OFL-1.1** fonts (Ark Pixel 12px + Source Han Sans SC), so redistribution stays legal.

---

## Flashing

**Full step-by-step guide: [FLASHING.md](FLASHING.md)**

**You need:**

| Item | Notes |
|---|---|
| An ESP32-S3 board | With a 3.5" IPS TFT (ST77922 + FT5x06). **This firmware targets that display only.** |
| A USB **data** cable | Onboard USB. The Type-C PHY is taken by Serial-JTAG, so the board **cannot act as a USB host** (no USB drives, no card readers). |
| A computer | Windows / macOS / Linux |
| A TF card | Optional — you can push the station list over serial instead. |

**Three files go into the flash, at fixed offsets:**

| Offset | Content | File |
|---|---|---|
| `0x0` | Bootloader | `bootloader.bin` |
| `0x8000` | Partition table | `partition-table.bin` |
| `0x10000` | Application | **`xiandial-radio-en.bin`** |

> ⚠️ **The most common beginner mistake: flashing the application to `0x0`.** That writes it into the bootloader region and the board will not boot (no serial log, black screen).

**★ Easiest — use the merged image.** All three segments in one file, flash at `0x0`, no address mistakes possible:

**Easiest path — web flasher, nothing to install:**

Open [ESP Web Tools](https://espressif.github.io/esptool-js/) → `Connect` → pick your port →
select **`XianDial-EN-v1.51-merged.bin`** → chip **ESP32-S3**, address **`0x0`** → `Start`.

**Command line:**

```bash
pip install esptool
esptool.py --list-ports

# Recommended when upgrading versions (clears WiFi config and favourites)
esptool.py --chip esp32s3 --port COM3 erase_flash

esptool.py --chip esp32s3 --port COM3 --baud 460800 \
    write_flash 0x0 XianDial-EN-v1.51-merged.bin

esptool.py --chip esp32s3 --port COM3 monitor
```

> ⚠️ **Flashing overwrites whatever is already on the chip.** There is **no OTA partition**, so every upgrade means flashing again.

### First boot

1. The screen shows an English WiFi setup page. Join the hotspot `XianDial-XXXX`; `http://192.168.4.1` opens a page where you pick your WiFi and enter the password.
2. It works without WiFi too — the device sits at "0 stations" waiting for your list rather than showing a blank screen.

---

## Adding stations: three steps

```
①  Open XianForge.en.html (single HTML file, double-click to run)  →  export stations.tsv
②  Copy stations.tsv to the root of a TF card
③  Insert the card and power on — the device reads it
```

A missing card, an unreadable card, or a malformed file will never produce a blank screen: the device stays at "0 stations" and tells you to import.

**Hot reload** works with the card inserted: send `st_reload` over serial to re-read without rebooting, or `st_put stations.tsv` to overwrite over serial.

**The favourites list is empty on first boot — this is expected.** The built-in list is empty and the author's favourites were not shipped.

---

## Station file format

Path must be **`/sdcard/stations.tsv`** (TF card root).

```
Station Name<TAB>Category<TAB>Region<TAB>URL
```

| Requirement | Value |
|---|---|
| Encoding | UTF-8, **no BOM** |
| Line endings | LF (**not** CRLF) |
| Separator | **TAB** (not spaces) |
| Max entries | 2000 |
| Blank lines / `#` comments | Ignored |

**Why BOM and CRLF matter so much:** a BOM puts a stray box glyph in front of the first station name and breaks name-based favourite matching. CRLF leaves a trailing `\r` on the URL — the symptom is "every other station works, but these few don't", which is miserable to debug.

Category and region values (the converter infers them, so you rarely need to care) — **these are Simplified Chinese in every build, including this one**:

- **Category (13):** `新闻综合` `交通台` `音乐` `文艺` `说书` `戏曲` `怀旧老歌` `网络台` `教育台` `电视伴音` `综合` `宗教` `境外新闻`
- **Region (42):** `北京` … `新疆` `中国台湾` `中国香港` `中国澳门` `其他华语` `北美` `欧洲` `日韩` `新马` `东南亚` `大洋洲` `海外中文`
- Empty region or `其他` → treated as a nationwide station

> These tokens are **not** display strings. The interface shows `News` / `Traffic` / `Music` … and `Beijing` / `Shanghai` / `Hong Kong, China` … from the English display tables, while the identifiers above stay exactly as they are. Your `stations.tsv` is therefore portable between the English and Chinese builds.

---

## How many of your stations will play

This shouldn't be vague, so here's the model:

```
playable ≈ total entries × format support rate × server reachability × https pass rate
```

| Factor | What drives it | Typical |
|---|---|---|
| **Format support** | Direct MP3/AAC/FLAC/OGG and HLS (`.m3u8`). **Not supported:** nested `.m3u`/`.pls` playlists, audiobook formats, streams needing a `Referer` header, authenticated streams, non-standard containers | ~85% |
| **Server reachability** | Is the origin still alive? Does it allow your IP? Region or rate limits? | 70–85% |
| **https pass rate** | https costs TLS buffers. **Fixed in this build** (v1.51 shrank the mbedTLS single-connection buffer 16 KB → 4 KB). Failures now are almost always certificate-chain problems | near 100% after the fix |

> ⚠️ **In v1.50 and earlier, https stations were essentially all broken.** Root cause: the largest *contiguous* block of internal RAM was too small for TLS (this is not the same as "free memory is too low" — checking free memory gives you the opposite conclusion). **That is fixed here.**

**Plan for ~70% and treat anything above that as a bonus.**

### When a station won't play

Use the serial console to find out which layer fails (**the `?insecure` suffix only exists in self-compiled builds** — it's compiled out of the release):

```
st_net http://your-stream-url
```

The log tells you which layer broke: **DNS / TCP / TLS / HTTP / decode**. If any of the first four fails, it isn't the firmware's fault.

- `403` → needs a `Referer` or UA spoof. Not supported.
- `open` fails + TLS error → certificate chain problem (expired or self-signed).
- `200` but nothing after `#EXTM3U` → the server responds but serves no data.
- Decode errors → codec the firmware doesn't handle (some WMA and AMR variants).

---

## Why the station list is empty

The built-in count is **0**. That's deliberate:

1. A list of real radio stream URLs is this project's single biggest public risk (provenance and copyright). Publishing one would make the repo a distribution channel for somebody else's servers.
2. The author's own list was built years ago with `http` + `.ts` segments to save RAM. **Most M3U files available today are `https` + raw ADTS AAC** — different code paths entirely. Shipping a "starter list" would actively mislead you.

**Division of labour: the firmware provides the interface, the tool builds the list, the URLs are yours.**

---

## Companion tool

**`XianForge.en.html`** — a single HTML file. Double-click to run. **No network, no uploads, nothing leaves your machine.**

- Reads M3U (any `#EXTINF` variant) and plain-text lists
- Auto-detects UTF-8 / GBK / UTF-16
- Infers category and region from the station name
- De-duplicates by URL (multiple mirrors of the same station are all kept)
- Exports `stations.tsv`

**Run the self-test after any change** (needs Node.js, no browser):

```bash
node tools/_xf_selftest.js
```

31 assertions covering category inference (including the very common `XX People's Broadcasting Station` pattern and the TV-audio priority rule), M3U parsing, URL de-duplication, TSV four-field compliance, and edge cases such as CRLF, a trailing CR, missing names, and rows with no URL. Non-zero exit code means something is wrong.

> This self-test is not ceremony. It caught a real bug during development: the category rule table only listed `中央人民广播电台`, so every `北京人民广播电台` was classified as the fallback `综合`. The build was clean and the UI showed nothing wrong.

### _sd_push.py — serial card push (optional, in `tools/`)

The onboard TF slot runs in SDIO, so Windows can't see the card; and the Type-C PHY is occupied by Serial-JTAG, so the board can't act as a USB host. The firmware therefore exposes a serial channel that can read and write the card directly:

```bash
pip install pyserial

python tools/_sd_push.py stations.tsv     # push the list (with flow control and a line-count check)
python tools/_sd_cmd.py   st_reload       # re-read and rebuild the UI, no reboot
python tools/_sd_cmd.py   st_ls           # list files on the card
```

Supported serial commands: `st_dump` `st_reload` `st_ls` `st_cat <file>` `st_rm <file>` `st_put <file>` (line-by-line, a lone `.` terminates) `st_play <index>` `st_net <url>` (layered network test).

File writes go to a `.tmp` first and are then renamed, so a dropped connection never leaves a half-written file behind.

---

## Hardware specification

The configuration verified on the author's own unit. **This firmware has only been tested on this hardware** — a different display, touch controller, or audio path means code changes.

| Item | Spec |
|---|---|
| MCU | **ESP32-S3** (512 KB SRAM, 8 MB PSRAM) |
| Display | **3.5" IPS TFT**, 480×320, **ST77922**, SPI |
| Touch | **FT5x06** capacitive (I2C), 5 points |
| Audio | I2S DAC → amplifier (**+5 V**), 4 Ω 3 W mono speaker |
| Storage | Onboard **TF slot** (SDIO); 16 MB flash for firmware |
| Network | 2.4 GHz WiFi + Bluetooth (on-module) |
| Power | 3.7 V Li-ion + LDO, LDO-only ⇒ ~2 hours from 1000 mAh |
| Sleep | Deep sleep, **RTC wake from IO0–21 only** (BOOT button = IO0) |
| WiFi setup | **SoftAP + captive portal** (IO45/46 cannot wake from deep sleep) |
| Framework | ESP-IDF 6.1 + LVGL 9 (`LV_MEM_SIZE` = 112 KB) |
| Partitions | No OTA; upgrading requires a reflash |

**Known hardware constraints (read before changing boards):**

- The onboard Type-C USB PHY is occupied by Serial-JTAG, so the board **cannot act as a USB host** — external USB drives and card readers do not work. Use the onboard TF slot.
- The TF card is on **SDIO** and is not visible when the board is plugged into a PC. On a desktop, use a USB card reader.
- Power-off relies on a hardware quirk (Q3's gate is not wired to the MCU), so **no GPIO can actually cut power**. "Off" means display off + WiFi off + deep sleep.

---

## Building from source

The English build is **generated from the Chinese build's source**, not hand-maintained in parallel. That matters: the playback and import logic must stay identical, and hand-maintaining two copies guarantees they diverge.

```bash
git clone <this repo>
cd xiandial-radio-en

# 1. Generate the English sources from the Chinese ones
python tools/_gen_fw_en.py

# 2. Generate the fonts (English UI font + reuse the generic station fonts)
cd .. && python gen_fonts_en.py

# 3. Build
cd xiandial-radio/en && idf.py set-target esp32s3 && idf.py -B build_en build
```

Or use the wrapper, which also runs the pre-flight gates:

```bash
python _build_en.py          # idf.py -B build_en build
python _build_en.py ninja    # incremental, much faster after UI changes
```

Requires ESP-IDF **6.0+** (built with **6.1**).

### How the translation works

| File | Role |
|---|---|
| `tools/_xf_fw_en_text.py` | **Single source of truth for English UI text.** ~300 entries. |
| `tools/_xf_fw_en_names.py` | English display names for categories (13) and regions (42, short + long forms) |
| `tools/_gen_fw_en.py` | The generator. Translates literals, replaces the captive-portal page wholesale, injects the display tables, and **fails loudly** if anything unexpected is found. |

Two hard rules the generator enforces:

1. **Protocol tokens are never translated.** `app_stlist.c` is compared against them with `strcmp`; translating them breaks station import silently. The generator only rewrites display sites, and it verifies that `strcmp(s, g_cat_name[i])` is byte-identical to the Chinese build.
2. **Serial log strings may stay Chinese.** Nobody reads them except someone debugging over serial, where Chinese is faster to read and costs no flash. The generator decides by call site — `lv_label_set_text` / `tap_note` / the setup web page count as UI; `ESP_LOG` does not.

If you add UI text to the Chinese build, add it to `_xf_fw_en_text.py`; the generator will fail on the untranslated entry rather than leaving Chinese on screen.

### What's in the repo, and what isn't

| Content | Status |
|---|---|
| Application source (English) | ✅ Complete, generated |
| UI fonts `xs_font_en12/16/24.c` | ✅ 183 KB (ASCII + symbols) |
| Station-name fonts `xs_font_st12/16.c` | ✅ Generic (GB2312 level-1, 3755 chars + ASCII) |
| SD-page font `xs_font_cjk12.c` | ✅ GB2312 full set, 2bpp |
| Release station list | ✅ **0 entries** |
| Real station lists, fonts baked from real station names, source TTFs | ❌ Not included |

---

## Disclaimer

**Please read this before you flash anything.**

### What you are using

This is the firmware for a **personal open-source hardware project**. The author was building one internet radio for himself and tidied the code into a public repository. It is **not a commercial product**, and it has had **no commercial certification, no stress testing, and no long-term maintenance**.

### What the author does not promise

- **That your stations will play.** Whether a station works depends on the source itself — container format, whether the server accepts your client at all, certificate validity, and whether the bitrate is one the device can sustain. This firmware handles the mainstream formats only. This build ships **zero** stations, so there is not a single address here that is known to work.
- **Stability.** There is no OTA; upgrading means reflashing. It has been verified on **the author's own unit only** — your board revision, display, touch controller and TF card may all differ.
- **That it is free of bugs.** It is a one-person project with no test team.

### What is your responsibility

- **Flashing overwrites whatever was already on the chip.** Make sure you know what you are erasing.
- **Station addresses belong to whoever publishes them.** This repository **ships no station addresses at all** (built-in count: 0). Any M3U, station list or stream URL you obtain from anywhere is yours to judge for legality — the author is not responsible for what you do with it.
- **Obey the law where you are, and the copyright rules covering the audio you stream.**
- **Your own data is your own responsibility.** The firmware reports nothing about you, but the traffic levels and the IP address involved are yours.

### Limits of the author's liability

- The source is open and **MIT-licensed, with no warranty of any kind** — see `LICENSE`.
  ※ The MIT text requires the copyright notice to be kept: if you fork or redistribute this, retain the attribution at the bottom of this file.
- Fonts are under their respective OFL-1.1 licences (`OFL-1.1-*.txt`).
- **The author accepts no liability for any direct or indirect loss arising from the use of this firmware.**

### The one-sentence version

> This is a **reference implementation of a complete workflow**, not a product that is guaranteed to work. It shows you how an internet radio can be put together; the rest of the road is yours.

---

## License and credits

**Firmware source:** see [LICENSE](LICENSE).

**Fonts:** generated from Ark Pixel 12px and Source Han Sans SC, both **SIL Open Font License 1.1** — [OFL-1.1-Ark-Pixel.txt](OFL-1.1-Ark-Pixel.txt) · [OFL-1.1-Source-Han-Sans.txt](OFL-1.1-Source-Han-Sans.txt). OFL permits embedding and subsetting in firmware.

**Author:** © Lanlan Eternal · *永远的兰兰*

**This is a personal open-source project, not affiliated with any broadcaster.** All station URLs come from you. The firmware ships with no stations, no analytics, no telemetry, and no network calls other than the ones your station list triggers.

---

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| Black screen, no serial log | You flashed the application at `0x0` instead of `0x10000`, or didn't flash the bootloader at all. Use the **merged** image at `0x0`. |
| Screen shows UI but no station plays | Import a `stations.tsv` first — the built-in count is 0. Then run `st_net <url>` on the serial console to see which layer fails. |
| Station name shows □ | Glyph outside the shipped font (see *Station names outside Latin + Simplified Chinese*). Rebuild `xs_font_st12/16.c`. |
| Import succeeds but every station lands in "other" | The category/region columns were edited by hand. They must match the fixed tokens listed above exactly. |
| WiFi page won't open | Join the `XianDial-XXXX` hotspot first; the page is at `http://192.168.4.1`. |
| Device reboots after deep sleep | The BOOT button must be **held** at power-on (IO0 low). Also: **do not** wire anything to IO45/46 expecting wake — the S3's RTC GPIOs are IO0–21 only. |