# lab: physics & engineering watchfaces

Fourteen Pebble Round 2 (`gabbro`, 260x260) watchfaces that share one framework. Each is its own Pebble
project (own UUID, `pebble build`), but they all `#include "../../../common/face.h"`.

![overview](docs/overview.png)

| Face | Idea | Wrist tap |
|---|---|---|
| [`orrery`](orrery) | Planets at their approximate real positions today, Moon in its real phase | whole system / inner planets |
| [`scope`](scope) | Phosphor oscilloscope: Lissajous figure set by the time, LED readout | Lissajous / Fourier square wave (Gibbs) |
| [`elements`](elements) | Hour / minute / seconds / day as periodic-table element tiles | seconds / steps element |
| [`gears`](gears) | Meshing spur gears with correct ratios and tooth phasing, blueprint look | blueprint / brass |
| [`pendulum`](pendulum) | 15-pendulum wave that realigns every 60 s | motion-trail ghosts |
| [`resistor`](resistor) | Hour & minute as resistor colour bands, BCD LED matrix | LED colour |
| [`radar`](radar) | ATC scope with a sweep, phosphor wedge and drifting targets | green / amber / ice |
| [`chaos`](chaos) | Double pendulum + a 0.001 rad twin: the butterfly effect | twin / art trail |
| [`ripple`](ripple) | Wave interference from 2-4 point sources | 2 / 3 / 4 sources |
| [`kepler`](kepler) | The hour hand is a planet on a Kepler orbit; equal areas in equal times | eccentricity 0.5 / 0.7 |
| [`logic`](logic) | Logic-analyzer timing diagrams of the minute/second counters, bus decode | MIN:SEC / HR:MIN |
| [`analemma`](analemma) | The Sun's figure-8 over the year, today marked (equation of time + declination) | season / mono |
| [`bohr`](bohr) | Electron jumps between levels, emitting real hydrogen spectral lines | spectrum history |
| [`cradle`](cradle) | Newton's cradle: n in, n out (n from the minute) | ball style |

## Shared pieces

- `common/face.h` - state (time, steps, battery), low-power ticking (once a second only while visible and
  above 15% battery), the wrist-tap hook with a saved setting, and an exact pixel-graphics toolkit: discs, rings,
  thick lines, triangles, arcs, integer / float trig, a 3x5 micro font and neon-tube 7-segment digits.
- `tools/` - `render.c` + `preview_lib.py` (host-side renderer: compiles a face's real `main.c` against a stub
  Pebble API and dumps frames), `newface.py` (scaffolder), `finalize.py` (AddressSanitizer/UBSan sweep + preview).
  Everything is drawn with rectangles and no system fonts, so previews match the watch pixel for pixel.
- `release/` - a prebuilt `.pbw` for every face (SDK 4.33.1, 10-14 KB each) and `lab-faces.zip`.

## Draw-call cost (a CPU/battery proxy)

I can't time anything on a real watch, so `tools/stats.py` counts `fill_rect` calls per frame in the host
renderer and snapshots pixel hashes. The shared `line()` and micro-font text now merge runs of pixels into
single rectangles (pixel-identical output, verified by hash), Ripple Tank merges equal-colour runs per row,
Kepler Clock fills its wedges scanline by scanline, and the shared `ring()` draws top/bottom caps as row spans and the sides as column spans. Before -> after, average calls per frame:
Kepler Clock 8,311 -> 2,430; Oscilloscope 3,202 -> 1,653; Pendulum Wave 3,030 -> 1,217; Chaos Clock
2,716 -> 1,607; Ripple Tank 2,462 -> 1,884; Newton's Cradle 1,285 -> 793. Heaviest now: Gear Train (~3,200; teeth use a one-pass convex quad fill),
Kepler Clock (~2,400) and Radar (~2,000); Gear Train was 4,581 and Radar 2,807 originally. Current numbers:

```
face        avg calls      max
gears            3185     3211
kepler           2430     2646
radar            2010     2374
ripple           1884     1993
resistor         1523     1588
scope            1485     2430
chaos            1449     1651
logic            1388     1416
analemma         1276     1296
pendulum         1217     1462
orrery           1147     1276
bohr              915      956
cradle            793      945
elements          648      706
```


## Physics validation

`tests/run_checks.py` compiles the faces' real code (`tests/probe.c`) and checks it against known real-world
values: seconds since J2000 against Python's `datetime` (including years 1999-2400); Earth's heliocentric
longitude at the 2025 equinoxes and solstices (within 0.7 degrees); the Moon's phase on nine real new and full
moons (within 8 degrees, about 16 hours, which is what a mean-synodic-month model can do); the equation of time
and solar declination on six dates (EoT within 0.5 min, declination within 1 degree); the residual of
Kepler's equation for e = 0.5 and 0.7 (under 0.01 rad); the Bohr Atom's Rydberg wavelengths against the real
hydrogen lines (Lyman, Balmer and Paschen, within 1 nm - which led to correcting 1/R from 91.13 to 91.18 nm);
energy conservation of the double-pendulum integrator (drift under 0.1% over 20 s); and the Gear Train's
tooth-phase meshing condition and speed ratios at every second. All pass. Run it with
`python3 tests/run_checks.py SCRATCHDIR`.

## Status / caveats

- Every face builds for `gabbro` and passed a sanitizer sweep (800-2,000 random states each); that sweep
  already caught one real bug (a 32-bit seconds overflow for far-future dates) which is fixed.
- **None has been run on the emulator or a real watch** (the emulator never connected in the build
  environment), so animation timing, tap behaviour, step counts and per-frame cost are untested. The heavier
  faces (`chaos`, `ripple`, `scope`, `radar`) redraw a lot of primitives once a second; their cost is unmeasured.
- Astronomy faces use standard low-precision approximations (planet positions to a few degrees; analemma to
  about half a degree / half a minute) and treat the watch's local time as UTC - fine for a watchface.
- To publish one, follow `../desk-pet/release/SUBMIT.md` (real screenshots, author name, Unlisted first).
