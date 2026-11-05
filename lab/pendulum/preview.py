#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [dict(name=f"s{s}", hour=14, minute=27, sec=s, tick=s) for s in (0, 5, 12, 20, 30, 41, 50, 58)]
SCENES[2]["aux"] = 1
SCENES[5]["aux"] = 1
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
