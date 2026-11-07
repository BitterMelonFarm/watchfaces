// Collider Clock - a retro-future, Atoms-for-Peace-era homage to the Long Island accelerator lab.
//
// The dial is a RHIC-style collider: two beam rings (BLUE and YELLOW) drawn as wavy circles that swap places
// at six interaction points (12, 2, 4, 6, 8 and 10 o'clock, where the experiments sit). The hour hand is a
// bunch of particles on the Blue ring, the minute hand a bunch on the Yellow ring, and a proton races round the
// bezel once a minute. When the two bunches meet (11 times in 12 hours) they COLLIDE in a starburst. (The real
// beams circulate in opposite directions; here both run clockwise so the face stays readable.)
// Steps are "luminosity" and battery is "beam current".
//
// Wrist tap: switch to TENNIS FOR TWO, the 1958 oscilloscope game built by a Brookhaven physicist.
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define CX 130
#define CY 130
#define R0 96            // mean beam radius
#define AMP 7            // how far the rings swing in/out between interaction points

// atomic-age palette
#define C_BG     GColorOxfordBlue
#define C_CREAM  GColorPastelYellow
#define C_TEAL   GColorTiffanyBlue
#define C_ORANGE GColorOrange
#define C_MUST   GColorChromeYellow
#define C_BLUE   GColorVividCerulean
#define C_YEL    GColorYellow
#define C_NAVY   GColorDukeBlue

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// chunky 5x7 "atomic age" digits
static const char *BIG[10][7] = {
  {"01110", "10001", "10011", "10101", "11001", "10001", "01110"},
  {"00100", "01100", "00100", "00100", "00100", "00100", "01110"},
  {"01110", "10001", "00001", "00110", "01000", "10000", "11111"},
  {"11110", "00001", "00001", "01110", "00001", "00001", "11110"},
  {"00010", "00110", "01010", "10010", "11111", "00010", "00010"},
  {"11111", "10000", "11110", "00001", "00001", "10001", "01110"},
  {"00110", "01000", "10000", "11110", "10001", "10001", "01110"},
  {"11111", "00001", "00010", "00100", "01000", "01000", "01000"},
  {"01110", "10001", "10001", "01110", "10001", "10001", "01110"},
  {"01110", "10001", "10001", "01111", "00001", "00010", "01100"},
};
static void big_digit(int d, int x, int y, int sc, GColor c) {
  for (int r = 0; r < 7; r++)
    for (int k = 0; k < 5; k++)
      if (BIG[d][r][k] == '1') R(c, x + k * sc, y + r * sc, sc, sc);
}

// Googie starburst: 4 or 8 long thin rays and a bright core
static void star(int cx, int cy, int r, bool eight, GColor c, GColor core) {
  R(c, cx - r, cy, 2 * r + 1, 1);
  R(c, cx, cy - r, 1, 2 * r + 1);
  if (eight) {
    int d = r * 7 / 10;
    line(c, cx - d, cy - d, cx + d, cy + d, 1);
    line(c, cx - d, cy + d, cx + d, cy - d, 1);
  }
  disc(c, cx, cy, 2);
  PX(core, cx, cy);
}

// radius of the BLUE (sgn=+1) or YELLOW (sgn=-1) ring at clock angle a: they swap sides every 60 degrees
static int ring_r(int a, int sgn) { return R0 + sgn * AMP * isin(3 * a) / 1024; }

static void beam(int sgn, GColor c) {
  int px = 0, py = 0;
  for (int a = 0; a <= 360; a += 3) {
    int x, y;
    clock_pt(CX, CY, ring_r(a, sgn), a, &x, &y);
    if (a > 0) line(c, px, py, x, y, 2);
    px = x; py = y;
  }
}

static void bunch(int angle, int sgn, GColor core, GColor trail) {
  for (int k = 5; k >= 1; k--) {                          // the bunch train trailing behind
    int x, y;
    clock_pt(CX, CY, ring_r(angle - k * 6, sgn), angle - k * 6, &x, &y);
    disc(trail, x, y, k < 3 ? 3 : 2);
  }
  int x, y;
  clock_pt(CX, CY, ring_r(angle, sgn), angle, &x, &y);
  disc(GColorBlack, x, y, 8);
  disc(trail, x, y, 7);
  disc(core, x, y, 5);
  disc(GColorWhite, x - 1, y - 1, 2);
}

