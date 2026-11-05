// Ripple Tank - wave interference from 2, 3 or 4 point sources. Each cell shows the sum of the waves
// (cos(k r - w t)) arriving from every source: bright crests, dark troughs, and the still nodal lines
// where waves cancel. The source spacing follows the minutes and the wavelength follows the hour, so the
// interference pattern is different every few minutes; the wave phase advances with the seconds.
//
// Wrist tap: 2 sources / 3 sources / 4 sources.
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define CELL 5

static void face_init(void) { s_aux %= 3; }
static void face_tap(void) { s_aux = (s_aux + 1) % 3; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

static void face_draw(void) {
  int nsrc = 2 + s_aux;
  int d = 36 + (s_min % 20) * 3;                    // source spacing (px)
  int lambda = 14 + (s_hour % 6) * 2;               // wavelength (px)
  int sx[4], sy[4];
  if (nsrc == 2) {
    sx[0] = 130 - d / 2; sy[0] = 84;  sx[1] = 130 + d / 2; sy[1] = 84;
  } else if (nsrc == 3) {
    sx[0] = 130; sy[0] = 70;  sx[1] = 130 - d / 2; sy[1] = 70 + d * 87 / 100;  sx[2] = 130 + d / 2; sy[2] = 70 + d * 87 / 100;
  } else {
    sx[0] = 130 - d / 2; sy[0] = 80;  sx[1] = 130 + d / 2; sy[1] = 80;
    sx[2] = 130 - d / 2; sy[2] = 80 + d;  sx[3] = 130 + d / 2; sy[3] = 80 + d;
  }
  static const GColor ramp[9] = {GColorOxfordBlue, GColorDukeBlue, GColorCobaltBlue, GColorBlueMoon,
                                 GColorVividCerulean, GColorCyan, GColorElectricBlue, GColorCeleste, GColorWhite};
  int wt = (s_sec * 60) % 360;                       // wave phase advances 60 degrees per second
  for (int cy = 0; cy < 260; cy += CELL) {
    int run_x = 0, run_idx = -1;                       // merge runs of equal colour along the row
    for (int cx = 0; cx <= 260; cx += CELL) {
      int idx = -1;
      if (cx < 260) {
        int x = cx + CELL / 2, y = cy + CELL / 2;
        int dx0 = x - 130, dy0 = y - 130;
        if (dx0 * dx0 + dy0 * dy0 <= 128 * 128) {
          int sum = 0;
          for (int i = 0; i < nsrc; i++) {
            int dx = x - sx[i], dy = y - sy[i];
            int r = isqrt(dx * dx + dy * dy);
            sum += isin(r * 360 / lambda - wt);
          }
          int lim = 1024 * nsrc;                       // amplitude range [-lim, lim]
          idx = (sum + lim) * 8 / (2 * lim);
          if (idx < 0) idx = 0;
          if (idx > 8) idx = 8;
        }
      }
      if (idx != run_idx) {
        if (run_idx >= 0) R(ramp[run_idx], run_x, cy, cx - run_x, CELL);
        run_x = cx;
        run_idx = idx;
      }
    }
  }
  // sources
  for (int i = 0; i < nsrc; i++) {
    disc(GColorBlack, sx[i], sy[i], 6);
    disc(GColorYellow, sx[i], sy[i], 4);
    disc(GColorWhite, sx[i] - 1, sy[i] - 1, 1);
  }
  // readouts
  char buf[32];
  RR(GColorBlack, 70, 12, 120, 24, 5);
  micro_c("RIPPLE TANK", 130, 16, 2, GColorCyan);
  snprintf(buf, sizeof(buf), "%d SRC  D=%d  L=%d", nsrc, d, lambda);
  micro_c(buf, 130, 29, 1, GColorLightGray);
  // time plate
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorLightGray, 62, 205, 136, 42, 7);
  RR(GColorBlack, 64, 207, 132, 38, 6);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, 3, GColorCyan, GColorCobaltBlue);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorCyan, 128, 219, 3, 3); R(GColorCyan, 128, 229, 3, 3); }
  snprintf(buf, sizeof(buf), "%s %02d", DAYS[s_wday % 7], s_mday);
  micro(buf, 38, 226, 1, GColorWhite);
  snprintf(buf, sizeof(buf), "%d%%", s_batt);
  micro_r(buf, 222, 226, 1, s_batt <= 20 ? GColorRed : GColorGreen);
}
