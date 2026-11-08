// Rainy Window - a cosy, drowsy moment (original art, in the spirit of gentle pastoral animation). You are indoors,
// looking out through a rain-streaked window at soft grey hills. The time is written in condensation on the glass
// with little trails of water running down from the digits. On the sill: a steaming teapot, five teacups (the
// battery, filled with tea), a sleepy cat that breathes, and a potted plant that grows a leaf for every 1,000 steps.
// The light outside follows the real time of day.
//
// Wrist tap: light rain <-> downpour.
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
  const bool evening = (!night && m >= 1020);
  GColor top = night ? GColorOxfordBlue : (evening ? GColorLiberty : GColorLightGray);
  GColor bot = night ? GColorDukeBlue : (evening ? GColorMelon : GColorCeleste);
  GColor hfar = night ? GColorMidnightGreen : (evening ? GColorIndigo : GColorCadetBlue);
  GColor hmid = night ? GColorDarkGreen : (evening ? GColorPurpureus : GColorMayGreen);
  GColor hnear = night ? GColorDarkGreen : (evening ? GColorJaegerGreen : GColorJaegerGreen);
  // the view outside: soft sky, misty hills, a few distant windows lit at dusk
  gradient(0, 175, top, bot);
  hills(hfar, 150, 16, 18, 30, 0, 260);
  for (int y = 140; y < 176; y += 3) R(bot, 0, y, SW_, 1);                    // mist between the hills
  hills(hmid, 172, 12, 26, 140, 0, 260);
  R(hnear, 0, 190, SW_, 40);
  if (night || evening) for (int i = 0; i < 7; i++) {
    int x = 24 + (int)(hash2((uint32_t)i, 60) % 210), y = 160 + (int)(hash2((uint32_t)i, 61) % 22);
    R(GColorYellow, x, y, 2, 2);
  }
  // rain: slanted streaks falling
  int nr = s_aux ? 70 : 34;
  for (int i = 0; i < nr; i++) {
    int x = ((int)(hash2((uint32_t)i, 70) % 300) + (int)s_tick * 3) % 290 - 15;
    int y = ((int)(hash2((uint32_t)i, 71) % 320) + (int)s_tick * (s_aux ? 34 : 24)) % 300 - 20;
    line(night ? GColorBabyBlueEyes : GColorWhite, x, y, x - 2, y + 8, 1);
  }
  // droplets running down the glass
  for (int i = 0; i < 12; i++) {
    int x = 26 + (int)(hash2((uint32_t)i, 80) % 208);
    int y = ((int)(hash2((uint32_t)i, 81) % 190) + (int)s_tick * (2 + (i % 3))) % 190;
    if (y < 6) continue;
    R(night ? GColorLiberty : GColorCeleste, x, y - 8, 1, 8);
    disc(night ? GColorBabyBlueEyes : GColorWhite, x, y, 2);
    PX(GColorWhite, x - 1, y - 1);
  }
  // the time, written in condensation with running trails
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {68, 96, 139, 167};                                        // 5x7 digits at scale 5, kept inside the window ring
  GColor ink = night ? GColorBabyBlueEyes : GColorWhite;
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    big_digit(dg[i], xs[i] + 2, 46 + 2, 5, night ? GColorLiberty : (evening ? GColorPurpureus : GColorCadetBlue));   // soft blurred shadow
    big_digit(dg[i], xs[i], 46, 5, ink);
    int tx = xs[i] + 3 + (i & 1) * 10;                                       // a trail running down from the digit
    int len = 14 + ((int)s_tick * 2 + i * 9) % 26;
    R(night ? GColorLiberty : GColorBabyBlueEyes, tx, 82, 2, len);
    disc(ink, tx + 1, 82 + len, 2);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(ink, 128, 56, 5, 5); R(ink, 128, 68, 5, 5); }
  char buf[28];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 90, 1, night ? GColorBabyBlueEyes : GColorWhite);
  // window frame (a wooden ring) and the sill with its cosy things
  ring(GColorWindsorTan, 130, 130, 131, 119);
  ring(GColorRajah, 130, 130, 119, 117);
  R(GColorWindsorTan, 0, 196, SW_, 64);
  R(GColorRajah, 0, 196, SW_, 3);
  R(GColorBulgarianRose, 0, 212, SW_, 2);
  micro_c("SLOW DAY", 130, 224, 1, GColorRajah);
  const int sill = 196;
  // teapot with steam
  disc(GColorTiffanyBlue, 62, sill - 10, 11);
  disc(GColorMediumAquamarine, 59, sill - 13, 4);
  R(GColorTiffanyBlue, 50, sill - 3, 24, 3);
  R(GColorTiffanyBlue, 73, sill - 14, 8, 3);  R(GColorTiffanyBlue, 79, sill - 18, 3, 6);     // spout
  ring(GColorTiffanyBlue, 48, sill - 11, 6, 4);                                              // handle
  R(GColorMediumAquamarine, 57, sill - 23, 10, 4);  disc(GColorMediumAquamarine, 62, sill - 25, 2);
  for (int i = 0; i < 3; i++) {
    int sway = 4 * isin((int)s_tick * 50 + i * 100) / 1024;
    disc(i == 0 ? GColorWhite : GColorCeleste, 66 + sway, sill - 32 - i * 9, 3 + i);
  }
  // five teacups: battery = tea left
  int cups = (s_batt + 19) / 20;
  for (int i = 0; i < 5; i++) {
    int x = 96 + i * 13;
    R(GColorPastelYellow, x, sill - 8, 9, 8);  R(GColorWhite, x, sill - 8, 9, 1);
    ellipse(GColorPastelYellow, x + 4, sill, 5, 1);
    R(i < cups ? GColorWindsorTan : GColorBlack, x + 1, sill - 8, 7, 2);
  }
  // a sleepy cat that breathes
  int br = (int)((s_tick & 3) < 2 ? 1 : 0);
  ellipse(GColorRajah, 170, sill - 7 - br, 19, 8 + br);
  ellipse(GColorOrange, 170, sill - 9 - br, 15, 4);
  disc(GColorRajah, 152, sill - 8, 8);
  tri(GColorRajah, 146, sill - 13, 150, sill - 20, 154, sill - 13);  tri(GColorRajah, 152, sill - 13, 157, sill - 20, 159, sill - 12);
  R(GColorBlack, 148, sill - 9, 3, 1);  R(GColorBlack, 154, sill - 9, 3, 1);                  // closed eyes
  PX(GColorShockingPink, 152, sill - 6);
  line(GColorOrange, 188, sill - 4, 196, sill - 10, 3);                                       // curled tail
  // a potted plant: one leaf per 1,000 steps
  R(GColorSunsetOrange, 206, sill - 10, 14, 10);  R(GColorBulgarianRose, 204, sill - 12, 18, 3);
  R(GColorKellyGreen, 212, sill - 30, 2, 20);
  int leaves = s_steps / 1000 > 8 ? 8 : s_steps / 1000;
  for (int i = 0; i < leaves; i++) {
    int side = (i & 1) ? 1 : -1;
    int ly = sill - 14 - i * 2;
    int sway = (s_aux ? 2 : 1) * isin((int)s_tick * 40 + i * 50) / 1024;
    ellipse(i % 3 ? GColorBrightGreen : GColorKellyGreen, 213 + side * (5 + i / 2) + sway, ly - i, 5, 2);
  }
  // lamp glow at dusk and night
  if (night || evening) { for (int r = 0; r < 3; r++) R(GColorChromeYellow, 0, 232 + r * 8, SW_, 1); }
}
