#!/usr/bin/env python3
"""python3 stats.py OUTDIR [--hash FILE] : per-face draw-call counts and a pixel-hash snapshot.
Calls are fill_rect invocations per frame (a proxy for CPU cost on the watch). The hash snapshot
lets an optimisation prove it changed no pixels: run once before, once after, and diff the files."""
import os, sys, subprocess, hashlib, json, random
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import preview_lib as P
ROOT = os.path.abspath(os.path.join(here, "..", ".."))
FACES = []                                           # (name, directory) for every face under lab/ and ghibli/
for group in ("lab", "ghibli"):
    gdir = os.path.join(ROOT, group)
    for n in sorted(os.listdir(gdir)):
        if os.path.exists(os.path.join(gdir, n, "package.json")):
            FACES.append((n, os.path.join(gdir, n)))
out = sys.argv[1]
os.makedirs(out, exist_ok=True)
rnd = random.Random(3)
states = [(rnd.randrange(24), rnd.randrange(60), rnd.randrange(60), rnd.randrange(7), rnd.randrange(1, 29), rnd.randrange(12),
           2026, rnd.choice([100, 4000, 9000]), rnd.choice([10, 60, 100]), rnd.randrange(8), rnd.randrange(3), 1, rnd.choice([0, 1]))
          for _ in range(12)]
hashes, table = {}, []
for f, d in FACES:
    sub = os.path.join(out, f)
    P.build(d, sub)
    calls = []
    h = hashlib.sha256()
    for st in states:
        r = subprocess.run([P.exe(), *map(str, st)], capture_output=True, check=True)
        h.update(r.stdout)
        for line in r.stderr.decode().splitlines():
            if line.startswith("STAT"):
                calls.append(int(line.split()[1]))
    hashes[f] = h.hexdigest()
    table.append((f, sum(calls) // len(calls), max(calls)))
table.sort(key=lambda t: -t[1])
print(f"{'face':10s} {'avg calls':>10s} {'max':>8s}")
for f, a, m in table: print(f"{f:10s} {a:10d} {m:8d}")
if "--hash" in sys.argv:
    json.dump(hashes, open(sys.argv[sys.argv.index("--hash") + 1], "w"), indent=1)
