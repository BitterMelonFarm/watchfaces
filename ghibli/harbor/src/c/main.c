// Harbor Light - a whitewashed seaside town on a cliff above a sparkling harbour (original art, in the spirit of
// gentle pastoral animation). A lighthouse beam sweeps once a minute after dusk (the seconds hand); sailboats
// rock on the water, one for every 2,000 steps; the sky and sea follow the real time of day. The time is on a
// string of bunting flags, one digit per pennant. Battery is five lamps along the quay.
//
// Wrist tap: bright beam <-> soft beam, and flags flutter more / less.
#define FACE_SECONDS 1
#include "../../../../lab/common/face.h"
#include "../../../../lab/common/paint.h"

#define HZ 134

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void boat(int x, int y, int rock, bool night, GColor sail) {
  int tilt = rock;                                           // -2..2
  RR(GColorWindsorTan, x - 11, y, 22, 6, 3);
  R(GColorBulgarianRose, x - 11, y + 4, 22, 2);
  R(GColorDarkGray, x + tilt, y - 26, 1, 27);                // mast
  tri(sail, x + 2 + tilt, y - 25, x + 2, y - 3, x + 14, y - 3);
  tri(GColorWhite, x - 1 + tilt, y - 20, x - 1, y - 3, x - 11, y - 3);
  if (night) { R(GColorYellow, x + 6, y - 3, 2, 3); }
}

