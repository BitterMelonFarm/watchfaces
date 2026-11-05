#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="a", hour=12, minute=34, sec=10, wday=3, mday=5, mon=10),
    dict(name="b", hour=12, minute=34, sec=25, wday=3, mday=5, mon=10, tick=1),
    dict(name="c", hour=18, minute=5, sec=47, wday=6, mday=8, mon=2, tick=2),
    dict(name="d", hour=3, minute=48, sec=2, wday=0, mday=14, mon=6, tick=3, batt=15),
    dict(name="e", hour=9, minute=15, sec=30, aux=1),
    dict(name="f", hour=21, minute=45, sec=55, aux=1, tick=1),
    dict(name="g", hour=7, minute=20, sec=18, aux=2),
    dict(name="pm", hour=15, minute=59, sec=40, aux=2, h24=0, tick=1),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
