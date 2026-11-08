// Hilltop Stars - a quiet hilltop under an enormous sky (original art, in the spirit of gentle pastoral animation).
// Twinkling stars and a Milky Way, the Moon in its REAL phase, a shooting star every minute, a lone tree and a
// little tent with a glowing doorway. The time is a CONSTELLATION: each digit is a pattern of stars joined by faint
// lines. The sky follows the real time of day (the stars fade in as it gets dark). One bright sparkle appears for
// every 1,000 steps; the battery is five lanterns hanging from the tree.
//
// Wrist tap: show / hide the constellation lines.
#define FACE_SECONDS 1
#include "../../../../lab/common/face.h"
#include "../../../../lab/common/paint.h"

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// a digit as a constellation: a star at every lit cell, faint lines between neighbours
static void constellation(int d, int x, int y, int sc, GColor star, GColor link) {
  for (int r = 0; r < 7; r++)
    for (int k = 0; k < 5; k++) {
      if (BIG[d][r][k] != '1') continue;
      int cx = x + k * sc, cy = y + r * sc;
      if (s_aux == 0) {
        if (k < 4 && BIG[d][r][k + 1] == '1') line(link, cx, cy, cx + sc, cy, 1);
        if (r < 6 && BIG[d][r + 1][k] == '1') line(link, cx, cy, cx, cy + sc, 1);
      }
    }
  for (int r = 0; r < 7; r++)
    for (int k = 0; k < 5; k++) {
      if (BIG[d][r][k] != '1') continue;
      int cx = x + k * sc, cy = y + r * sc;
      bool bright = ((r * 5 + k + d) % 3) == 0;
      R(star, cx - (bright ? 1 : 0), cy - (bright ? 1 : 0), bright ? 3 : 2, bright ? 3 : 2);
    }
}

