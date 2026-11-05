// Chaos Clock - a double pendulum, the textbook example of sensitive dependence on initial conditions.
// Two pendulums start from the same minute-seeded state, one nudged by 0.001 radian, and the butterfly
// effect pulls them apart over the minute. The state is a deterministic function of the clock: each frame
// re-integrates (RK4, 20 ms steps) from the top of the minute, so there is no state to lose.
//
// Wrist tap: switch between TWIN (two pendulums + separation readout) and ART (one pendulum, long rainbow trail).
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define PX_ 130
#define PY_ 112
#define LARM 46
#define GRAV 9.81f
#define DT 0.02f
#define STEPS_PER_SEC 50
#define TRAIL 90                      // trail samples (every 0.1 s)

typedef struct { float a1, a2, w1, w2; } St;

static St deriv(St s) {
  float d = s.a1 - s.a2, sd = fsin(d), cd = fcos(d);
  float den = 3.0f - (2.0f * cd * cd - 1.0f);
  St r;
  r.a1 = s.w1;
  r.a2 = s.w2;
  r.w1 = (-3.0f * GRAV * fsin(s.a1) - GRAV * fsin(s.a1 - 2.0f * s.a2)
          - 2.0f * sd * (s.w2 * s.w2 + s.w1 * s.w1 * cd)) / den;
  r.w2 = (2.0f * sd * (2.0f * s.w1 * s.w1 + 2.0f * GRAV * fcos(s.a1) + s.w2 * s.w2 * cd)) / den;
  return r;
}
static St add(St a, St b, float k) {
  St r = {a.a1 + k * b.a1, a.a2 + k * b.a2, a.w1 + k * b.w1, a.w2 + k * b.w2};
  return r;
}
static St rk4(St s) {
  St k1 = deriv(s);
  St k2 = deriv(add(s, k1, DT / 2));
  St k3 = deriv(add(s, k2, DT / 2));
  St k4 = deriv(add(s, k3, DT));
  St r = s;
  r.a1 += DT / 6 * (k1.a1 + 2 * k2.a1 + 2 * k3.a1 + k4.a1);
  r.a2 += DT / 6 * (k1.a2 + 2 * k2.a2 + 2 * k3.a2 + k4.a2);
  r.w1 += DT / 6 * (k1.w1 + 2 * k2.w1 + 2 * k3.w1 + k4.w1);
  r.w2 += DT / 6 * (k1.w2 + 2 * k2.w2 + 2 * k3.w2 + k4.w2);
  return r;
}

static void tip(St s, int *x1, int *y1, int *x2, int *y2) {
  int dd1 = (int)(s.a1 * 572.9578f + (s.a1 >= 0 ? 0.5f : -0.5f));
  int dd2 = (int)(s.a2 * 572.9578f + (s.a2 >= 0 ? 0.5f : -0.5f));
  *x1 = PX_ + LARM * isin10(dd1) / 1024;
  *y1 = PY_ + LARM * isin10(dd1 + 900) / 1024;
  *x2 = *x1 + LARM * isin10(dd2) / 1024;
  *y2 = *y1 + LARM * isin10(dd2 + 900) / 1024;
}

typedef struct { St now; int16_t tx[TRAIL], ty[TRAIL]; int n; } Run;

// integrate from the top of the minute to second `sec`, recording the tip every 0.1 s
static void run(Run *r, float da1, int sec) {
  int mod = s_hour * 60 + s_min;
  St s = {1.8f + (mod % 37) * 0.03f + da1, 2.2f + (mod % 53) * 0.02f, 0.0f, 0.0f};
  r->n = 0;
  int total = sec * STEPS_PER_SEC;
  for (int i = 0; i <= total; i++) {
    if (i % 5 == 0) {
      int x1, y1, x2, y2;
      tip(s, &x1, &y1, &x2, &y2);
      int idx = (i / 5) % TRAIL;
      r->tx[idx] = (int16_t)x2;
      r->ty[idx] = (int16_t)y2;
      r->n = i / 5 + 1;
    }
    if (i < total) s = rk4(s);
  }
  r->now = s;
}

