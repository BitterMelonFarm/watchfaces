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
  // hour wedges: equal-area sectors from the Sun, painted scanline by scanline. Rays run from the Sun
  // to each hour mark (plus one to the planet inside the current hour); every row is cut at the points
  // where rays cross it and each span gets the colour of the wedge it lies in.
  float tnow = (float)(s_hour % 12) + (float)s_min / 60.0f + (float)s_sec / 3600.0f;
  int hcur = s_hour % 12;
  float Enow = solve_kepler(TWO_PI * tnow / 12.0f, e);
  int rx[14], ry[14], nr = 0;
  GColor rc[14];                                   // colour of the wedge that STARTS at this ray (clockwise)
  for (int k = 0; k < 12; k++) {
    float Ek = solve_kepler(TWO_PI * (float)k / 12.0f, e);
    orbit_pt(Ek, (float)b, &rx[nr], &ry[nr]);
    rc[nr] = (k == hcur) ? GColorOrange : ((k % 2) ? GColorOxfordBlue : GColorImperialPurple);
    nr++;
    if (k == hcur) {                               // the planet's own ray splits the current hour
      orbit_pt(Enow, (float)b, &rx[nr], &ry[nr]);
      rc[nr] = GColorDarkCandyAppleRed;
      nr++;
    }
  }
  for (int y = CYO - AOR; y <= CYO + AOR; y++) {
    int dy = y - CYO;
    int hw = isqrt(b * b * (AOR * AOR - dy * dy) / (AOR * AOR));
    int bnd[18], nb = 0;
    bnd[nb++] = CXO - hw;
    for (int j = 0; j < nr; j++) {
      int dyj = ry[j] - fy, t = y - fy;
      if (dyj == 0) continue;
      if ((dyj > 0 && t >= 0 && t <= dyj) || (dyj < 0 && t <= 0 && t >= dyj)) {
        int x = fx + (rx[j] - fx) * t / dyj;
        if (x > CXO - hw && x < CXO + hw) bnd[nb++] = x;
      }
    }
    bnd[nb++] = CXO + hw + 1;
    for (int i = 1; i < nb; i++) {                 // insertion sort
      int v = bnd[i], j = i - 1;
      while (j >= 0 && bnd[j] > v) { bnd[j + 1] = bnd[j]; j--; }
      bnd[j + 1] = v;
    }
    int run_x = bnd[0];
    GColor run_c = GColorBlack;
    bool have = false;
    for (int i = 0; i + 1 < nb; i++) {
      if (bnd[i + 1] <= bnd[i]) continue;
      int xm = (bnd[i] + bnd[i + 1]) / 2;
      GColor c = rc[0];
      for (int w = 0; w < nr; w++) {
        int ax = rx[w] - fx, ay = ry[w] - fy;
        int bx = rx[(w + 1) % nr] - fx, by = ry[(w + 1) % nr] - fy;
        int vx = xm - fx, vy = y - fy;
        if (ax * vy - ay * vx >= 0 && vx * by - vy * bx >= 0) { c = rc[w]; break; }
      }
      if (!have) { run_x = bnd[i]; run_c = c; have = true; }
      else if (c.argb != run_c.argb) {
        R(run_c, run_x, y, bnd[i] - run_x, 1);
        run_x = bnd[i]; run_c = c;
      }
    }
    if (have) R(run_c, run_x, y, bnd[nb - 1] - run_x, 1);
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
