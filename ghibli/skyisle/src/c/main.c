// Sky Isle - a little island drifting in the clouds (original art, in the spirit of gentle pastoral animation).
// It bobs gently; a lone tree sways; a small windmill turns with the seconds; a waterfall trickles into a mist;
// parallax clouds drift. The sky follows the real time of day. The time is written on a soft cloud; one bird joins
// the flock for every 1,000 steps and the battery is five hanging lanterns.
//
// Wrist tap: breeze (faster windmill and clouds) <-> still.
#define FACE_SECONDS 1
#include "../../../../lab/common/face.h"
#include "../../../../lab/common/paint.h"

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void ellipse(GColor c, int cx, int cy, int rx, int ry) {
  for (int dy = -ry; dy <= ry; dy++) {
    int w = rx * isqrt(ry * ry * 100 - dy * dy * 100) / (ry * 10 > 0 ? ry * 10 : 1);
    R(c, cx - w, cy + dy, 2 * w + 1, 1);
  }
}

static void face_draw(void) {
  const int m = s_hour * 60 + s_min;
  const bool night = (m >= 1260 || m < 300);
  Sky k = sky_for(m);
  gradient(0, 130, k.top, k.mid);
  gradient(130, 260, k.mid, k.hor);
  if (night) {
    for (int i = 0; i < 34; i++) {
      int x = (int)(hash2((uint32_t)i, 31) % 260), y = (int)(hash2((uint32_t)i, 32) % 200);
      PX(((s_tick + (uint32_t)i) % 6) == 0 ? GColorWhite : GColorLightGray, x, y);
    }
    disc(GColorBabyBlueEyes, 206, 96, 13);  disc(GColorPastelYellow, 206, 96, 11);  disc(GColorCeleste, 210, 93, 3);
  } else {
    int p = (m - 360) * 180 / 750;  p = p < 0 ? 0 : (p > 180 ? 180 : p);
    int sx, sy;
    math_pt(130, 200, 112, 180 - p, &sx, &sy);
    disc(m >= 1020 ? GColorOrange : GColorChromeYellow, sx, sy, 16);
    disc(m >= 1020 ? GColorChromeYellow : GColorPastelYellow, sx, sy, 12);
  }
  // back clouds (slow), then front clouds (faster)
  int drift = (int)(s_aux ? 2 : 1);
  for (int i = 0; i < 3; i++) {
    int x = ((i * 130 + m * drift / 2) % 380) - 60;
    cloud(x, 188 + (i * 23) % 40, 9, k.cloud_body, k.cloud_body, k.cloud_shade);
  }
  const int bob = 3 * isin((int)s_tick * 40) / 1024;
  const int icx = 130, icy = 150 + bob;
  // waterfall from the left rim of the island into a mist
  for (int i = 0; i < 12; i++) {
    int y = icy + 6 + ((int)(s_tick * 9) + i * 17) % 96;
    R(i & 1 ? GColorWhite : GColorCeleste, 76 + (i % 3) - 1, y, 2, 5);
  }
  cloud(78, icy + 108, 7, GColorWhite, GColorWhite, k.cloud_shade);
  // rocky underside: layered cone with cracks and hanging roots
  tri(GColorWindsorTan, icx - 62, icy, icx + 62, icy, icx, icy + 70);
  tri(GColorBulgarianRose, icx, icy, icx + 62, icy, icx, icy + 70);
  tri(GColorRajah, icx - 62, icy, icx - 40, icy, icx - 20, icy + 40);
  line(GColorBulgarianRose, icx - 20, icy + 10, icx - 10, icy + 34, 1);
  line(GColorWindsorTan, icx + 20, icy + 12, icx + 14, icy + 40, 1);
  for (int i = 0; i < 6; i++) {
    int rx = icx - 44 + i * 16;
    int len = 10 + (i * 7) % 14;
    line(i & 1 ? k.midh : GColorWindsorTan, rx, icy + 6, rx + ((i & 1) ? 2 : -2), icy + 6 + len, 1);
  }
  // grassy top
  ellipse(k.midh, icx, icy, 64, 14);
  ellipse(k.near, icx, icy - 2, 62, 12);
  ellipse(k.lightg, icx - 6, icy - 5, 48, 8);
  // lone tree, swaying a touch
  int sway = (s_aux ? 3 : 2) * isin((int)s_tick * 45) / 1024;
  R(GColorWindsorTan, icx - 22, icy - 24, 6, 26);
  disc(k.far, icx - 18 + sway, icy - 42, 19);
  disc(k.midh, icx - 28 + sway, icy - 36, 13);
  disc(k.midh, icx - 8 + sway, icy - 36, 14);
  disc(k.near, icx - 22 + sway, icy - 48, 11);
  disc(k.lightg, icx - 26 + sway, icy - 50, 6);
  disc(GColorRed, icx - 12 + sway, icy - 34, 2);  disc(GColorRed, icx - 30 + sway, icy - 42, 2);
  // windmill with turning blades
  const int wx = icx + 30, wy = icy - 22;
  quad(GColorPastelYellow, wx - 8, icy - 2, wx + 8, icy - 2, wx + 5, wy, wx - 5, wy);
  quad(GColorRajah, wx + 2, icy - 2, wx + 8, icy - 2, wx + 5, wy, wx + 3, wy);
  tri(GColorSunsetOrange, wx - 8, wy + 1, wx + 8, wy + 1, wx, wy - 10);
  R(night ? GColorYellow : GColorWindsorTan, wx - 2, icy - 12, 4, 5);
  int ang = (int)(s_sec * (s_aux ? 12 : 6));
  for (int b = 0; b < 4; b++) {
    int bx, by;
    clock_pt(wx, wy - 2, 17, ang + b * 90, &bx, &by);
    line(GColorWhite, wx, wy - 2, bx, by, 2);
    int cx2, cy2;
    clock_pt(wx, wy - 2, 11, ang + b * 90 + 14, &cx2, &cy2);
    line(GColorCeleste, cx2, cy2, bx, by, 1);
  }
  disc(GColorWindsorTan, wx, wy - 2, 2);
  // flock of birds: one per 1,000 steps
  int flock = s_steps / 1000 > 8 ? 8 : s_steps / 1000;
  for (int i = 0; i < flock; i++) {
    int a = (int)(s_tick * 7) + i * 45;
    int bx = 130 + 100 * icos(a) / 1024, by = 120 + 34 * isin(a * 2) / 1024 + i * 3;
    bird(bx, by, (int)((s_tick + (uint32_t)i) & 1) * 2 + 1, night ? GColorBabyBlueEyes : GColorDukeBlue);
  }
  // lanterns (battery) hanging on a rope along the bottom
  int lamps = (s_batt + 19) / 20;
  for (int x = 40; x < 224; x += 4) R(GColorWindsorTan, x, 238 + (int)(4 * isin((x - 40) * 180 / 184) / 1024), 4, 1);
  for (int i = 0; i < 5; i++) {
    int x = 62 + i * 34;
    int y = 238 + 4 * isin((x - 40) * 180 / 184) / 1024;
    R(GColorWindsorTan, x, y, 1, 5);
    RR(i < lamps ? GColorChromeYellow : GColorDarkGray, x - 3, y + 5, 7, 9, 2);
    if (i < lamps) { R(GColorYellow, x - 1, y + 7, 3, 5); disc(GColorPastelYellow, x, y + 10, 1); }
  }
  // the time, written on a big soft cloud
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  cloud(130, 50, 26, GColorWhite, night ? GColorBabyBlueEyes : GColorWhite, night ? GColorBlueMoon : k.cloud_shade);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {79, 101, 137, 159};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    big_digit(dg[i], xs[i] + 1, 31 + 1, 4, GColorBabyBlueEyes);
    big_digit(dg[i], xs[i], 31, 4, GColorDukeBlue);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorDukeBlue, 129, 40, 4, 4); R(GColorDukeBlue, 129, 52, 4, 4); }
  char buf[28];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 66, 1, GColorBlueMoon);
}
