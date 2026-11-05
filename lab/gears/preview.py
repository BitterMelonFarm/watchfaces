#!/usr/bin/env python3
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, "..", "tools"))
import preview_lib as P
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "out")
P.build(here, out)
SCENES = [dict(name=f"bp{i}", hour=10, minute=9, sec=s, tick=i) for i, s in enumerate((0, 7, 15, 30))] + \
         [dict(name=f"br{i}", hour=19, minute=42, sec=s, tick=i, aux=1, batt=15 if i == 3 else 80) for i, s in enumerate((3, 21, 44, 58))]
ims = [P.frame(**s) for s in SCENES]
P.sheet("sheet", ims, 4)
print("wrote", out)