static void face_draw(void) {
  const int m = s_hour * 60 + s_min;
  const bool night = (m >= 1260 || m < 300);
  const bool dark = night || (m >= 1140 && m < 1260) || (m >= 300 && m < 400);   // dusk and early dawn show stars too
  Sky k = sky_for(m);
  gradient(0, 120, k.top, k.mid);
  gradient(120, 215, k.mid, k.hor);
  R(k.hor, 0, 215, SW_, 50);
  if (dark) {
    // Milky Way: a diagonal drift of faint stars
    for (int i = 0; i < 160; i++) {
      uint32_t h = hash2((uint32_t)i, 5);
      int t = (int)(h % 240);                                        // position along the band
      int off = (int)((h >> 8) % 31) - 15;
      int x = 14 + t, y = 200 - t * 150 / 240 + off;
      PX(((h >> 16) & 3) == 0 ? GColorWhite : (night ? GColorBabyBlueEyes : GColorLiberty), x, y);
    }
    // twinkling stars
    for (int i = 0; i < 70; i++) {
      int x = (int)(hash2((uint32_t)i, 6) % 260), y = (int)(hash2((uint32_t)i, 7) % 190);
      uint32_t ph = (s_tick + (uint32_t)i * 3) % 7;
      PX(ph == 0 ? GColorLightGray : GColorWhite, x, y);
      if (ph == 3 && (i % 5) == 0) { PX(GColorWhite, x - 1, y); PX(GColorWhite, x + 1, y); PX(GColorWhite, x, y - 1); PX(GColorWhite, x, y + 1); }
    }
    // a shooting star a few seconds into every minute
    if (s_sec >= 20 && s_sec <= 23) {
      uint32_t h = hash2((uint32_t)(s_hour * 60 + s_min), 3);
      int x0 = 60 + (int)(h % 120), y0 = 56 + (int)((h >> 8) % 30);
      int dx = (s_sec - 20) * 16;
      line(GColorWhite, x0 + dx, y0 + dx / 2, x0 + dx - 20, y0 + dx / 2 - 10, 1);
      line(GColorBabyBlueEyes, x0 + dx - 20, y0 + dx / 2 - 10, x0 + dx - 38, y0 + dx / 2 - 19, 1);
      disc(GColorWhite, x0 + dx, y0 + dx / 2, 1);
    }
  } else {
    // daylight: a sun on its arc and a couple of soft clouds
    int p = (m - 360) * 180 / 750;  p = p < 0 ? 0 : (p > 180 ? 180 : p);
    int sx, sy;
    math_pt(130, 222, 84, 180 - p, &sx, &sy);
    disc(m >= 1020 ? GColorOrange : GColorChromeYellow, sx, sy, 15);
    disc(m >= 1020 ? GColorChromeYellow : GColorPastelYellow, sx, sy, 11);
    cloud(46 + (int)(m % 160), 150, 10, k.cloud_lit, k.cloud_body, k.cloud_shade);
  }
  // the Moon in its real phase, high on the right
  moon_disc(206, 62, 13, moon_phase_deg(), dark ? GColorPastelYellow : GColorWhite, dark ? GColorDukeBlue : GColorBabyBlueEyes);
  // sparkles: one bright four-point star per 1,000 steps
  int sp = s_steps / 1000 > 8 ? 8 : s_steps / 1000;
  static const int spx[8] = {30, 226, 70, 192, 20, 238, 108, 152}, spy[8] = {96, 112, 34, 30, 140, 150, 20, 14};
  for (int i = 0; i < sp; i++) {
    int r = 4 + (int)((s_tick + (uint32_t)i) % 3);
    R(GColorWhite, spx[i] - r, spy[i], 2 * r + 1, 1);  R(GColorWhite, spx[i], spy[i] - r, 1, 2 * r + 1);
    disc(GColorPastelYellow, spx[i], spy[i], 1);
  }
  // the hilltop, a lone tree with lanterns, and a tent with a glowing doorway
  hills(shade(k.far, -1), 226, 14, 20, 60, 0, 260);
  hills(shade(k.midh, -1), 238, 10, 26, 200, 0, 260);
  R(shade(k.midh, -1), 0, 246, SW_, 20);
  const int tx = 196, ty = 214;
  R(GColorDarkGray, tx - 3, ty - 38, 5, 44);                                // trunk
  for (int i = 0; i < 4; i++) disc(GColorDarkGreen, tx - 1 + (i - 2) * 9, ty - 44 - (i & 1) * 6, 13);
  int lamps = (s_batt + 19) / 20;
  for (int i = 0; i < 5; i++) {                                              // lanterns hang from the branches
    int lx = tx - 26 + i * 12, ly = ty - 28 + (i & 1) * 5;
    R(GColorDarkGray, lx, ly - 5, 1, 5);
    disc(i < lamps ? GColorChromeYellow : GColorDarkGray, lx, ly + 1, 3);
    if (i < lamps) disc(GColorYellow, lx, ly + 1, 1);
  }
  const int px = 78, py = 232;                                               // tent
  tri(GColorSunsetOrange, px - 26, py, px + 26, py, px, py - 28);
  tri(GColorBulgarianRose, px, py - 28, px + 26, py, px + 4, py);
  tri(GColorYellow, px - 7, py, px + 7, py, px, py - 14);
  tri(GColorChromeYellow, px - 4, py, px + 4, py, px, py - 9);
  if (dark) for (int r = 22; r > 12; r -= 5) ring(GColorChromeYellow, px, py - 4, r, r - 1);
  // fireflies by the tent at night
  if (night) for (int i = 0; i < 5; i++) {
    int fx = 40 + (int)(hash2((uint32_t)i, 12) % 150) + 6 * isin((int)s_tick * 25 + i * 70) / 1024;
    int fy = 196 + (int)(hash2((uint32_t)i, 13) % 40) + 4 * isin((int)s_tick * 33 + i * 50) / 1024;
    if (((s_tick + (uint32_t)i) % 5) != 4) disc(GColorYellow, fx, fy, 1);
  }
  // the time as a constellation
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {62, 92, 142, 172};
  GColor star = dark ? GColorWhite : GColorWhite;
  GColor link = dark ? GColorBabyBlueEyes : GColorWhite;
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    constellation(dg[i], xs[i], 74, 5, star, link);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorWhite, 128, 88, 4, 4); R(GColorWhite, 128, 106, 4, 4); }
  char buf[28];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 118, 1, dark ? GColorBabyBlueEyes : GColorWhite);
}
