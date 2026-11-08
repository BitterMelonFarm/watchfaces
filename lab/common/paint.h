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

// triangle filled on even rows only, so it reads as a translucent beam of light
static void tri_half(GColor c, int x0, int y0, int x1, int y1, int x2, int y2) {
  int ymin = imin(y0, imin(y1, y2)), ymax = imax(y0, imax(y1, y2));
  int xs[3] = {x0, x1, x2}, ys[3] = {y0, y1, y2};
  for (int y = ymin; y <= ymax; y++) {
    if (y & 1) continue;
    int lo = 1 << 20, hi = -(1 << 20);
    for (int e = 0; e < 3; e++) {
      int xa = xs[e], ya = ys[e], xb = xs[(e + 1) % 3], yb = ys[(e + 1) % 3];
      if (y < imin(ya, yb) || y > imax(ya, yb)) continue;
      if (ya == yb) { lo = imin(lo, imin(xa, xb)); hi = imax(hi, imax(xa, xb)); continue; }
      int x = xa + (xb - xa) * (y - ya) / (yb - ya);
      lo = imin(lo, x);
      hi = imax(hi, x);
    }
    if (lo <= hi) R(c, lo, y, hi - lo + 1, 1);
  }
}

static void bird(int x, int y, int flap, GColor c) {          // a small "v" bird
  line(c, x - 4, y - flap, x, y, 1);
  line(c, x, y, x + 4, y - flap, 1);
}

// ---- shared day-cycle sky palette (minute of day -> colours) ----
typedef struct { GColor top, mid, hor, cloud_lit, cloud_body, cloud_shade, far, midh, near, lightg; } Sky;
static Sky sky_for(int m) {
  if (m >= 1260 || m < 300)                       // night
    return (Sky){GColorOxfordBlue, GColorDukeBlue, GColorIndigo, GColorBabyBlueEyes, GColorLiberty, GColorDukeBlue,
                 GColorDarkGreen, GColorDarkGreen, GColorIslamicGreen, GColorJaegerGreen};
  if (m < 420)                                    // dawn
    return (Sky){GColorBabyBlueEyes, GColorMelon, GColorRajah, GColorWhite, GColorMelon, GColorRichBrilliantLavender,
                 GColorJaegerGreen, GColorMayGreen, GColorKellyGreen, GColorInchworm};
  if (m < 1020)                                   // day
    return (Sky){GColorVividCerulean, GColorPictonBlue, GColorCeleste, GColorWhite, GColorWhite, GColorBabyBlueEyes,
                 GColorJaegerGreen, GColorMayGreen, GColorKellyGreen, GColorBrightGreen};
  if (m < 1140)                                   // golden hour
    return (Sky){GColorPictonBlue, GColorRajah, GColorChromeYellow, GColorPastelYellow, GColorMelon, GColorRajah,
                 GColorMayGreen, GColorKellyGreen, GColorInchworm, GColorSpringBud};
  return (Sky){GColorIndigo, GColorPurpureus, GColorSunsetOrange, GColorMelon, GColorRichBrilliantLavender, GColorPurpureus,   // dusk
               GColorDarkGreen, GColorJaegerGreen, GColorMayGreen, GColorKellyGreen};
}


// ---- chunky 5x7 digits for the time plates ----
static const char *BIG[10][7] = {
  {"01110", "10001", "10011", "10101", "11001", "10001", "01110"}, {"00100", "01100", "00100", "00100", "00100", "00100", "01110"},
  {"01110", "10001", "00001", "00110", "01000", "10000", "11111"}, {"11110", "00001", "00001", "01110", "00001", "00001", "11110"},
  {"00010", "00110", "01010", "10010", "11111", "00010", "00010"}, {"11111", "10000", "11110", "00001", "00001", "10001", "01110"},
  {"00110", "01000", "10000", "11110", "10001", "10001", "01110"}, {"11111", "00001", "00010", "00100", "01000", "01000", "01000"},
  {"01110", "10001", "10001", "01110", "10001", "10001", "01110"}, {"01110", "10001", "10001", "01111", "00001", "00010", "01100"},
};
static void big_digit(int d, int x, int y, int sc, GColor c) {
  for (int r = 0; r < 7; r++)
    for (int k = 0; k < 5; k++)
      if (BIG[d][r][k] == '1') R(c, x + k * sc, y + r * sc, sc, sc);
}