static void detector(int a, const char *name) {
  int x, y;
  clock_pt(CX, CY, R0, a, &x, &y);
  disc(C_BG, x, y, 9);
  ring(C_CREAM, x, y, 8, 6);
  for (int k = 0; k < 6; k++) {                           // little hexagonal detector with spokes
    int sx, sy;
    math_pt(x, y, 6, k * 60 + 30, &sx, &sy);
    line(C_CREAM, x, y, sx, sy, 1);
  }
  disc(C_ORANGE, x, y, 2);
  if (name) {
    int lx, ly;
    clock_pt(CX, CY, R0 - 16, a, &lx, &ly);
    micro_c(name, lx, ly - 2, 1, C_CREAM);
  }
}

// ------------------------------------------------------------------ mode 0: the collider dial
static void draw_collider(void) {
  // background: a few Googie stars
  for (int i = 0; i < 24; i++) {
    int x = (int)(hash2((uint32_t)i, 11) % 260), y = (int)(hash2((uint32_t)i, 12) % 260);
    int dx = x - CX, dy = y - CY;
    if (dx * dx + dy * dy > 120 * 120 || dx * dx + dy * dy < 62 * 62) continue;
    PX(((s_tick + (uint32_t)i) % 6) == 0 ? C_CREAM : C_TEAL, x, y);
  }
  // bezel with hour ticks and a diamond at 12
  ring(C_CREAM, CX, CY, 128, 126);
  ring(C_MUST, CX, CY, 124, 123);
  for (int h = 0; h < 12; h++) {
    int x0, y0, x1, y1;
    clock_pt(CX, CY, 117, h * 30, &x0, &y0);
    clock_pt(CX, CY, 122, h * 30, &x1, &y1);
    line(h % 3 == 0 ? C_ORANGE : C_MUST, x0, y0, x1, y1, h % 3 == 0 ? 3 : 2);
  }
  // the two beam rings
  beam(-1, C_YEL);
  beam(+1, C_BLUE);
  // detectors at the six interaction points
  detector(0, NULL);  detector(60, "BRAHMS");  detector(120, NULL);
  detector(180, "STAR");  detector(240, "PHENIX");  detector(300, "PHOBOS");
  // bunches: hour on the Blue ring, minute on the Yellow ring
  int th = (s_hour % 12) * 30 + s_min / 2;
  int tm = s_min * 6 + s_sec / 10;
  bunch(tm, -1, C_YEL, C_MUST);
  bunch(th, +1, C_BLUE, C_TEAL);
  int d = iabs(((th - tm) % 360 + 540) % 360 - 180);
  if (d <= 5) {                                            // COLLISION!
    int x, y;
    clock_pt(CX, CY, R0, (th + tm) / 2, &x, &y);
    star(x, y, 14, true, C_ORANGE, GColorWhite);
    star(x, y, 9, false, GColorWhite, GColorWhite);
    micro_c("BANG", x, y + 14, 1, C_ORANGE);
  }
  // a proton races round the bezel once a minute
  for (int k = 6; k >= 0; k--) {
    int x, y;
    clock_pt(CX, CY, 121, ((s_sec - k + 60) % 60) * 6, &x, &y);
    R(k == 0 ? GColorWhite : (k < 3 ? C_CREAM : C_TEAL), x - 1, y - 1, 3, 3);
  }
  // title banner
  micro_c("BROOKHAVEN", 130, 48, 2, C_CREAM);
  micro_c("NATIONAL LABORATORY", 130, 62, 1, C_TEAL);
  // beam labels
  micro("BLUE", 48, 118, 1, C_BLUE);  micro("BEAM", 48, 126, 1, C_BLUE);
  micro("YELLOW", 195, 118, 1, C_YEL);  micro("BEAM", 199, 126, 1, C_YEL);
  // decorative atomic starbursts
  star(70, 70, 6, false, C_MUST, GColorWhite);
  star(190, 70, 6, false, C_MUST, GColorWhite);
  star(66, 192, 5, false, C_ORANGE, GColorWhite);
  star(194, 192, 5, false, C_ORANGE, GColorWhite);
  // central time plate with an orange drop shadow
  disc(C_ORANGE, CX + 4, CY + 4, 58);
  disc(C_CREAM, CX, CY, 58);
  ring(C_TEAL, CX, CY, 55, 53);
  ring(C_MUST, CX, CY, 51, 50);
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  micro_c("ATOMIC TIME", CX, 94, 1, C_TEAL);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {82, 104, 136, 158};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    big_digit(dg[i], xs[i] + 2, 116 + 2, 4, C_ORANGE);       // drop shadow
    big_digit(dg[i], xs[i], 116, 4, C_NAVY);
  }
  if (!(s_tick & 1) || !s_in_focus) {
    R(C_ORANGE, 129, 122, 4, 4);  R(C_NAVY, 128, 121, 4, 4);
    R(C_ORANGE, 129, 134, 4, 4);  R(C_NAVY, 128, 133, 4, 4);
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, CX, 153, 1, C_NAVY);
  micro_c(s_hour < 12 ? "A.M." : "P.M.", CX, 163, 1, C_TEAL);
  // luminosity (steps) and beam current (battery)
  int lit = s_steps >= STEP_GOAL ? 10 : s_steps * 10 / STEP_GOAL;
  micro("LUM", 62, 194, 1, C_CREAM);
  for (int i = 0; i < 10; i++) R(i < lit ? (s_steps >= STEP_GOAL ? C_ORANGE : C_MUST) : C_TEAL, 80 + i * 8, 193, 7, 6);
  snprintf(buf, sizeof(buf), "BEAM CURRENT %d MA", s_batt);
  micro_c(buf, CX, 200, 1, s_batt <= 20 ? GColorRed : C_CREAM);
}

