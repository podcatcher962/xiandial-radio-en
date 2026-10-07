#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""往板子串口发几条命令并把日志打出来（临时诊断用，不做批处理）。

用法：python _sd_cmd.py st_ls
      python _sd_cmd.py st_cat stations.tsv
"""
import sys
import time

import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else "COM3"
CMDS = sys.argv[2:]

ser = serial.Serial(PORT, 115200, timeout=0.3)
ser.reset_input_buffer()
time.sleep(0.4)
try:
    while ser.read(8192):
        pass
except Exception:
    pass

for c in CMDS:
    print("\n>>> %s" % c)
    ser.write(c.encode() + b"\r\n")
    time.sleep(1.6)
    buf = b""
    end = time.time() + 1.6
    while time.time() < end:
        try:
            ch = ser.read(4096)
        except Exception:
            break
        if not ch:
            time.sleep(0.05)
            continue
        buf += ch
    for l in buf.decode("utf-8", "replace").splitlines():
        print("   ", l)

ser.close()
