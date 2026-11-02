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

def frame(name, hour, wday, minute=9, steps=3000, batt=80, tick=0, species=0):
    r = subprocess.run([exe, *map(str, (hour, wday, minute, steps, batt, tick, species))], capture_output=True, check=True)
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
    ("sleep", 23, 2, 30, 3000, 80, 3), ("wake", 7, 2, 10, 100, 80, 0),
    ("brush", 7, 2, 40, 100, 80, 1), ("commute", 8, 2, 10, 500, 80, 3),
    ("code", 10, 2, 9, 3000, 80, 2), ("meeting", 11, 2, 10, 3500, 80, 4),
    ("lunch", 12, 2, 15, 3500, 60, 1), ("dinner", 18, 2, 30, 6000, 40, 1),
    ("play", 19, 2, 0, 5000, 80, 5), ("bath", 20, 2, 30, 6500, 70, 1),
    ("read", 21, 2, 30, 6500, 70, 4), ("walk", 10, 6, 0, 2000, 90, 5),
    ("game", 15, 0, 30, 3000, 80, 3), ("chill", 13, 0, 0, 3000, 80, 4),
    ("code_sad", 14, 2, 0, 400, 80, 2), ("happy", 19, 2, 0, 8500, 100, 1),
]
ims = [frame(*s) for s in SCENES]
sheet = Image.new("RGB", (260 * 4, 260 * 4))
for i, im in enumerate(ims):
    sheet.paste(im, ((i % 4) * 260, (i // 4) * 260))
sheet.save(os.path.join(out, "sheet.png"))
# One row per species: code, play, sad-at-desk, happy.
NAMES = ["blob", "cat", "bunny", "bear", "frog", "cthulhu", "shoggoth", "deepone", "eye"]
rows = []
for sp, n in enumerate(NAMES):
    rows.append([frame(f"{n}_code", 10, 2, 9, 3000, 80, 2, sp), frame(f"{n}_play", 19, 2, 0, 5000, 80, 5, sp),
                 frame(f"{n}_sad", 14, 2, 0, 400, 80, 3, sp), frame(f"{n}_happy", 10, 6, 0, 8500, 80, 1, sp)])
for fname, sel in (("pets.png", rows[:5]), ("eldritch.png", rows[5:])):
    sheet2 = Image.new("RGB", (260 * 4, 260 * len(sel)))
    for r_i, row in enumerate(sel):
        for c_i, im in enumerate(row):
            sheet2.paste(im, (c_i * 260, r_i * 260))
    sheet2.save(os.path.join(out, fname))
print("wrote", out)
