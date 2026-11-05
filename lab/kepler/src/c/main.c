// Kepler Clock - the hour hand is a planet on a real Keplerian orbit. Its 12-hour period is solved from
// Kepler's equation (E - e sin E = M, Newton's method), so it whips past the Sun at periapsis (12 o'clock)
// and crawls at apoapsis (6 o'clock), yet is exactly on the numbered hour mark at each full hour.
// The 12 wedges swept between hour marks all have EQUAL AREA (Kepler's second law); the area swept so
// far this hour is shaded bright.
//
// Wrist tap: eccentricity 0.5 <-> 0.7 (more dramatic).
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define CXO 130
#define CYO 112
#define AOR 88

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static float ecc(void) { return s_aux ? 0.7f : 0.5f; }

static float solve_kepler(float m, float e) {
  float E = m;
  for (int i = 0; i < 7; i++) E = E - (E - e * fsin(E) - m) / (1.0f - e * fcos(E));
  return E;
}

static void orbit_pt(float E, float b, int *x, int *y) {
  *x = CXO + (int)(b * fsin(E) + (fsin(E) >= 0 ? 0.5f : -0.5f));
  *y = CYO - (int)(AOR * fcos(E) + (fcos(E) >= 0 ? 0.5f : -0.5f));
}

static void face_draw(void) {
  const float e = ecc();
  const float TWO_PI = 6.2831853f;
  // semi-minor axis b = a sqrt(1 - e^2); isqrt in fixed point
  int b = (int)isqrt((int)((1.0f - e * e) * 1000000.0f)) * AOR / 1000;
  int fy = CYO - (int)(AOR * e);                    // the Sun sits at a focus, near periapsis (top)
  int fx = CXO;
  // hour wedges: equal-area sectors from the Sun
  float tnow = (float)(s_hour % 12) + (float)s_min / 60.0f + (float)s_sec / 3600.0f;
  int hcur = s_hour % 12;
  float Enow = solve_kepler(TWO_PI * tnow / 12.0f, e);
  for (int k = 0; k < 12; k++) {
    float E0 = solve_kepler(TWO_PI * (float)k / 12.0f, e);
    float E1 = solve_kepler(TWO_PI * (float)(k + 1) / 12.0f, e);
    int sub = 6;
    for (int j = 0; j < sub; j++) {
      float Ea = E0 + (E1 - E0) * (float)j / (float)sub, Eb = E0 + (E1 - E0) * (float)(j + 1) / (float)sub;
      int xa, ya, xb, yb;
      orbit_pt(Ea, (float)b, &xa, &ya);
      orbit_pt(Eb, (float)b, &xb, &yb);
      GColor c = (k % 2) ? GColorOxfordBlue : GColorImperialPurple;
      if (k == hcur) {
        c = (Ea + 0.0001f <= Enow && Eb <= Enow + 0.0001f) ? GColorOrange : GColorDarkCandyAppleRed;
        if (Ea < Enow && Eb > Enow) {                // split the sub-wedge at the planet
          int xm, ym;
          orbit_pt(Enow, (float)b, &xm, &ym);
          tri(GColorOrange, fx, fy, xa, ya, xm, ym);
          tri(GColorDarkCandyAppleRed, fx, fy, xm, ym, xb, yb);
          continue;
        }
      }
      tri(c, fx, fy, xa, ya, xb, yb);
    }
  }
  // orbit outline and hour spokes
  for (int a = 0; a < 360; a++) {
    int x, y;
    orbit_pt((float)a * 0.0174533f, (float)b, &x, &y);
    R(GColorWhite, x, y, 1, 1);
  }
  for (int k = 0; k < 12; k++) {
    float Ek = solve_kepler(TWO_PI * (float)k / 12.0f, e);
    int x, y, xo, yo;
    orbit_pt(Ek, (float)b, &x, &y);
    line(GColorDarkGray, fx, fy, x, y, 1);
    xo = x + (x - CXO) * 14 / (b + 1);
    yo = y + (y - CYO) * 14 / AOR;
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", k == 0 ? 12 : k);
    micro_c(buf, xo, yo - 2, 1, k == hcur ? GColorYellow : GColorLightGray);
  }
  // radius vector, Sun and planet
  int px, py;
  orbit_pt(Enow, (float)b, &px, &py);
  line(GColorYellow, fx, fy, px, py, 1);
  disc(GColorOrange, fx, fy, 8);
  disc(GColorYellow, fx, fy, 6);
  disc(GColorPastelYellow, fx - 2, fy - 2, 2);
  disc(GColorBlack, px, py, 8);
  disc(GColorBlueMoon, px, py, 6);
  disc(GColorGreen, px - 1, py, 2);
  disc(GColorWhite, px - 3, py - 3, 1);
  // labels
  char buf[32];
  int e100 = (int)(e * 100.0f + 0.5f);
  snprintf(buf, sizeof(buf), "E=%d.%02d", e100 / 100, e100 % 100);
  micro("PERIAPSIS", 160, 16, 1, GColorLightGray);
  micro("APOAPSIS", 160, 198, 1, GColorLightGray);
  micro(buf, 34, 120, 1, GColorCyan);
  micro_c("EQUAL AREAS", 130, 140, 1, GColorLightGray);
  micro_c("EQUAL TIMES", 130, 148, 1, GColorLightGray);
  // time plate
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorDarkGray, 62, 207, 136, 40, 7);
  RR(GColorBlack, 64, 209, 132, 36, 6);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 212, 3, GColorOrange, GColorBulgarianRose);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorOrange, 128, 220, 3, 3); R(GColorOrange, 128, 230, 3, 3); }
}