static void draw_trail(const Run *r, GColor bright, GColor mid, GColor dim, int maxn) {
  int n = imin(r->n, imin(TRAIL, maxn));
  int px = 0, py = 0;
  for (int k = n - 1; k >= 0; k--) {            // oldest to newest
    int idx = (r->n - 1 - k + TRAIL * 4) % TRAIL;
    int x = r->tx[idx], y = r->ty[idx];
    if (k < n - 1) {
      int age = k;                              // 0 = newest
      GColor c = age < n / 6 ? bright : (age < n / 2 ? mid : dim);
      line(c, px, py, x, y, age < n / 8 ? 2 : 1);
    }
    px = x; py = y;
  }
}

static void draw_pend(St s, GColor rod, GColor bob, GColor pivot) {
  int x1, y1, x2, y2;
  tip(s, &x1, &y1, &x2, &y2);
  line(rod, PX_, PY_, x1, y1, 2);
  line(rod, x1, y1, x2, y2, 2);
  disc(bob, x1, y1, 5);
  disc(GColorWhite, x1 - 1, y1 - 1, 1);
  disc(bob, x2, y2, 6);
  disc(GColorWhite, x2 - 2, y2 - 2, 2);
  disc(pivot, PX_, PY_, 3);
}

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static void face_draw(void) {
  // protractor ring around the pivot
  ring(GColorDarkGray, PX_, PY_, 97, 96);
  for (int a = 0; a < 360; a += 15) {
    int x0, y0, x1, y1;
    clock_pt(PX_, PY_, 97, a, &x0, &y0);
    clock_pt(PX_, PY_, a % 90 == 0 ? 88 : 92, a, &x1, &y1);
    line(a % 90 == 0 ? GColorLightGray : GColorDarkGray, x0, y0, x1, y1, 1);
  }
  R(GColorOxfordBlue, PX_ - 96, PY_, 193, 1);
  R(GColorOxfordBlue, PX_, PY_ - 96, 1, 193);

  static Run A, B;                                 // big; keep off the stack
  run(&A, 0.0f, s_sec);
  char buf[32];
  if (s_aux == 0) {
    run(&B, 0.001f, s_sec);
    draw_trail(&B, GColorFashionMagenta, GColorBulgarianRose, GColorDarkCandyAppleRed, 40);
    draw_trail(&A, GColorCyan, GColorVividCerulean, GColorCobaltBlue, 40);
    draw_pend(B.now, GColorLightGray, GColorFashionMagenta, GColorWhite);
    draw_pend(A.now, GColorWhite, GColorCyan, GColorWhite);
    // angular separation of the outer arms
    float d = (A.now.a2 - B.now.a2) * 57.29578f;            // wrap to +-180 degrees (the arms may have spun many times)
    int turns = (int)((d + (d >= 0 ? 180.0f : -180.0f)) / 360.0f);
    d -= 360.0f * (float)turns;
    if (d < 0) d = -d;
    int di = (int)d;
    snprintf(buf, sizeof(buf), "SEP %d.%d DEG", di, (int)((d - (float)di) * 10.0f));
    micro_c("DOUBLE PENDULUM", 130, 12, 1, GColorLightGray);
    micro_c("TWIN OFFSET 0.001 RAD", 130, 20, 1, GColorFashionMagenta);
    micro_c(buf, 130, 196, 1, di > 30 ? GColorYellow : GColorCyan);
  } else {
    draw_trail(&A, GColorYellow, GColorFashionMagenta, GColorElectricUltramarine, TRAIL);
    draw_pend(A.now, GColorWhite, GColorCyan, GColorWhite);
    micro_c("DOUBLE PENDULUM ART", 130, 12, 1, GColorLightGray);
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
  snprintf(buf, sizeof(buf), "T+%02d S", s_sec);
  micro(buf, 34, 120, 1, GColorLightGray);
  snprintf(buf, sizeof(buf), "%d%%", s_batt);
  micro_r(buf, 228, 120, 1, s_batt <= 20 ? GColorRed : GColorGreen);
}
