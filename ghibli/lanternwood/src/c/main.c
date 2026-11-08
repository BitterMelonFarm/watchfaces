// Lantern Forest - a hushed forest of light shafts (komorebi), original art in the spirit of gentle pastoral animation.
// Layered trunks, a leafy canopy, a mossy path and a stone lantern. Small round forest spirits sit along the path:
// one for every hour of the clock (1-12), blinking now and then. Fireflies drift about (more of them the more
// you walk), the forest changes with the real time of day (golden shafts by day, pale moonbeams at night), and
// the time glows on a hanging paper lantern. Battery is a row of five little mushrooms.
//
// Wrist tap: fireflies glow brighter / softer.
#define FACE_SECONDS 1
#include "../../../../lab/common/face.h"
#include "../../../../lab/common/paint.h"

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

typedef struct { GColor top, fog, trunk, d, mid, l, shaft, moss; } Wood;
static Wood wood_for(int m) {
  if (m >= 1260 || m < 300) return (Wood){GColorOxfordBlue, GColorMidnightGreen, GColorBulgarianRose, GColorDarkGreen, GColorMidnightGreen, GColorJaegerGreen, GColorLiberty, GColorJaegerGreen};
  if (m < 420)  return (Wood){GColorJaegerGreen, GColorMelon, GColorWindsorTan, GColorJaegerGreen, GColorMayGreen, GColorInchworm, GColorPastelYellow, GColorKellyGreen};
  if (m < 1020) return (Wood){GColorKellyGreen, GColorMintGreen, GColorWindsorTan, GColorIslamicGreen, GColorKellyGreen, GColorBrightGreen, GColorPastelYellow, GColorBrightGreen};
  if (m < 1140) return (Wood){GColorMayGreen, GColorRajah, GColorWindsorTan, GColorJaegerGreen, GColorKellyGreen, GColorSpringBud, GColorChromeYellow, GColorInchworm};
  return (Wood){GColorDukeBlue, GColorPurpureus, GColorBulgarianRose, GColorDarkGreen, GColorJaegerGreen, GColorMayGreen, GColorRichBrilliantLavender, GColorJaegerGreen};
}

static void spirit(int x, int y, bool blink, bool glow) {
  if (glow) { disc(GColorPastelYellow, x, y, 9); }
  disc(GColorBlack, x, y + 1, 7);
  disc(GColorWhite, x, y, 6);
  disc(GColorCeleste, x + 2, y + 2, 3);                          // soft shading
  R(GColorBlack, x - 3, y - 1, 2, blink ? 1 : 2);                // eyes
  R(GColorBlack, x + 2, y - 1, 2, blink ? 1 : 2);
  R(GColorWhite, x - 3, y - 4, 1, 1);                            // little tuft
  R(GColorWhite, x, y - 8, 1, 3);
  R(GColorWhite, x - 1, y - 8, 3, 1);
}

