// Orrery - the planets at (approximately) their real positions today, a Moon in its real phase,
// and the time. Mean orbital elements + a first-order equation-of-centre correction give
// heliocentric longitudes good to a few degrees: plenty for a watchface. Orbit radii are not to scale.
//
// Wrist tap: toggle between the whole solar system and a zoomed-in inner-planets view.
#define FACE_SECONDS 1
#include "../../../common/face.h"

typedef struct {
  int32_t l0_udeg;      // mean longitude at J2000, micro-degrees
  int32_t n_udeg_day;   // mean motion, micro-degrees per day
  int16_t varpi;        // longitude of perihelion, degrees
  int16_t e_milli;      // eccentricity x 1000
} Elems;

static const Elems EL[6] = {
  {252250324, 4092334, 77, 206},     // Mercury
  {181979099, 1602130, 131, 7},      // Venus
  {100464572, 985647, 103, 17},      // Earth
  {355446568, 524021, 336, 93},      // Mars
  {34396441, 83091, 14, 48},         // Jupiter
  {49954244, 33471, 93, 54},         // Saturn
};

static int helio_long(int i) {
  int32_t secs = secs_since_j2000();
  int64_t l = (int64_t)EL[i].l0_udeg + (int64_t)EL[i].n_udeg_day * secs / 86400;
  l %= 360000000LL;
  if (l < 0) l += 360000000LL;
  int ldeg = (int)(l / 1000000);
  int m = ldeg - EL[i].varpi;
  int c_dd = (EL[i].e_milli * 1146 * isin(m)) / 1024 / 1000;       // equation of centre, tenths of a degree
  return ldeg + (c_dd + (c_dd >= 0 ? 5 : -5)) / 10;
}

// moon phase 0..359 degrees (0 = new, 180 = full) from the synodic month
static int moon_phase_deg(void) {
  int32_t secs = secs_since_j2000() - 454000;        // reference new moon: 2000-01-06 18:14 (5.26 d after J2000)
  int32_t period = 2551443;                          // 29.530588 days in seconds
  int32_t r = secs % period;
  if (r < 0) r += period;
  return (int)((int64_t)r * 360 / period);
}

static void face_init(void) {}
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const char *MOON_NAMES[8] = {"NEW MOON", "WAXING CRESCENT", "FIRST QUARTER", "WAXING GIBBOUS",
                                    "FULL MOON", "WANING GIBBOUS", "LAST QUARTER", "WANING CRESCENT"};
static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void moon_icon(int cx, int cy, int r, int theta) {
  int ct = icos(theta);
  for (int dy = -r; dy <= r; dy++) {
    int w = isqrt(r * r + r - dy * dy);
    R(GColorDarkGray, cx - w, cy + dy, 2 * w + 1, 1);
    int xt = w * ct / 1024;
    if (theta <= 180) {
      if (xt <= w) R(GColorPastelYellow, cx + xt, cy + dy, w - xt + 1, 1);
    } else {
      R(GColorPastelYellow, cx - w, cy + dy, -xt + w + 1, 1);
    }
  }
}

