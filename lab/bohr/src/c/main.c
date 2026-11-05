// Bohr Atom - an electron hopping between energy levels. Every second it jumps to a new level and either
// emits (jumping down) or absorbs (jumping up) a photon whose wavelength comes from the Rydberg formula,
// 1/lambda = R (1/nf^2 - 1/ni^2). Transitions down to n=2 are the visible Balmer lines (H-alpha 656 nm red,
// H-beta 486 nm, H-gamma 434 nm, H-delta 410 nm); those to n=1 are ultraviolet and to n=3 infrared.
// Orbit radii are not to scale; orbital speed falls with n as in the Bohr model.
//
// Wrist tap: switch the spectrum bar between "this second only" and "last 10 seconds".
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define BCX 130
#define BCY 110

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const int LEVELS[8] = {2, 2, 3, 4, 5, 1, 3, 2};
static int level_at(int t) { return LEVELS[hash2((uint32_t)t, 9) % 8]; }

static int rad_of(int n) { return 12 + (n - 1) * 16; }

// wavelength in nm for a transition between ni and nf (ni > nf)
static int lambda_nm(int ni, int nf) {
  int a = ni * ni, b = nf * nf;
  return (int)(9113LL * a * b / (a - b) / 100);          // 91.13 nm * ni^2 nf^2 / (ni^2 - nf^2)
}

static GColor lambda_col(int nm) {
  if (nm < 380 || nm > 740) return GColorLightGray;      // outside the visible range
  if (nm >= 620) return GColorRed;
  if (nm >= 590) return GColorOrange;
  if (nm >= 570) return GColorYellow;
  if (nm >= 520) return GColorGreen;
  if (nm >= 490) return GColorMediumAquamarine;
  if (nm >= 450) return GColorVividCerulean;
  return GColorElectricUltramarine;
}

static int nm_to_x(int nm) { return 52 + (nm - 380) * 156 / 360; }     // 380..740 nm -> 52..208

static void wavy(int x0, int y0, int x1, int y1, GColor c) {           // a photon: sine wave along a segment
  int dx = x1 - x0, dy = y1 - y0;
  int len = isqrt(dx * dx + dy * dy);
  if (len == 0) return;
  int px = x0, py = y0;
  for (int i = 1; i <= len; i += 2) {
    int off = 3 * isin(i * 40) / 1024;
    int x = x0 + dx * i / len + (-dy * off) / len;
    int y = y0 + dy * i / len + (dx * off) / len;
    line(c, px, py, x, y, 2);
    px = x; py = y;
  }
}

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

static void face_draw(void) {
  int t = s_hour * 3600 + s_min * 60 + s_sec;
  int n = level_at(t), nprev = level_at(t - 1);
  // orbits and the nucleus
  for (int k = 1; k <= 5; k++) {
    int r = rad_of(k);
    for (int a = 0; a < 360; a += 6) {
      int x, y;
      clock_pt(BCX, BCY, r, a, &x, &y);
      PX(k == n ? GColorCyan : GColorDarkGray, x, y);
    }
    char b[4];
    snprintf(b, sizeof(b), "%d", k);
    micro(b, BCX + 3, BCY - r - 2, 1, GColorLightGray);
  }
  disc(GColorBulgarianRose, BCX, BCY, 8);
  disc(GColorRed, BCX - 2, BCY - 2, 3);  disc(GColorLightGray, BCX + 2, BCY - 1, 3);
  disc(GColorRed, BCX + 1, BCY + 3, 3);  disc(GColorLightGray, BCX - 3, BCY + 2, 2);
  // the electron: angular speed ~ 1/n^3
  int ang = (t * 360 / (n * n * n) / 2) % 360;
  int ex, ey;
  clock_pt(BCX, BCY, rad_of(n), ang, &ex, &ey);
  // photon from the transition that happened at the top of this second
  int nm = 0;
  GColor pc = GColorWhite;
  const char *kind = "";
  if (n != nprev) {
    int hi = n > nprev ? n : nprev, lo = n < nprev ? n : nprev;
    nm = lambda_nm(hi, lo);
    pc = lambda_col(nm);
    kind = n < nprev ? "EMIT" : "ABSORB";
    int ox, oy;
    clock_pt(BCX, BCY, rad_of(n) + 30, ang, &ox, &oy);
    if (n < nprev) wavy(ex, ey, ox, oy, pc); else wavy(ox, oy, ex, ey, pc);
  }
  disc(GColorBlack, ex, ey, 6);
  disc(GColorCyan, ex, ey, 4);
  disc(GColorWhite, ex - 1, ey - 1, 1);
  // spectrum bar: dim rainbow, the four Balmer lines, and the current photon
  RR(GColorDarkGray, 46, 190, 168, 16, 3);
  for (int x = 52; x <= 208; x++) {
    int w = 380 + (x - 52) * 360 / 156;
    R(lambda_col(w), x, 194, 1, 8);
  }
  for (int x = 52; x <= 208; x++) if (x & 1) R(GColorBlack, x, 194, 1, 8);        // dim by dithering
  static const int balmer[4] = {656, 486, 434, 410};
  for (int i = 0; i < 4; i++) R(lambda_col(balmer[i]), nm_to_x(balmer[i]), 194, 2, 8);
  int hist = s_aux == 1 ? 10 : 1;
  for (int k = hist - 1; k >= 0; k--) {
    int tk = t - k, a = level_at(tk), b2 = level_at(tk - 1);
    if (a == b2) continue;
    int w = lambda_nm(a > b2 ? a : b2, a < b2 ? a : b2);
    if (w < 380 || w > 740) continue;
    R(k == 0 ? GColorWhite : lambda_col(w), nm_to_x(w) - 1, 192, 4, 12);
  }
  micro("400", 26, 195, 1, GColorLightGray);  micro("700", 214, 195, 1, GColorLightGray);
  char buf[40];
  if (nm) {
    const char *nmname = (n < nprev && n == 2) ? (nprev == 3 ? " H-ALPHA" : (nprev == 4 ? " H-BETA" : (nprev == 5 ? " H-GAMMA" : ""))) : "";
    int hi = n > nprev ? n : nprev, lo = n < nprev ? n : nprev;
    snprintf(buf, sizeof(buf), "%s %d>%d  %d NM%s", kind, hi, lo, nm, nmname);
  } else {
    snprintf(buf, sizeof(buf), "N=%d  STABLE", n);
  }
  micro_c(buf, 130, 14, 1, nm ? pc : GColorLightGray);
  micro_c(nm && (nm < 380 || nm > 740) ? (nm < 380 ? "ULTRAVIOLET" : "INFRARED") : (nm ? "VISIBLE" : "GROUND STATE"), 130, 22, 1, GColorDarkGray);
  // time plate
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorDarkGray, 62, 207, 136, 38, 6);
  RR(GColorBlack, 64, 209, 132, 34, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, 3, GColorCyan, GColorCobaltBlue);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorCyan, 128, 219, 3, 3); R(GColorCyan, 128, 229, 3, 3); }
  snprintf(buf, sizeof(buf), "%s %02d", DAYS[s_wday % 7], s_mday);
  micro(buf, 38, 247, 1, GColorLightGray);
}
