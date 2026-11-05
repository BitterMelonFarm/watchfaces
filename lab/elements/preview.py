#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="a", hour=12, minute=34, sec=21, wday=3, mday=5, mon=10),
    dict(name="b", hour=8, minute=26, sec=7, wday=4, mday=14, mon=2, tick=1),
    dict(name="c", hour=17, minute=53, sec=44, wday=0, mday=29, mon=7),
    dict(name="d", hour=23, minute=59, sec=59, wday=6, mday=31, mon=11),
    dict(name="steps", hour=6, minute=9, sec=3, steps=6100, aux=1),
    dict(name="goal", hour=19, minute=20, sec=30, steps=8800, aux=1, batt=15),
    dict(name="zero", hour=0, minute=0, sec=0, wday=1, mday=1, mon=0),
    dict(name="pm", hour=15, minute=45, sec=10, h24=0),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