static void face_draw(void) {
  const int cx = 130, cy = 130;
  // starfield
  for (int i = 0; i < 46; i++) {
    int x = (int)(hash2((uint32_t)i, 3) % 260), y = (int)(hash2((uint32_t)i, 4) % 260);
    int dx = x - cx, dy = y - cy;
    if (dx * dx + dy * dy > 124 * 124) continue;
    bool twinkle = ((s_tick + (uint32_t)i) % 5) == 0;
    PX(twinkle ? GColorWhite : GColorDarkGray, x, y);
  }
  static const int RAD0[6] = {20, 34, 50, 66, 88, 110};
  static const int RAD1[6] = {30, 52, 76, 100, 0, 0};
  const int *rad = s_aux ? RAD1 : RAD0;
  int nplanets = s_aux ? 4 : 6;
  // sun with a glow
  disc(GColorBulgarianRose, cx, cy, 11);
  disc(GColorOrange, cx, cy, 9);
  disc(GColorYellow, cx, cy, 7);
  disc(GColorPastelYellow, cx - 2, cy - 2, 3);
  // orbits (dotted)
  for (int i = 0; i < nplanets; i++) {
    for (int a = 0; a < 360; a += 5) {
      int x, y;
      math_pt(cx, cy, rad[i], a, &x, &y);
      PX(i == 2 ? GColorCobaltBlue : GColorDarkGray, x, y);
    }
  }
  // planets
  int earth_x = cx, earth_y = cy, earth_l = 0;
  for (int i = 0; i < nplanets; i++) {
    int l = helio_long(i), x, y;
    math_pt(cx, cy, rad[i], l, &x, &y);
    switch (i) {
      case 0: disc(GColorLightGray, x, y, s_aux ? 3 : 2); break;
      case 1: disc(GColorPastelYellow, x, y, s_aux ? 4 : 3); disc(GColorWhite, x - 1, y - 1, 1); break;
      case 2:
        disc(GColorCobaltBlue, x, y, s_aux ? 5 : 4);
        R(GColorGreen, x - 2, y - 1, 2, 2);
        PX(GColorWhite, x + 1, y - 2);
        earth_x = x; earth_y = y; earth_l = l;
        break;
      case 3: disc(GColorRed, x, y, s_aux ? 4 : 3); PX(GColorMelon, x - 1, y - 1); break;
      case 4:
        disc(GColorRajah, x, y, 6);
        R(GColorWindsorTan, x - 5, y - 1, 11, 1);
        R(GColorOrange, x - 5, y + 2, 11, 1);
        break;
      default:
        disc(GColorChromeYellow, x, y, 4);
        line(GColorRajah, x - 8, y + 2, x + 8, y - 2, 1);                // ring
        break;
    }
  }
  // the Moon circles the Earth at its real elongation from the Sun
  int phase = moon_phase_deg();
  int mr = s_aux ? 12 : 10;
  for (int a = 0; a < 360; a += 15) {
    int x, y;
    math_pt(earth_x, earth_y, mr, a, &x, &y);
    PX(GColorDarkGray, x, y);
  }
  int mx, my;
  math_pt(earth_x, earth_y, mr, earth_l + 180 + phase, &mx, &my);
  disc(GColorWhite, mx, my, 1);
  // bezel: 60 ticks with the second hand as a bright comet
  ring(GColorDarkGray, cx, cy, 129, 127);
  for (int i = 0; i < 60; i++) {
    int x, y;
    clock_pt(cx, cy, 124, i * 6, &x, &y);
    bool major = (i % 5) == 0;
    PX(major ? GColorLightGray : GColorDarkGray, x, y);
    if (major) { clock_pt(cx, cy, 122, i * 6, &x, &y); PX(GColorLightGray, x, y); }
  }
  for (int k = 0; k < 6; k++) {
    int x, y;
    clock_pt(cx, cy, 125, ((s_sec - k + 60) % 60) * 6, &x, &y);
    R(k == 0 ? GColorWhite : (k < 3 ? GColorCyan : GColorTiffanyBlue), x - 1, y - 1, 3, 3);
  }
  // time plate (top)
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  char buf[24];
  snprintf(buf, sizeof(buf), "%02d:%02d", hour, s_min);
  RR(GColorCyan, 78, 20, 104, 32, 5);
  RR(GColorOxfordBlue, 80, 22, 100, 28, 4);
  micro_c(buf, 130, 26, 4, GColorWhite);
  // info plate (bottom): date + moon phase
  RR(GColorDarkGray, 58, 202, 144, 38, 5);
  RR(GColorBlack, 60, 204, 140, 34, 4);
  moon_icon(80, 221, 11, phase);
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro(buf, 98, 208, 2, GColorCyan);
  int illum = (1024 - icos(phase)) * 100 / 2048;
  snprintf(buf, sizeof(buf), "%s %d%%", MOON_NAMES[((phase + 22) / 45) % 8], illum);
  micro(buf, 98, 224, 1, GColorLightGray);
  snprintf(buf, sizeof(buf), "%d%%", s_batt);
  micro_r(buf, 196, 224, 1, s_batt <= 20 ? GColorRed : GColorGreen);
}
