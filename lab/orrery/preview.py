#!/usr/bin/env python3
"""python3 preview.py OUTDIR  -> renders docs/preview.png style sheet"""
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="today", hour=12, minute=34, sec=21, wday=3, mday=5, mon=10, year=2026),
    dict(name="inner", hour=9, minute=41, sec=2, wday=3, mday=5, mon=10, year=2026, aux=1, tick=1),
    dict(name="j2000", hour=12, minute=0, sec=0, wday=6, mday=1, mon=0, year=2000, batt=15),
    dict(name="newyear", hour=0, minute=0, sec=0, wday=4, mday=1, mon=0, year=2027, batt=95, tick=4),
    dict(name="mar2025", hour=18, minute=45, sec=33, wday=6, mday=14, mon=2, year=2025, tick=2),
    dict(name="fullmoon", hour=23, minute=5, sec=48, wday=1, mday=24, mon=11, year=2026, tick=5),
    dict(name="pm12h", hour=15, minute=7, sec=59, wday=2, mday=3, mon=6, year=2026, h24=0),
    dict(name="future", hour=6, minute=15, sec=10, wday=0, mday=20, mon=7, year=2031, tick=0),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
