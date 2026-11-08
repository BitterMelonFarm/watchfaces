#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "..", "lab", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="dawn", hour=6, minute=15, sec=10, wday=3, mday=8, mon=10, steps=1200),
    dict(name="day", hour=10, minute=34, sec=21, wday=3, mday=8, mon=10, steps=5300, tick=2),
    dict(name="noon", hour=13, minute=5, sec=44, wday=3, mday=8, mon=10, steps=8400, tick=4, aux=1),
    dict(name="golden", hour=17, minute=40, sec=5, wday=5, mday=14, mon=5, steps=3100, tick=1),
    dict(name="dusk", hour=19, minute=35, sec=30, wday=6, mday=3, mon=7, steps=7200, tick=3),
    dict(name="night", hour=23, minute=12, sec=50, wday=0, mday=1, mon=0, steps=900, batt=15),
    dict(name="late", hour=3, minute=48, sec=2, steps=0, tick=5),
    dict(name="pm", hour=15, minute=7, sec=18, h24=0, steps=6500),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
