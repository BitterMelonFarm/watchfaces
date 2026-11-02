#!/usr/bin/env python3
"""Render Neon Grid on the host: python3 render.py OUTDIR  (needs gcc + Pillow).
Compiles the real src/c/main.c against a stub Pebble API. There are no system
fonts involved (everything is drawn with rectangles), so the output matches the watch."""
import os, subprocess, sys
from PIL import Image, ImageDraw

here = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
SW = 260
os.makedirs(out, exist_ok=True)
exe = os.path.join(out, "render")
subprocess.run(["gcc", "-O1", "-Wall", f"-DSW={SW}", "-I", here, "-o", exe, os.path.join(here, "render.c")], check=True)

def col(a):
    return tuple(((a >> s) & 3) * 85 for s in (4, 2, 0))

def frame(name, hour=12, minute=34, sec=21, wday=2, mday=2, mon=10, steps=4300, batt=80, tick=3, theme=0, glitch=0, h24=1, mask=True):
    r = subprocess.run([exe, *map(str, (hour, minute, sec, wday, mday, mon, steps, batt, tick, theme, glitch, h24))], capture_output=True, check=True)
    im = Image.new("RGB", (SW, SW))
    px = im.load()
    for y in range(SW):
        for x in range(SW):
            px[x, y] = col(r.stdout[y * SW + x])
    if mask:
        m = Image.new("L", (SW, SW), 0)
        ImageDraw.Draw(m).ellipse((0, 0, SW - 1, SW - 1), fill=255)
        bg = Image.new("RGB", (SW, SW), (24, 24, 24))
        bg.paste(im, mask=m)
        im = bg
    im.save(os.path.join(out, name + ".png"))
    return im

SCENES = [
    dict(name="neon_day", hour=12, minute=34, sec=21, theme=0, steps=4300, batt=80, tick=3),
    dict(name="neon_night", hour=23, minute=7, sec=48, wday=5, mday=6, theme=0, steps=9100, batt=62, tick=5),
    dict(name="blade", hour=18, minute=45, sec=9, wday=6, mday=7, theme=1, steps=6200, batt=44, tick=2),
    dict(name="matrix", hour=3, minute=14, sec=59, wday=0, mday=8, theme=2, steps=800, batt=91, tick=6),
    dict(name="glitch", hour=9, minute=41, sec=2, theme=0, glitch=3, tick=1),
    dict(name="low_battery", hour=16, minute=20, sec=30, theme=1, steps=2100, batt=12, tick=4),
    dict(name="pm_12h", hour=15, minute=5, sec=40, theme=2, steps=7700, batt=55, tick=2, h24=0),
    dict(name="goal", hour=21, minute=30, sec=15, theme=0, steps=8420, batt=33, tick=5),
]
ims = [frame(**s) for s in SCENES]
sheet = Image.new("RGB", (SW * 4, SW * 2))
for i, im in enumerate(ims):
    sheet.paste(im, ((i % 4) * SW, (i // 4) * SW))
sheet.save(os.path.join(out, "sheet.png"))
print("wrote", out)
