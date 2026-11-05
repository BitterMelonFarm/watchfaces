#!/usr/bin/env python3
"""python3 finalize.py <face-dir> : sanitizer sweep + copy preview sheet into docs/ (run after preview.py)"""
import os, sys, shutil
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import preview_lib as P
face = os.path.abspath(sys.argv[1])
scratch = sys.argv[2]
runs, fails = P.sweep(face, scratch, n=int(sys.argv[3]) if len(sys.argv) > 3 else 1500)
print(f"sanitizer sweep: {runs} states, {len(fails)} failures")
for c, e in fails[:3]: print(c, e)
src = os.path.join(scratch, "sheet.png")
if os.path.exists(src):
    os.makedirs(os.path.join(face, "docs"), exist_ok=True)
    shutil.copy(src, os.path.join(face, "docs", "preview.png"))
    print("copied preview")
sys.exit(1 if fails else 0)