static void face_draw(void) {
  const int m = s_hour * 60 + s_min;
  const bool night = (m >= 1260 || m < 300) || m >= 1140;
  Wood w = wood_for(m);
  gradient(0, 206, w.top, w.mid);
  // far trunks in the mist
  for (int i = 0; i < 10; i++) {
    int x = 6 + i * 27 + (int)(hash2((uint32_t)i, 1) % 9);
    int tw = 7 + (int)(hash2((uint32_t)i, 2) % 5);
    R(w.trunk, x, 10, tw, 210);
    R(shade(w.trunk, -1), x + tw - 2, 10, 2, 210);
  }
  for (int y = 168; y < 208; y += 3) R(w.fog, 0, y, SW_, 1);        // a thin band of mist near the ground
  // light shafts (every other row so they look translucent)
  for (int s = 0; s < 3; s++) {
    int x0 = 30 + s * 70 + (int)(s_tick % 40) / 8;
    for (int y = 14; y < 215; y += 2) {
      int shift = (y - 14) * 5 / 10;
      R(w.shaft, x0 + shift, y, 14 + s * 3, 1);
    }
  }
  // big foreground trunks with roots and moss
  static const int tx[3] = {22, 232, 128};
  for (int i = 0; i < 3; i++) {
    int x = tx[i], tw = i == 2 ? 14 : 22;
    R(w.trunk, x - tw / 2, 0, tw, 214);
    R(shade(w.trunk, -1), x + tw / 2 - 4, 0, 4, 214);
    R(shade(w.trunk, 1), x - tw / 2, 0, 3, 214);
    tri(w.trunk, x - tw / 2 - 12, 214, x - tw / 2 + 4, 190, x - tw / 2 + 4, 214);
    tri(w.trunk, x + tw / 2 + 12, 214, x + tw / 2 - 4, 190, x + tw / 2 - 4, 214);
    for (int j = 0; j < 4; j++) disc(w.moss, x - tw / 2 + 2 + (j % 2) * 6, 170 + j * 12, 4);
  }
  // leafy canopy across the top
  for (int i = 0; i < 16; i++) {
    int x = i * 18 - 4 + (int)(hash2((uint32_t)i, 3) % 10);
    int y = 6 + (int)(hash2((uint32_t)i, 4) % 22);
    disc(w.d, x, y, 17 + (int)(hash2((uint32_t)i, 5) % 6));
  }
  for (int i = 0; i < 12; i++) {
    int x = i * 23 + (int)(hash2((uint32_t)i, 6) % 12);
    int y = 8 + (int)(hash2((uint32_t)i, 7) % 18);
    disc(w.mid, x, y, 12);
    disc(w.l, x - 4, y - 4, 5);
  }
  // mossy ground and a path
  R(w.moss, 0, 206, SW_, 60);
  for (int i = 0; i < 40; i++) PX(w.l, (int)(hash2((uint32_t)i, 8) % 260), 208 + (int)(hash2((uint32_t)i, 9) % 50));
  quad(shade(w.moss, 1), 112, 206, 148, 206, 200, 262, 60, 262);
  quad(w.fog, 120, 206, 140, 206, 170, 262, 90, 262);
  // stone lantern on the right
  const int lx = 196, ly = 208;
  R(GColorLightGray, lx - 8, ly - 4, 16, 4);  R(GColorDarkGray, lx - 8, ly - 1, 16, 1);
  R(GColorLightGray, lx - 3, ly - 20, 6, 16);
  R(GColorLightGray, lx - 9, ly - 24, 18, 4);
  R(night ? GColorYellow : GColorPastelYellow, lx - 6, ly - 34, 12, 10);
  R(GColorDarkGray, lx - 1, ly - 34, 2, 10);
  tri(GColorDarkGray, lx - 12, ly - 34, lx + 12, ly - 34, lx, ly - 44);
  if (night) { for (int r = 14; r > 6; r -= 4) ring(GColorChromeYellow, lx, ly - 29, r, r - 1); }
  // forest spirits: one per hour, sitting along the path and beside it
  static const int sxp[12] = {92, 112, 146, 164, 78, 128, 100, 154, 66, 176, 120, 138};
  static const int syp[12] = {222, 214, 214, 224, 238, 230, 244, 246, 224, 238, 252, 240};
  int hour12 = s_hour % 12 == 0 ? 12 : s_hour % 12;
  for (int i = 0; i < hour12; i++) {
    bool blink = ((s_tick + (uint32_t)i * 3) % 9) == 0;
    spirit(sxp[i], syp[i] + (int)((s_tick + (uint32_t)i) % 3 == 0 ? -1 : 0), blink, night);
  }
  // fireflies: five, plus one per 1,000 steps
  int nf = 5 + (s_steps / 1000 > 9 ? 9 : s_steps / 1000);
  for (int i = 0; i < nf; i++) {
    int fx = (int)(hash2((uint32_t)i, 40) % 220) + 20 + 14 * isin((int)s_tick * 22 + i * 61) / 1024;
    int fy = 70 + (int)(hash2((uint32_t)i, 41) % 140) + 8 * isin((int)s_tick * 31 + i * 97) / 1024;
    if (((s_tick + (uint32_t)i) % 5) == 4) continue;                // a firefly winks out now and then
    if (s_aux) disc(GColorChromeYellow, fx, fy, 4);
    disc(night ? GColorYellow : GColorSpringBud, fx, fy, 2);
    PX(GColorWhite, fx, fy);
  }
  // time on a hanging paper lantern
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  R(GColorWindsorTan, 129, 0, 2, 16);                                 // rope
  RR(GColorBulgarianRose, 62, 14, 136, 9, 4);                          // top cap
  RR(GColorOrange, 66, 21, 128, 50, 20);                               // glowing paper body
  RR(night ? GColorYellow : GColorRajah, 70, 25, 120, 42, 17);
  RR(GColorPastelYellow, 76, 29, 108, 34, 13);
  for (int i = 1; i < 5; i++) R(GColorChromeYellow, 66 + i * 26, 22, 1, 48);   // paper ribs
  RR(GColorBulgarianRose, 62, 68, 136, 9, 4);                          // bottom cap
  R(GColorOrange, 129, 77, 2, 9);  disc(GColorOrange, 130, 88, 3);     // tassel
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {79, 101, 137, 159};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    big_digit(dg[i], xs[i], 33, 4, GColorBulgarianRose);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorBulgarianRose, 129, 42, 4, 4); R(GColorBulgarianRose, 129, 54, 4, 4); }
  char buf[28];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 92, 1, night ? GColorPastelYellow : GColorBulgarianRose);
  // battery: five little mushrooms
  int caps = (s_batt + 19) / 20;
  for (int i = 0; i < 5; i++) {
    int x = 36 + i * 11, y = 238 - (i & 1) * 3;
    R(GColorPastelYellow, x - 1, y, 3, 5);
    disc(i < caps ? GColorRed : GColorDarkGray, x, y, 4);
    if (i < caps) { PX(GColorWhite, x - 2, y - 1); PX(GColorWhite, x + 1, y - 2); }
  }
}
