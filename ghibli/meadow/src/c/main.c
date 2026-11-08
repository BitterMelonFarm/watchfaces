// Soft Meadow - a chill, hand-painted day-cycle landscape (original art, in the spirit of gentle pastoral
// animation). The sky blends through night, dawn, day, golden hour and dusk; the sun and moon cross the sky with
// the real time; cumulus clouds drift; a cottage's chimney smoke curls; tall grass sways; and a flower blooms in the
// foreground for every 1,000 steps. The time sits on a soft cream plate.
//
// Wrist tap: toggle gentle "wind" (grass and smoke sway more) / calm.
#define FACE_SECONDS 1
#include "../../../../lab/common/face.h"
#include "../../../../lab/common/paint.h"

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

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

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

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

static void face_draw(void) {
  const int m = s_hour * 60 + s_min;
  const bool night = (m >= 1260 || m < 300);
  Sky k = sky_for(m);
  // sky: top -> mid -> horizon
  gradient(0, 80, k.top, k.mid);
  gradient(80, 160, k.mid, k.hor);
  R(k.hor, 0, 160, SW_, 100);
  // stars at night
  if (night) {
    for (int i = 0; i < 36; i++) {
      int x = (int)(hash2((uint32_t)i, 21) % 260), y = (int)(hash2((uint32_t)i, 22) % 120);
      PX(((s_tick + (uint32_t)i) % 7) == 0 ? GColorWhite : GColorLightGray, x, y);
    }
  }
  // sun by day, moon by night: an arc from the left horizon to the right horizon
  int sx, sy;
  if (!night) {
    int p = (m - 360) * 180 / 750;                           // 06:00 -> 18:30
    p = p < 0 ? 0 : (p > 180 ? 180 : p);
    math_pt(130, 172, 82, 180 - p, &sx, &sy);
    disc(m >= 1020 ? GColorOrange : GColorChromeYellow, sx, sy, 17);
    disc(m >= 1020 ? GColorChromeYellow : GColorPastelYellow, sx, sy, 13);
    disc(GColorWhite, sx - 3, sy - 3, 4);
  } else {
    int mm = (m >= 1260 ? m - 1260 : m + 180);              // 21:00 -> 06:00 (540 min)
    int p = mm * 180 / 540;
    math_pt(130, 172, 82, 180 - p, &sx, &sy);
    disc(GColorBabyBlueEyes, sx, sy, 14);
    disc(GColorPastelYellow, sx, sy, 12);
    disc(GColorCeleste, sx + 4, sy - 3, 3);  disc(GColorCeleste, sx - 4, sy + 4, 2);
  }
  // drifting clouds
  for (int i = 0; i < 3; i++) {
    int x = ((i * 118 + m * 2 / 3) % 360) - 50;
    int y = 70 + (i * 37) % 60;
    cloud(x, y, 12 + (i % 2) * 3, k.cloud_lit, k.cloud_body, k.cloud_shade);
  }
  // hill layers
  hills(k.far, 168, 14, 22, 20, 0, 260);
  hills(k.midh, 190, 12, 30, 140, 0, 260);
  // a cottage on the middle hill, with chimney smoke
  const int hx = 178;
  int hy = hill_y(190, 12, 30, 140, hx);
  R(GColorMelon, hx - 12, hy - 14, 24, 14);                  // walls
  R(GColorRajah, hx - 12, hy - 14, 24, 2);
  tri(GColorSunsetOrange, hx - 16, hy - 13, hx + 16, hy - 13, hx, hy - 28);  // roof
  tri(GColorBulgarianRose, hx - 16, hy - 13, hx - 8, hy - 13, hx - 4, hy - 20);
  R(GColorWindsorTan, hx - 3, hy - 9, 6, 9);                 // door
  R(night ? GColorYellow : GColorCeleste, hx + 5, hy - 11, 5, 4);   // window glows at night
  R(GColorBulgarianRose, hx + 6, hy - 30, 4, 8);             // chimney
  for (int i = 0; i < 4; i++) {                               // smoke puffs rising and swaying
    int sway = (s_aux ? 6 : 3) * isin((int)s_tick * 45 + i * 70) / 1024;
    disc(i < 2 ? k.cloud_body : k.cloud_shade, hx + 8 + sway + i * 2, hy - 36 - i * 8, 3 + i);
  }
  hills(k.near, 214, 10, 24, 60, 0, 260);
  // foreground meadow and swaying tall grass
  R(k.near, 0, 224, SW_, 40);
  for (int i = 0; i < 34; i++) {
    int bx = 4 + i * 8 + (int)(hash2((uint32_t)i, 5) % 4);
    int h = 16 + (int)(hash2((uint32_t)i, 6) % 12);
    int sway = ((s_aux ? 7 : 3) * isin((int)s_tick * 50 + bx * 9) / 1024);
    line(i % 3 ? k.lightg : k.midh, bx, 236 + (int)(hash2((uint32_t)i, 7) % 8), bx + sway, 222 - h / 2 + (int)(hash2((uint32_t)i, 7) % 8), 2);
  }
  // a flower blooms for every 1,000 steps
  int bloom = s_steps / 1000 > 8 ? 8 : s_steps / 1000;
  static const int fx[8] = {40, 72, 100, 126, 152, 180, 208, 232}, fy[8] = {226, 234, 228, 240, 230, 238, 227, 233};
  static const GColor fp[4] = {GColorWhite, GColorMelon, GColorShockingPink, GColorChromeYellow};
  for (int i = 0; i < 8; i++) {
    R(k.midh, fx[i], fy[i], 1, 8);
    if (i < bloom) {
      GColor pc = fp[i % 4];
      disc(pc, fx[i] - 3, fy[i], 2);  disc(pc, fx[i] + 3, fy[i], 2);
      disc(pc, fx[i], fy[i] - 3, 2);  disc(pc, fx[i], fy[i] + 3, 2);
      disc(GColorYellow, fx[i], fy[i], 2);
    } else {
      disc(k.lightg, fx[i], fy[i] - 1, 2);                   // a bud
    }
  }
  // a few small birds by day
  if (!night) for (int i = 0; i < 2; i++) bird(60 + i * 90 + (int)((s_tick * (2 + (uint32_t)i)) % 20), 40 + i * 14, (int)((s_tick + (uint32_t)i) & 1) * 2 + 1, GColorDarkGray);
  // time plate
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  plate(62, 22, 136, 56, 14, GColorPastelYellow, GColorWindsorTan, night ? GColorDukeBlue : GColorRajah);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {76, 98, 134, 156};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    big_digit(dg[i], xs[i] + 1, 29 + 1, 4, GColorRajah);
    big_digit(dg[i], xs[i], 29, 4, GColorDukeBlue);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorWindsorTan, 128, 38, 4, 4); R(GColorWindsorTan, 128, 50, 4, 4); }
  char buf[28];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 64, 1, GColorWindsorTan);
  // battery as five sprouts
  int leaves = (s_batt + 19) / 20;
  for (int i = 0; i < 5; i++) {
    int x = 100 + i * 15;
    R(k.midh, x, 246, 2, 6);
    disc(i < leaves ? k.lightg : k.far, x - 2, 245, 2);
    disc(i < leaves ? GColorBrightGreen : k.far, x + 4, 244, 2);
  }
}
