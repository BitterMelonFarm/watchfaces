// Painterly helpers for the soft, hand-painted look: dithered sky gradients, shaded clouds, hill layers.
// Needs face.h included first.
#pragma once

// Vertical gradient from c1 (top) to c2 (bottom) over rows y0..y1-1, blended by interleaving rows
// with an 8-level ordered pattern (full-width rectangles, so it is cheap).
static void gradient(int y0, int y1, GColor c1, GColor c2) {
  static const uint8_t bayer[8] = {0, 4, 2, 6, 1, 5, 3, 7};
  int n = y1 - y0;
  if (n <= 0) return;
  for (int r = 0; r < n; r++) {
    int t = r * 8 / n;                       // 0..7
    R(bayer[r & 7] < t ? c2 : c1, 0, y0 + r, SW_, 1);
  }
}

// A cumulus cloud: overlapping puffs with a shaded underside and a bright top edge.
static void cloud(int cx, int cy, int s, GColor lit, GColor body, GColor shade) {
  static const int8_t puffs[6][3] = {{-22, 4, 11}, {-8, -6, 14}, {10, -9, 15}, {24, 2, 11}, {0, 6, 13}, {-14, 8, 9}};
  for (int i = 0; i < 6; i++) disc(shade, cx + puffs[i][0] * s / 10 + 2, cy + puffs[i][1] * s / 10 + 3, puffs[i][2] * s / 10);
  for (int i = 0; i < 6; i++) disc(body, cx + puffs[i][0] * s / 10, cy + puffs[i][1] * s / 10, puffs[i][2] * s / 10);
  for (int i = 1; i < 4; i++) disc(lit, cx + puffs[i][0] * s / 10 - 2, cy + puffs[i][1] * s / 10 - 3, puffs[i][2] * s / 10 * 6 / 10);
}

// Rolling hill: a sine-wave profile filled down to the bottom in 2px columns.
static void hills(GColor c, int base, int amp, int deg_per_px, int phase, int x0, int x1) {
  for (int x = x0; x < x1; x += 2) {
    int y = base - amp * isin(x * deg_per_px / 10 + phase) / 1024;
    R(c, x, y, 2, SW_ - y);
  }
}
static int hill_y(int base, int amp, int deg_per_px, int phase, int x) {
  return base - amp * isin(x * deg_per_px / 10 + phase) / 1024;
}

// soft rounded plate with an offset shadow
static void plate(int x, int y, int w, int h, int rad, GColor fill, GColor edge, GColor shadow) {
  RR(shadow, x + 3, y + 4, w, h, rad);
  RR(edge, x, y, w, h, rad);
  RR(fill, x + 2, y + 2, w - 4, h - 4, rad > 2 ? rad - 2 : rad);
}

static void bird(int x, int y, int flap, GColor c) {          // a small "v" bird
  line(c, x - 4, y - flap, x, y, 1);
  line(c, x, y, x + 4, y - flap, 1);
}
