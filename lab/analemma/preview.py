#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="a", hour=12, minute=34, sec=10, wday=3, mday=5, mon=10, year=2026),
    dict(name="b", hour=7, minute=9, sec=1, wday=0, mday=21, mon=5, year=2026, tick=1),
    dict(name="c", hour=18, minute=47, sec=22, wday=2, mday=21, mon=11, year=2026, aux=1),
    dict(name="d", hour=9, minute=15, sec=40, wday=6, mday=14, mon=1, year=2027, tick=2),
    dict(name="e", hour=15, minute=2, sec=33, aux=1, mday=3, mon=2),
    dict(name="f", hour=21, minute=30, sec=17, tick=1),
    dict(name="g", hour=4, minute=58, sec=5, aux=2, h24=0, batt=15),
    dict(name="h", hour=23, minute=11, sec=50, aux=0, tick=1, mday=29, mon=1, year=2028),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
