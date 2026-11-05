#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="a", hour=12, minute=34, sec=56, wday=3, mday=5, mon=10),
    dict(name="b", hour=7, minute=9, sec=21, wday=6, mday=8, mon=2, tick=1),
    dict(name="c", hour=23, minute=59, sec=59, wday=0, mday=31, mon=11, aux=1),
    dict(name="d", hour=0, minute=0, sec=0, wday=1, mday=1, mon=0, aux=2, tick=1, batt=15),
    dict(name="e", hour=18, minute=45, sec=33, aux=1, tick=1),
    dict(name="f", hour=9, minute=58, sec=18, aux=2),
    dict(name="g", hour=21, minute=17, sec=42, aux=0),
    dict(name="pm", hour=15, minute=26, sec=8, h24=0),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
