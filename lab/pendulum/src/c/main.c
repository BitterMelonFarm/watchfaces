// Pendulum Wave - the classic demonstration: 15 pendulums whose lengths are tuned so pendulum i makes
// (N + i) swings in exactly 60 seconds. They start in a line at :00, drift into travelling waves, snakes
// and standing patterns, and realign perfectly at the next :00. The seconds ARE the phase of the system.
// Lengths follow T^2 ~ L, so the longest pendulum (N swings per minute) is the slowest.
//
// Wrist tap: show ghost positions from the previous two seconds (motion trails).
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define NP 15
#define NBASE 40               // swings per minute of the slowest (longest) pendulum
#define BAR_Y 46
#define LMAX 150
#define THETA0_DD 140          // swing amplitude: 14 degrees

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static GColor bob_col(int i) {
  static GColor t[NP];
  t[0] = GColorRed;  t[1] = GColorSunsetOrange; t[2] = GColorOrange;  t[3] = GColorChromeYellow;
  t[4] = GColorYellow; t[5] = GColorSpringBud; t[6] = GColorGreen;   t[7] = GColorMediumSpringGreen;
  t[8] = GColorMediumAquamarine; t[9] = GColorCyan; t[10] = GColorVividCerulean; t[11] = GColorBlueMoon;
  t[12] = GColorElectricUltramarine; t[13] = GColorMagenta; t[14] = GColorFashionMagenta;
  return t[i];
}

static int plen(int i) {                          // pixel length from T^2 ~ L
  int n = NBASE + i;
  return LMAX * NBASE * NBASE / (n * n);
}

// bob position of pendulum i at integer time t (seconds into the cycle)
static void bob_pos(int i, int t, int *x, int *y) {
  int n = NBASE + i;
  int ph = (n * t * 6) % 360;                     // 6 degrees of phase per second per swing/min
  int th = THETA0_DD * isin(ph) / 1024;           // swing angle in tenths of a degree
  int L = plen(i);
  int px0 = 52 + i * 11;
  *x = px0 + L * isin10(th) / 1024;
  *y = BAR_Y + L * icos10(th) / 1024;
}

static void face_draw(void) {
  // title and cycle counter
  micro_c("PENDULUM WAVE", 130, 14, 1, GColorLightGray);
  char buf[24];
  snprintf(buf, sizeof(buf), "T=%02d/60 S", s_sec);
  micro_c(buf, 130, 24, 1, GColorCyan);
  // support bar
  RR(GColorLightGray, 36, BAR_Y - 6, 188, 5, 2);
  R(GColorDarkGray, 36, BAR_Y - 2, 188, 1);
  // rest lines
  for (int i = 0; i < NP; i++) {
    int x0 = 52 + i * 11;
    for (int y = BAR_Y + 4; y < BAR_Y + plen(i); y += 8) PX(GColorDarkGray, x0, y);
  }
  // ghost trails from the previous two seconds
  if (s_aux == 1) {
    for (int k = 2; k >= 1; k--) {
      int t = (s_sec - k + 60) % 60;
      int lx = 0, ly = 0;
      for (int i = 0; i < NP; i++) {
        int x, y;
        bob_pos(i, t, &x, &y);
        disc(k == 2 ? GColorDarkGray : GColorLightGray, x, y, 2);
        if (i > 0) line(k == 2 ? GColorDarkGray : GColorDarkGray, lx, ly, x, y, 1);
        lx = x; ly = y;
      }
    }
  }
  // the pendulums: string, then connecting "wave" line, then bobs
  int bx[NP], by[NP];
  for (int i = 0; i < NP; i++) {
    bob_pos(i, s_sec, &bx[i], &by[i]);
    line(GColorLightGray, 52 + i * 11, BAR_Y, bx[i], by[i], 1);
  }
  for (int i = 1; i < NP; i++) line(GColorDarkGray, bx[i - 1], by[i - 1], bx[i], by[i], 1);
  for (int i = 0; i < NP; i++) {
    disc(GColorDarkGray, bx[i] + 1, by[i] + 1, 5);
    disc(bob_col(i), bx[i], by[i], 5);
    disc(GColorWhite, bx[i] - 2, by[i] - 2, 1);
  }
  // time readout
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorDarkGray, 64, 207, 132, 38, 6);
  RR(GColorBlack, 66, 209, 128, 34, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, 3, GColorCyan, GColorCobaltBlue);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorCyan, 128, 219, 3, 3); R(GColorCyan, 128, 229, 3, 3); }
  snprintf(buf, sizeof(buf), "BAT %d%%", s_batt);
  micro_c(buf, 130, 33, 1, s_batt <= 20 ? GColorRed : GColorGreen);
}
