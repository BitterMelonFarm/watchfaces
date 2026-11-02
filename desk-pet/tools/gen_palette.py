#!/usr/bin/env python3
"""Generate the MUTE[] colour table embedded in src/c/main.c.

Every colour the face draws goes through this map so the whole palette can be
softened in one place. Pebble colours have only 4 levels per channel, so mild
saturation tweaks just snap back; this lowers saturation by half while keeping
brightness, then applies a few hand-picked overrides. Re-run after editing and
paste the output over the MUTE table:  python3 tools/gen_palette.py
"""
import colorsys, re, sys

HDR = "/root/.local/share/pebble-sdk/SDKs/4.33.1/sdk-core/pebble/gabbro/include/gcolor_definitions.h"
SATURATION = 0.5
OVERRIDES = {   # name -> name
    "ShockingPink": "Purpureus",   # the loud magenta-pink frame/outline -> dusty mauve
    "Orange": "SunsetOrange",      # cat outline / armchair -> coral
    "Rajah": "Rajah",              # keep the cat's peach body
    "Yellow": "Icterine",          # sun, lamps -> soft yellow instead of olive
    "ChromeYellow": "Rajah",
    "SunsetOrange": "SunsetOrange",
    "DarkCandyAppleRed": "DarkCandyAppleRed",
}

txt = open(HDR).read()
real = {m.group(1): int(m.group(2), 2)
        for m in re.finditer(r"#define\s+GColor(\w+?)ARGB8\s+\(\(uint8_t\)0b([01]{8})\)", txt)}
rgb = lambda a: ((a >> 4) & 3, (a >> 2) & 3, a & 3)
pal = [(r, g, b) for r in range(4) for g in range(4) for b in range(4)]
Y = lambda c: .3 * c[0] + .59 * c[1] + .11 * c[2]

def mute(c):
    h, l, s = colorsys.rgb_to_hls(*(x / 3 for x in c))
    t = tuple(x * 255 for x in colorsys.hls_to_rgb(h, l, s * SATURATION))
    def cost(p):
        pp = tuple(x * 85 for x in p)
        return sum((pp[i] - t[i]) ** 2 for i in range(3)) + 18 * (Y(pp) - Y(t)) ** 2
    return min(pal, key=cost)

table = {c: mute(c) for c in pal}
for k, v in OVERRIDES.items():
    table[rgb(real[k])] = rgb(real[v])

enc = lambda c: (c[0] << 4) | (c[1] << 2) | c[2]
vals = [enc(table[(r, g, b)]) for r in range(4) for g in range(4) for b in range(4)]
print("static const uint8_t MUTE[64] = {")
for i in range(0, 64, 16):
    print("  " + ", ".join("0x%02X" % v for v in vals[i:i + 16]) + ",")
print("};")
