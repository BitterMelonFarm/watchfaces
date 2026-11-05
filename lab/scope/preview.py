#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [
    dict(name="a", hour=3, minute=12, sec=10, wday=3, mday=5),
    dict(name="b", hour=10, minute=47, sec=33, wday=3, mday=5, tick=2),
    dict(name="c", hour=7, minute=5, sec=50, wday=6, mday=8, tick=1),
    dict(name="d", hour=12, minute=30, sec=0, wday=1, mday=9),
    dict(name="f1", hour=3, minute=20, sec=5, aux=1),
    dict(name="f2", hour=9, minute=20, sec=22, aux=1, tick=1),
    dict(name="f3", hour=12, minute=0, sec=41, aux=1, tick=2),
    dict(name="pm", hour=17, minute=58, sec=14, h24=0, batt=12),
]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
