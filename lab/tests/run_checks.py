#!/usr/bin/env python3
"""Validate the physics in the lab faces against known real-world values.
   python3 run_checks.py SCRATCHDIR      (needs gcc)"""
import os, subprocess, sys, math, datetime
here = os.path.dirname(os.path.abspath(__file__))
scratch = sys.argv[1]
os.makedirs(scratch, exist_ok=True)

def build(face, flag):
    exe = os.path.join(scratch, f"probe_{face}")
    src = os.path.abspath(os.path.join(here, "..", face, "src", "c", "main.c"))
    subprocess.run(["gcc", "-O1", "-w", "-DSW=260", f'-DFACE_SRC="{src}"', f"-D{flag}", "-I", os.path.join(here, "..", "tools"),
                    "-o", exe, os.path.join(here, "probe.c")], check=True)
    return exe

def run(exe, *args):
    return list(map(int, subprocess.run([exe, *map(str, args)], capture_output=True, check=True).stdout.split()))

fails = 0
def check(name, got, want, tol):
    global fails
    ok = abs(got - want) <= tol
    fails += not ok
    print(f"{'ok  ' if ok else 'FAIL'} {name:46s} got {got:9.2f}  want {want:9.2f}  (tol {tol})")

def angdiff(a, b):
    return (a - b + 180) % 360 - 180

# ---- core: seconds since J2000 against Python's datetime
core = build("orrery", "PROBE_CORE")
for (y, mo, d, h, mi, s) in [(2000, 1, 1, 12, 0, 0), (2026, 11, 5, 12, 34, 21), (2100, 3, 1, 0, 0, 0), (1999, 12, 31, 23, 59, 59), (2400, 2, 29, 6, 0, 0)]:
    want = int((datetime.datetime(y, mo, d, h, mi, s) - datetime.datetime(2000, 1, 1, 12, 0, 0)).total_seconds())
    got = run(core, y, mo, d, h, mi, s)[0]
    check(f"secs_since_j2000 {y}-{mo:02d}-{d:02d}", got, want, 0)

# ---- orrery: Earth at equinoxes/solstices (heliocentric longitude = Sun's geocentric longitude + 180),
#      in the J2000 frame the Standish elements use, so subtract ~0.36 deg of precession since 2000.
orr = build("orrery", "PROBE_ORRERY")
prec = 0.0139 * (2025.5 - 2000)            # degrees
for label, (y, mo, d, h, mi), want in [("Earth @ 2025 March equinox", (2025, 3, 20, 9, 1), 180.0),
                                       ("Earth @ 2025 June solstice", (2025, 6, 21, 2, 42), 270.0),
                                       ("Earth @ 2025 Sept equinox", (2025, 9, 22, 18, 19), 0.0),
                                       ("Earth @ 2025 Dec solstice", (2025, 12, 21, 15, 3), 90.0)]:
    L = run(orr, y, mo, d, h, mi, 0)[2]
    check(label + " (deg, wrapped)", angdiff(L, want - prec), 0.0, 1.5)

# ---- orrery: Moon phase against real new / full moons (mean synodic month, so allow +-12 degrees ~ 1 day)
for label, (y, mo, d, h, mi), want in [("NEW  2000-01-06 18:14", (2000, 1, 6, 18, 14), 0), ("NEW  2023-11-13 09:27", (2023, 11, 13, 9, 27), 0),
                                       ("NEW  2024-04-08 18:21", (2024, 4, 8, 18, 21), 0), ("NEW  2025-03-29 10:58", (2025, 3, 29, 10, 58), 0),
                                       ("NEW  2025-09-21 19:54", (2025, 9, 21, 19, 54), 0), ("FULL 2024-01-25 17:54", (2024, 1, 25, 17, 54), 180),
                                       ("FULL 2024-09-18 02:34", (2024, 9, 18, 2, 34), 180), ("FULL 2025-03-14 06:55", (2025, 3, 14, 6, 55), 180),
                                       ("FULL 2025-09-07 18:09", (2025, 9, 7, 18, 9), 180)]:
    ph = run(orr, y, mo, d, h, mi, 0)[6]
    check("moon phase " + label, angdiff(ph, want), 0.0, 12.0)

# ---- analemma: equation of time (minutes) and declination (degrees)
ana = build("analemma", "PROBE_ANALEMMA")
for label, (mo, d), eot, dec in [("Feb 11", (2, 11), -14.2, -13.5), ("May 14", (5, 14), 3.6, 18.5), ("Jul 26", (7, 26), -6.5, 19.2),
                                 ("Nov 3", (11, 3), 16.4, -15.0), ("Jun 21", (6, 21), -1.8, 23.44), ("Dec 21", (12, 21), 1.9, -23.44)]:
    n, e100, d100 = run(ana, 2026, mo, d, 12, 0, 0)
    check(f"EoT {label} (min)", e100 / 100, eot, 0.8)
    check(f"declination {label} (deg)", d100 / 100, dec, 1.0)

# ---- kepler: residual of Kepler's equation
kep = build("kepler", "PROBE_KEPLER")
worst = 0
for e in (500, 700):
    for m in range(0, 6283, 349):
        worst = max(worst, abs(run(kep, 2026, 1, 1, 0, 0, 0, e, m)[0]) / 1e6)
check("Kepler solver worst residual (rad)", worst, 0.0, 0.01)
print("\nALL CHECKS PASSED" if not fails else f"\n{fails} CHECK(S) FAILED")
sys.exit(1 if fails else 0)
