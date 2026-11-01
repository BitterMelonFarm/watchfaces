#!/usr/bin/env python3
"""Render Desk Pet scenes on the host: python3 render.py OUTDIR  (needs gcc + Pillow).
Compiles src/c/main.c against a stub Pebble API, so it is the real drawing code,
but text uses a stand-in font and there is no real firmware/emulator involved."""
import os, subprocess, sys
from PIL import Image, ImageDraw, ImageFont

here = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
os.makedirs(out, exist_ok=True)
exe = os.path.join(out, "render")
subprocess.run(["gcc", "-O1", "-Wall", "-I", here, "-o", exe, os.path.join(here, "render.c")], check=True)

def col(a):  # Pebble 6-bit color -> RGB
    return tuple(((a >> s) & 3) * 85 for s in (4, 2, 0))

def fnt(name):
    size = {"BITHAM_42_BOLD": 44, "GOTHIC_24_BOLD": 24, "GOTHIC_18_BOLD": 18}[name]
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",):
        if os.path.exists(p):
            return ImageFont.truetype(p, int(size * .8))
    return ImageFont.load_default()

def frame(name, hour, wday, minute=9, steps=3000, batt=80, tick=0):
    r = subprocess.run([exe, *map(str, (hour, wday, minute, steps, batt, tick))], capture_output=True, check=True)
    im = Image.new("RGB", (260, 260))
    px = im.load()
    for y in range(260):
        for x in range(260):
            px[x, y] = col(r.stdout[y * 260 + x])
    d = ImageDraw.Draw(im)
    for line in r.stderr.decode().splitlines():
        if not line.startswith("T|"): continue
        _, s, f, x, y, w, h, c, al = line.split("|")
        x, y, w, h = map(int, (x, y, w, h))
        ft = fnt(f)
        tw = d.textlength(s, font=ft)
        tx = x + (w - tw) / 2 if al == "1" else x
        d.text((tx, y + 4), s, font=ft, fill=col(int(c)))
    mask = Image.new("L", (260, 260), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, 259, 259), fill=255)
    round_im = Image.new("RGB", (260, 260), (30, 30, 30))
    round_im.paste(im, mask=mask)
    round_im.save(os.path.join(out, name + ".png"))
    return round_im

# (name, hour, weekday[0=Sun], minute, steps, batt, tick)
SCENES = [
    ("sleep", 23, 2, 30, 3000, 80, 3), ("wake", 7, 2, 40, 100, 80, 0),
    ("code", 10, 2, 9, 3000, 80, 2), ("lunch", 12, 2, 15, 3500, 60, 1),
    ("dinner", 17, 2, 30, 6000, 40, 1), ("play", 19, 2, 0, 8200, 20, 5),
    ("chill", 14, 0, 0, 3000, 80, 4), ("code_sad", 15, 2, 0, 400, 80, 2),
]
ims = [frame(*s) for s in SCENES]
sheet = Image.new("RGB", (260 * 4, 260 * 2))
for i, im in enumerate(ims):
    sheet.paste(im, ((i % 4) * 260, (i // 4) * 260))
sheet.save(os.path.join(out, "sheet.png"))
print("wrote", out)
