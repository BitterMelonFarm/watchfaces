#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="a", hour=3, minute=5, sec=10, wday=3, mday=5, mon=10),
    dict(name="b", hour=3, minute=5, sec=20, wday=3, mday=5, mon=10, tick=1),
    dict(name="c", hour=9, minute=47, sec=33, wday=6, mday=8, mon=2, aux=1, tick=2),
    dict(name="d", hour=14, minute=20, sec=2, aux=2, tick=3),
    dict(name="e", hour=1, minute=30, sec=44, aux=0, batt=15),
    dict(name="f", hour=7, minute=59, sec=59, aux=1),
    dict(name="g", hour=11, minute=12, sec=8, aux=2, tick=1, h24=0),
    dict(name="h", hour=22, minute=41, sec=26, aux=0),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
