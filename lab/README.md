# lab: physics & engineering watchfaces

Pebble Round 2 (`gabbro`) faces that share one framework:

- `common/face.h` - state, low-power ticking, tap hook, and an exact pixel-graphics toolkit
  (discs, rings, thick lines, triangles, arcs, integer trig, 3x5 micro font, neon-tube digits).
- `tools/` - a generic host-side renderer (`render.c`, `preview_lib.py`), a scaffolder (`newface.py`)
  and a sanitizer-sweep runner (`finalize.py`). Everything is drawn with rectangles, so previews
  match the watch pixel for pixel.

Faces: see each folder's README.
