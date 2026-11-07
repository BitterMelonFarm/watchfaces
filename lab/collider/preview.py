#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="a", hour=10, minute=9, sec=21, wday=3, mday=7, mon=10, steps=4300),
    dict(name="b", hour=12, minute=0, sec=2, wday=0, mday=7, mon=10, steps=8200, tick=1),       # collision
    dict(name="c", hour=3, minute=47, sec=40, wday=5, mday=21, mon=2, batt=15, tick=2),
    dict(name="d", hour=7, minute=22, sec=55, wday=1, mday=1, mon=0, steps=900),
    dict(name="e", hour=1, minute=5, sec=10, tick=3),                                           # another collision
    dict(name="t1", hour=14, minute=27, sec=8, aux=1),
    dict(name="t2", hour=14, minute=27, sec=9, aux=1, tick=1),
    dict(name="pm", hour=18, minute=45, sec=33, h24=0),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
