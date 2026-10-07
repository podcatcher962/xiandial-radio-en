# Flashing Guide

> English firmware repository → [README.md](README.md)

## Hardware requirements

| Item | Spec |
|---|---|
| MCU | **ESP32-S3** |
| Display | 3.5" IPS TFT, 480×320, ST77922 |
| Touch | FT5x06 capacitive (I2C) |
| PSRAM | 8 MB |
| Flash | 16 MB |
| Port | Onboard USB (USB Serial/JTAG) |

**Use a USB cable that carries data** (some cables are charge-only). If no serial port shows up,
swap the cable first — that is the most common cause.

---

## ⚠️ Read this before flashing, or you may brick the boot

The ESP32-S3 stores **three** segments at fixed addresses:

| Address | Content | File |
|---|---|---|
| `0x0` | bootloader | `bootloader.bin` |
| `0x8000` | partition table | `partition-table.bin` |
| `0x10000` | application | `xiandial-radio-en.bin` |

**Flashing `xiandial-radio-en.bin` alone to `0x0` is wrong** — that writes the application into the
bootloader area and the board will not boot (no serial log, black screen). The flashing tool does
**not** report an error in that case.

If you would rather not memorize addresses, **use the merged image
`XianDial-EN-v1.51-merged.bin`**: all three segments in one file, flash at `0x0`, done.

---

## Method A — Web flasher (no tools to install)

1. Open <https://espressif.github.io/esptool-js/> (Chrome or Edge)
2. Click `Connect` and pick your serial port
3. Select **`XianDial-EN-v1.51-merged.bin`**
4. Chip type **ESP32-S3**, address **`0x0`**
5. Click `Start` and wait

> Requires a recent Chrome / Edge (Web Serial support).

---

## Method B — Command line

```bash
pip install esptool

# find your port
#   Windows : COM3
#   macOS   : /dev/cu.usbmodem*  or /dev/tty.usbserial-*
#   Linux   : /dev/ttyACM0
esptool.py --list-ports
```

Replace `COM3` below with your port.

### B1 · Merged image (recommended)

```bash
# when switching versions, erase first (this clears WiFi config and favorites)
esptool.py --chip esp32s3 --port COM3 erase_flash

esptool.py --chip esp32s3 --port COM3 --baud 460800 \
    write_flash 0x0 XianDial-EN-v1.51-merged.bin
```

### B2 · The three segments

```bash
esptool.py --chip esp32s3 --port COM3 --baud 460800 write_flash \
    0x0     bootloader.bin \
    0x8000  partition-table.bin \
    0x10000 xiandial-radio-en.bin
```

### Watching the log

```bash
esptool.py --chip esp32s3 --port COM3 monitor
```

A healthy boot prints the version, the station count and the WiFi state.
**"0 stations" is expected** — this build ships no station URLs at all; you import your own.

Leave the monitor with `Ctrl+]`.

---

## First boot after flashing

1. Once the screen is up, join the WiFi hotspot the device broadcasts (name like `XianDial-XXXX`)
2. Open `http://192.168.4.1` in a browser, enter your home WiFi name and password, submit
3. The device reboots and is ready

**You can skip WiFi setup** — the device simply stays at "0 stations" waiting for your station list.
It never shows a blank screen.

---

## Importing stations

**① Build the list** — open `XianForge.en.html` (double-click, works offline, uploads nothing),
drop your M3U / TXT file in, export `stations.tsv`.

**② Put it on the card** — copy `stations.tsv` to the **root** of a TF card.

**③ Insert the card and power on** — the device reads it automatically.

An unreadable card or a malformed file never blanks the screen: the device stays at "0 stations"
and tells you to import.

### TF card requirements

| Item | Requirement |
|---|---|
| Filesystem | **FAT32** |
| Capacity | 8–32 GB is plenty (the station list is ~13 KB) |

> ⚠️ **exFAT is not supported by this firmware.** Cards of 64 GB and larger ship formatted as
> exFAT, so they must be **reformatted to FAT32** before use — otherwise the card mounts nowhere
> and the device falls back to "0 stations". Windows' built-in format dialog refuses FAT32 above
> 32 GB; use the command line (`diskpart` → `format fs=fat32 quick`) or a tool such as
> GUIFormat / DiskGenius, and leave the cluster size at the recommended 32 KB.
>
> FAT32 also caps a single file at 4 GB — irrelevant here (audio files are far smaller).

### No card reader?

The onboard Type-C port cannot act as a USB host (its PHY belongs to USB Serial/JTAG, and VBUS is
input-only), so a PC cannot read the onboard TF card. Push files over the serial port instead:

```bash
pip install pyserial

python tools/_sd_push.py stations.tsv     # push the station list to the card
python tools/_sd_cmd.py  st_reload        # re-read the list and rebuild the UI, no reboot
python tools/_sd_cmd.py  st_ls            # list files on the card
```

Serial commands supported by the firmware:
`st_dump` `st_reload` `st_ls` `st_cat <file>` `st_rm <file>` `st_put <file>` `st_play <index>` `st_net <url>`

> `st_put` sends a file line by line (terminated by a single `.` on its own line) — fine for small
> lists. For big files insert the card and use `st_reload`.

---

## stations.tsv format

Path is fixed: **`/sdcard/stations.tsv`** (TF card root).

```
name<TAB>category<TAB>region<TAB>URL
```

| Item | Requirement |
|---|---|
| Encoding | UTF-8 **without BOM** |
| Line ending | LF (**not** CRLF) |
| Separator | **TAB** (not spaces) |
| Max lines | 2000 |
| Blank lines / lines starting with `#` | ignored, usable as comments |

> ★ **The category and region columns must use the fixed Chinese tokens** listed in the README
> (e.g. `新闻综合`, `北京`). They are protocol keys matched byte-for-byte by the firmware, not
> display text — the English UI shows them in English, but the file itself must contain the
> Chinese tokens. `XianForge.en.html` writes them for you, so this only matters if you hand-edit
> the file.
>
> ★ **BOM and CRLF are the two classic traps**: a BOM adds a stray box to the first station name;
> CRLF leaves a trailing `\r` on every URL, which shows up as "most stations work, these few never
> do" and is very hard to diagnose. Exporting from `XianForge.en.html` avoids both.

---

## FAQ

| Symptom | Cause |
|---|---|
| No board in the port list | Use a **data** cable; or hold `BOOT` while plugging in |
| `Failed to connect` while flashing | Same as above; or enter download mode manually: hold `BOOT` → press `EN/RESET` → release `BOOT` |
| Black screen and no serial log after flashing | ★ Wrong address. Use the merged image, or re-check the three addresses in B2 |
| Boot loop | Incomplete flash — run `erase_flash` and flash again |
| Card not read / 0 stations | CRLF or BOM in the file; file not in the card root; or more than 2000 lines |
| Some stations never play | The stream itself is dead (see "how many actually play" in the README), not a firmware bug |

---

## Building from source

Requires [ESP-IDF 6.0+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html)
(this project is built with 6.1):

```bash
git clone <this repo> && cd xiandial-radio-en
idf.py set-target esp32s3
idf.py -p COM3 build flash monitor
```

Rebuilding the release image:

```bash
idf.py -B build_en build
esptool --chip esp32s3 merge-bin --format raw -o XianDial-EN-v1.51-merged.bin \
    --flash-mode dio --flash-size 16MB --flash-freq 80m \
    0x0 build_en/bootloader/bootloader.bin \
    0x8000 build_en/partition_table/partition-table.bin \
    0x10000 build_en/xiandial-radio-en.bin
```

---

© Lanlan Eternal
