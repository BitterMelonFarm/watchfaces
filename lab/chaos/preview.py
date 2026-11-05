#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [dict(name=f"s{s}", hour=10, minute=9, sec=s, tick=s) for s in (1, 6, 14, 25, 40, 58)] + \
         [dict(name="art1", hour=10, minute=9, sec=44, aux=1), dict(name="art2", hour=16, minute=31, sec=57, aux=1, h24=0)]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