// ------------------------------------------------------------------ mode 1: Tennis for Two (1958)
static void arc_seg(int xa, int ya, int xb, int yb, int h, GColor c, int dot) {
  int n = iabs(xb - xa);
  for (int i = 0; i <= n; i++) {
    int x = xa + (xb - xa) * i / (n ? n : 1);
    int s1000 = i * 1000 / (n ? n : 1);
    int y = ya + (yb - ya) * i / (n ? n : 1) - 4 * h * s1000 * (1000 - s1000) / 1000000;
    if (dot <= 1 || i % dot == 0) R(c, x, y, 2, 2);
  }
}

static void draw_tennis(void) {
  const GColor hi = GColorGreen, mid = GColorIslamicGreen, lo = GColorDarkGreen;
  micro_c("TENNIS FOR TWO", CX, 14, 2, hi);
  micro_c("1958  BROOKHAVEN VISITORS' DAY", CX, 28, 1, mid);
  // oscilloscope graticule
  for (int y = 40; y <= 210; y += 17) R(lo, 22, y, 216, 1);
  for (int x = 22; x <= 238; x += 18) R(lo, x, 40, 1, 171);
  // court: ground, net, rackets
  R(hi, 28, 176, 204, 2);
  R(hi, 129, 154, 3, 24);
  R(hi, 129, 154, 3, 2);
  int idx = s_hour * 3600 + s_min * 60 + s_sec;
  for (int k = 3; k >= 0; k--) {                           // the last four strokes fade out; the newest is bright
    int stroke = idx - k;
    bool left_hits = (stroke & 1) == 0;
    int xh = left_hits ? 50 : 210, xr = left_hits ? 210 : 50;
    int xb = left_hits ? 168 : 92;
    GColor c = k == 0 ? hi : (k == 1 ? mid : lo);
    int dot = k == 0 ? 1 : 3;
    arc_seg(xh, 150, xb, 176, 56, c, dot);                 // up and over the net to the bounce
    arc_seg(xb, 176, xr, 150, 24, c, dot);                 // the low return to the receiver
  }
  // rackets and the ball (at the bounce point of the newest stroke, where the trace is brightest)
  R(hi, 46, 142, 3, 34);  R(hi, 211, 142, 3, 34);
  bool lh = (idx & 1) == 0;
  int bx = lh ? 168 : 92;
  disc(hi, bx, 174, 4);
  disc(GColorWhite, bx, 174, 2);
  // the score board is the time, in phosphor tubes
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(mid, 64, 207, 132, 38, 6);
  RR(GColorBlack, 66, 209, 128, 34, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, 3, hi, lo);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(hi, 128, 219, 3, 3); R(hi, 128, 229, 3, 3); }
  char buf[24];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, CX, 190, 1, mid);
}

static void face_draw(void) {
  R(C_BG, 0, 0, SW_, SW_);
  if (s_aux == 0) draw_collider(); else { R(GColorBlack, 0, 0, SW_, SW_); draw_tennis(); }
}