static void face_draw(void) {
  const int m = s_hour * 60 + s_min;
  const bool night = (m >= 1260 || m < 300);
  const bool dusk = night || m >= 1100;
  Sky k = sky_for(m);
  gradient(0, 80, k.top, k.mid);
  gradient(80, HZ, k.mid, k.hor);
  GColor shallow = night ? GColorDukeBlue : (m >= 1020 ? GColorPurpureus : GColorCadetBlue);
  GColor deep = night ? GColorOxfordBlue : (m >= 1020 ? GColorIndigo : GColorTiffanyBlue);
  gradient(HZ, 205, k.hor, shallow);
  gradient(205, 260, shallow, deep);
  // sun or moon
  int sx, sy;
  if (!night) {
    int p = (m - 360) * 180 / 750;  p = p < 0 ? 0 : (p > 180 ? 180 : p);
    math_pt(130, HZ + 6, 56, 180 - p, &sx, &sy);
    disc(m >= 1020 ? GColorOrange : GColorChromeYellow, sx, sy, 14);
    disc(m >= 1020 ? GColorChromeYellow : GColorPastelYellow, sx, sy, 10);
  } else {
    int mm = (m >= 1260 ? m - 1260 : m + 180);
    math_pt(130, HZ + 6, 56, 180 - mm * 180 / 540, &sx, &sy);
    disc(GColorBabyBlueEyes, sx, sy, 12);  disc(GColorPastelYellow, sx, sy, 10);
    for (int i = 0; i < 28; i++) PX(((s_tick + (uint32_t)i) % 6) == 0 ? GColorWhite : GColorLightGray, (int)(hash2((uint32_t)i, 91) % 260), (int)(hash2((uint32_t)i, 92) % 100));
  }
  // glitter on the water under the sun/moon
  for (int y = HZ + 2; y < 200; y += 4) {
    int w = 3 + (y - HZ) / 5;
    R(night ? GColorBabyBlueEyes : GColorPastelYellow, sx - w / 2 + (int)(hash2((uint32_t)y, s_tick) % 7) - 3, y, w, 1);
  }
  // right headland with a lighthouse
  quad(GColorWindsorTan, 168, 200, 260, 168, 260, 262, 146, 262);
  quad(GColorRajah, 168, 200, 260, 168, 260, 176, 176, 204);
  const int lx = 212, top = 112;
  quad(GColorPastelYellow, lx - 9, 182, lx + 9, 182, lx + 5, top + 8, lx - 5, top + 8);
  quad(GColorRed, lx - 8, 160, lx + 8, 160, lx + 7, 148, lx - 7, 148);
  quad(GColorRed, lx - 6, 132, lx + 6, 132, lx + 5, 122, lx - 5, 122);
  R(GColorDarkGray, lx - 7, top, 14, 8);
  R(dusk ? GColorYellow : GColorCeleste, lx - 5, top + 1, 10, 6);
  tri(GColorBulgarianRose, lx - 9, top, lx + 9, top, lx, top - 9);
  // the beam sweeps once a minute after dusk
  if (dusk) {
    int ang = s_sec * 6;
    int ex, ey, fx, fy;
    clock_pt(lx, top + 4, 170, ang - 4, &ex, &ey);
    clock_pt(lx, top + 4, 170, ang + 4, &fx, &fy);
    tri_half(s_aux ? GColorWhite : GColorPastelYellow, lx, top + 4, ex, ey, fx, fy);
    disc(GColorChromeYellow, lx, top + 4, 8);  disc(GColorWhite, lx, top + 4, 4);
  }
  // left cliff with whitewashed houses
  quad(GColorWindsorTan, 0, 96, 64, 122, 112, 196, 0, 262);
  quad(GColorRajah, 0, 96, 64, 122, 72, 134, 0, 112);
  static const int hxp[7] = {10, 30, 52, 20, 44, 70, 34}, hyp[7] = {104, 114, 130, 140, 154, 156, 176};
  for (int i = 0; i < 7; i++) {
    int w = 16 + (i % 3) * 3, h = 12 + (i % 2) * 4;
    R(i % 2 ? GColorPastelYellow : GColorWhite, hxp[i], hyp[i], w, h);
    R(GColorLightGray, hxp[i] + w - 3, hyp[i], 3, h);
    tri(i % 3 == 0 ? GColorSunsetOrange : GColorOrange, hxp[i] - 2, hyp[i], hxp[i] + w + 2, hyp[i], hxp[i] + w / 2, hyp[i] - 8);
    R(dusk ? GColorYellow : GColorCobaltBlue, hxp[i] + 3, hyp[i] + 4, 3, 4);
    R(GColorWindsorTan, hxp[i] + w / 2, hyp[i] + h - 5, 3, 5);
  }
  // boats rocking in the harbour: one always, plus one per 2,000 steps
  int nb = 1 + (s_steps / 2000 > 4 ? 4 : s_steps / 2000);
  static const int bxp[5] = {96, 138, 118, 74, 160}, byp[5] = {190, 172, 214, 222, 226};
  static const GColor sails[5] = {GColorRed, GColorYellow, GColorBlueMoon, GColorOrange, GColorMediumAquamarine};
  for (int i = 0; i < nb; i++) {
    int rock = (int)((s_tick + (uint32_t)i * 2) % 4) - 2;
    rock = rock < 0 ? -rock : rock;
    boat(bxp[i], byp[i] + (int)((s_tick + (uint32_t)i) % 2), rock, night, sails[i]);
  }
  // wave sparkles
  for (int i = 0; i < 24; i++) {
    int y = HZ + 6 + (int)(hash2((uint32_t)i, 6) % 110);
    int x = ((int)(hash2((uint32_t)i, 7) % 280) + (int)(s_tick * (1 + i % 3))) % 280 - 10;
    R(night ? GColorLiberty : GColorWhite, x, y, 4 + (y - HZ) / 14, 1);
  }
  // gulls by day
  if (!night) for (int i = 0; i < 2; i++) bird(80 + i * 70 + (int)((s_tick * (1 + (uint32_t)i)) % 16), 98 + i * 8, (int)((s_tick + (uint32_t)i) & 1) * 2 + 1, GColorWhite);
  // quay with five lamps (battery)
  R(GColorWindsorTan, 0, 238, SW_, 22);
  R(GColorRajah, 0, 238, SW_, 2);
  for (int x = 6; x < 260; x += 12) R(GColorBulgarianRose, x, 240, 1, 20);
  int lamps = (s_batt + 19) / 20;
  for (int i = 0; i < 5; i++) {
    int x = 62 + i * 34;
    R(GColorBlack, x, 224, 2, 14);
    if (i < lamps) { disc(GColorChromeYellow, x + 1, 222, 5);  disc(GColorYellow, x + 1, 222, 3); }
    else disc(GColorDarkGray, x + 1, 222, 3);
  }
  // time on bunting flags
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  static const GColor fl[4] = {GColorRed, GColorYellow, GColorVividCerulean, GColorOrange};
  static const GColor fi[4] = {GColorWhite, GColorDukeBlue, GColorWhite, GColorDukeBlue};
  const int fx[4] = {50, 88, 140, 178};
  int flut = s_aux ? 2 : 1;
  for (int x = 44; x < 216; x += 3) R(GColorWindsorTan, x, 22 + (int)(8 * isin((x - 44) * 180 / 172) / 1024), 3, 1);
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    int base = 22 + (int)(8 * isin((fx[i] + 16 - 44) * 180 / 172) / 1024);
    int w = (int)((s_tick + (uint32_t)i) % 2) * flut;
    tri(fl[i], fx[i], base, fx[i] + 32, base, fx[i] + 16 + w, base + 40);
    big_digit(dg[i], fx[i] + 7, base + 4, 3, fi[i]);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorWhite, 129, 38, 3, 3); R(GColorWhite, 129, 48, 3, 3); }
  char buf[28];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 246, 1, GColorPastelYellow);
}
