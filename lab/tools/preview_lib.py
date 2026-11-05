"""Shared preview helpers for lab faces (needs gcc + Pillow).

    import preview_lib as P
    P.build(face_dir, out_dir)            # compile the face's real main.c against the stub API
    P.frame("name", hour=12, ...)         # render one frame -> PNG (round mask) and return the image
    P.sheet("sheet", images, cols)        # contact sheet
Everything is drawn with rectangles, so output matches the watch pixel for pixel.
"""
import os, subprocess
from PIL import Image, ImageDraw

SW = 260
_here = os.path.dirname(os.path.abspath(__file__))
_exe = None
_out = None

def build(face_dir, out_dir):
    global _exe, _out
    _out = out_dir
    os.makedirs(out_dir, exist_ok=True)
    _exe = os.path.join(out_dir, "render")
    src = os.path.abspath(os.path.join(face_dir, "src", "c", "main.c"))
    subprocess.run(["gcc", "-O1", "-Wall", f"-DSW={SW}", f'-DFACE_SRC="{src}"', "-I", _here,
                    "-o", _exe, os.path.join(_here, "render.c")], check=True)

def exe():
    return _exe

def col(a):
    return tuple(((a >> s) & 3) * 85 for s in (4, 2, 0))

def raw(hour=12, minute=34, sec=21, wday=2, mday=5, mon=10, year=2026, steps=4300, batt=80, tick=3, aux=0, focus=1, h24=1, exe_path=None):
    r = subprocess.run([exe_path or _exe, *map(str, (hour, minute, sec, wday, mday, mon, year, steps, batt, tick, aux, focus, h24))],
                       capture_output=True, check=True)
    return r.stdout

def frame(name, mask=True, **kw):
    data = raw(**kw)
    im = Image.new("RGB", (SW, SW))
    px = im.load()
    for y in range(SW):
        for x in range(SW):
            px[x, y] = col(data[y * SW + x])
    if mask:
        m = Image.new("L", (SW, SW), 0)
        ImageDraw.Draw(m).ellipse((0, 0, SW - 1, SW - 1), fill=255)
        bg = Image.new("RGB", (SW, SW), (24, 24, 24))
        bg.paste(im, mask=m)
        im = bg
    im.save(os.path.join(_out, name + ".png"))
    return im

def sheet(name, images, cols=4):
    rows = (len(images) + cols - 1) // cols
    s = Image.new("RGB", (SW * cols, SW * rows))
    for i, im in enumerate(images):
        s.paste(im, ((i % cols) * SW, (i // cols) * SW))
    s.save(os.path.join(_out, name + ".png"))
    return s

def sweep(face_dir, out_dir, n=2000, seed=5, aux_max=3):
    """AddressSanitizer + UBSan sweep over random states; returns (runs, failures)."""
    import random
    from concurrent.futures import ThreadPoolExecutor
    os.makedirs(out_dir, exist_ok=True)
    exe_path = os.path.join(out_dir, "render_asan")
    src = os.path.abspath(os.path.join(face_dir, "src", "c", "main.c"))
    subprocess.run(["gcc", "-g", "-O1", f"-DSW={SW}", f'-DFACE_SRC="{src}"', "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=undefined", "-I", _here, "-o", exe_path, os.path.join(_here, "render.c")],
                   check=True, capture_output=True)
    rnd = random.Random(seed)
    combos = [(rnd.randrange(24), rnd.randrange(60), rnd.randrange(60), rnd.randrange(7), rnd.randrange(1, 29), rnd.randrange(12),
               rnd.choice([2000, 2026, 2037, 2100, 1999]), rnd.choice([0, 1, 7999, 8000, 99999, rnd.randrange(20000)]),
               rnd.randrange(101), rnd.randrange(0, 600), rnd.randrange(aux_max + 1), rnd.choice([0, 1]), rnd.choice([0, 1]))
              for _ in range(n)]
    def run(c):
        r = subprocess.run([exe_path, *map(str, c)], capture_output=True)
        err = r.stderr.decode(errors="replace")
        return (c, err[:300]) if (r.returncode != 0 or "runtime error" in err or "AddressSanitizer" in err) else None
    with ThreadPoolExecutor(8) as ex:
        fails = [x for x in ex.map(run, combos) if x]
    return len(combos), fails
