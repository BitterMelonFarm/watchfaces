// Gear Train - an engineering drawing of three meshing spur gears driven by the seconds wheel.
// Tooth counts, gear ratios and tooth phasing are physically consistent: A (24T) turns once a minute,
// B (12T) meshes with A and turns twice as fast the other way, C (16T) meshes with B.
// A red marker on A's rim is the second hand. The time sits in a title-block plate up top.
//
// Wrist tap: swap between BLUEPRINT and BRASS (steampunk) palettes.
#define FACE_SECONDS 1
#include "../../../common/face.h"

typedef struct { GColor bg, dots, line, fill, edge, text, accent; } Pal;
static Pal P_;

static void load_pal(void) {
  if (s_aux == 0) {   // blueprint
    P_ = (Pal){GColorDukeBlue, GColorCobaltBlue, GColorCeleste, GColorCobaltBlue, GColorWhite, GColorCeleste, GColorOrange};
  } else {            // brass
    P_ = (Pal){GColorBlack, GColorBulgarianRose, GColorRajah, GColorWindsorTan, GColorChromeYellow, GColorRajah, GColorRed};
  }
}
static void face_init(void) { s_aux %= 2; load_pal(); }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; load_pal(); }

// spur gear: T teeth, pitch radius pr (module 4), rotation in tenths of a degree (maths, ccw)
static void gear(int cx, int cy, int T, int pr, int rot_dd, GColor fill, int grow) {
  int root = pr - 5 + grow, tip = pr + 4 + grow;
  disc(fill, cx, cy, root + 1);
  int half = 900 / T;                                   // half tooth thickness at pitch, tenths of a degree
  for (int i = 0; i < T; i++) {
    int a = rot_dd + i * 3600 / T;
    int bx0, by0, bx1, by1, tx0, ty0, tx1, ty1;
    math_pt10(cx, cy, root, a - half * 13 / 10, &bx0, &by0);
    math_pt10(cx, cy, root, a + half * 13 / 10, &bx1, &by1);
    math_pt10(cx, cy, tip, a - half * 55 / 100, &tx0, &ty0);
    math_pt10(cx, cy, tip, a + half * 55 / 100, &tx1, &ty1);
    quad(fill, bx0, by0, bx1, by1, tx1, ty1, tx0, ty0);
  }
}

static void gear_full(int cx, int cy, int T, int pr, int rot_dd, const char *label) {
  gear(cx, cy, T, pr, rot_dd, P_.edge, 1);              // bright outline (slightly larger)
  gear(cx, cy, T, pr, rot_dd, P_.fill, 0);
  // lightening holes and hub
  if (pr >= 30) {
    int nh = pr >= 40 ? 6 : 4;
    for (int i = 0; i < nh; i++) {
      int x, y;
      math_pt10(cx, cy, pr * 55 / 100, rot_dd + i * 3600 / nh, &x, &y);
      disc(P_.bg, x, y, pr / 6);
    }
  }
  disc(P_.edge, cx, cy, 6);
  disc(P_.bg, cx, cy, 3);
  // centre lines (dash-dot)
  for (int d = -pr - 10; d <= pr + 10; d += 6) {
    R(P_.line, cx + d, cy, 3, 1);
    R(P_.line, cx, cy + d, 1, 3);
  }
  micro_c(label, cx, cy + pr + 8, 1, P_.text);
}

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void arrow_h(int x0, int x1, int y, GColor c) {
  R(c, x0, y, x1 - x0 + 1, 1);
  R(c, x0, y - 2, 1, 5);  R(c, x1, y - 2, 1, 5);
  tri(c, x0, y, x0 + 4, y - 2, x0 + 4, y + 2);
  tri(c, x1, y, x1 - 4, y - 2, x1 - 4, y + 2);
}

static void face_draw(void) {
  R(P_.bg, 0, 0, SW_, SW_);
  // drawing sheet: dotted grid, border and registration marks
  for (int y = 10; y < 256; y += 13)
    for (int x = 10; x < 256; x += 13) PX(P_.dots, x, y);
  ring(P_.edge, 130, 130, 127, 126);
  ring(P_.line, 130, 130, 122, 121);
  static const int rx[4] = {130, 6, 254, 130}, ry[4] = {8, 130, 130, 252};
  for (int i = 0; i < 4; i++) { R(P_.edge, rx[i] - 4, ry[i], 9, 1); R(P_.edge, rx[i], ry[i] - 4, 1, 9); }

  // --- gear train: A (seconds wheel), B, C ---
  const int TA = 24, TB = 12, TC = 16;
  const int prA = 48, prB = 24, prC = 32;
  const int ax = 76, ay = 162;
  int phi1 = 150, phi2 = -500;                              // directions of the lines of centres, tenths of a degree
  int bx, by, cx2, cy2;
  math_pt10(ax, ay, prA + prB, phi1, &bx, &by);
  math_pt10(bx, by, prB + prC, phi2, &cx2, &cy2);
  int thA = -s_sec * 60;                                    // clockwise, 6 degrees per second
  int thB = (TA + TB) * phi1 / TB - TA * thA / TB + 1800 + 1800 / TB;
  int thC = (TB + TC) * phi2 / TC - TB * thB / TC + 1800 + 1800 / TC;
  gear_full(ax, ay, TA, prA, thA, "T=24");
  gear_full(bx, by, TB, prB, thB, "T=12");
  gear_full(cx2, cy2, TC, prC, thC, "T=16");
  // seconds marker on A's rim
  int mx, my;
  math_pt10(ax, ay, prA - 12, thA, &mx, &my);
  disc(P_.accent, mx, my, 3);
  disc(P_.edge, mx, my, 1);

  // dimension callouts
  arrow_h(ax - prA, ax + prA, 108, P_.line);
  micro_c("PD 96", ax, 100, 1, P_.text);
  R(P_.line, ax - prA, 106, 1, 6);  R(P_.line, ax + prA, 106, 1, 6);
  micro("MODULE 4", 176, 112, 1, P_.text);
  micro("RATIO 2:1", 176, 120, 1, P_.text);
  micro("DWG 042", 176, 128, 1, P_.text);
  micro("REV A", 176, 136, 1, P_.text);
  micro("SEC WHEEL", 46, 226, 1, P_.accent);

  // --- title block with the time ---
  RR(P_.edge, 58, 22, 144, 56, 6);
  RR(P_.bg, 60, 24, 140, 52, 5);
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {70, 98, 136, 164};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 29, 3, P_.edge, P_.dots);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(P_.edge, 128, 40, 3, 3); R(P_.edge, 128, 52, 3, 3); }
  char buf[32];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 68, 1, P_.text);
  snprintf(buf, sizeof(buf), "PWR %d%%", s_batt);
  micro(buf, 56, 236, 1, s_batt <= 20 ? P_.accent : P_.text);
}
