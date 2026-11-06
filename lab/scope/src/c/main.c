// Oscilloscope - a phosphor CRT showing a Lissajous figure whose frequency ratio is set by the time
// (hour : minute/10) and whose phase sweeps with the seconds, over a graticule, with a red LED
// time readout underneath.
//
// Wrist tap: switch between LISSAJOUS and FOURIER (a square wave built from as many harmonics as the
// hour, with the Gibbs overshoot, against the ideal square wave).
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define SCX 130
#define SCY 114
#define SCR 80          // half-size of the screen area in px

static void face_init(void) {}
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static void graticule(void) {
  const GColor grid = GColorDarkGreen, axis = GColorIslamicGreen;
  for (int i = -4; i <= 4; i++) {
    R(i == 0 ? axis : grid, SCX + i * 20, SCY - SCR, 1, 2 * SCR + 1);
    R(i == 0 ? axis : grid, SCX - SCR, SCY + i * 20, 2 * SCR + 1, 1);
  }
  for (int i = -40; i <= 40; i++) {                       // minor ticks on the centre axes
    int len = (i % 5 == 0) ? 5 : 3;
    R(axis, SCX + i * 2, SCY - len / 2, 1, len);
    R(axis, SCX - len / 2, SCY + i * 2, len, 1);
  }
}

static void lissajous(int a, int b, int phase) {
  const int amp = 74;
  int px = 0, py = 0;
  int bx = 0, by = 0;
  // two passes: a dim wide halo, then the bright core (the halo is skipped for long, busy curves to save work)
  bool halo = (a + b) <= 7;
  for (int pass = halo ? 0 : 1; pass < 2; pass++) {
    for (int t = 0; t <= 360; t++) {
      int x = SCX + (amp * isin(a * t + phase)) / 1024;
      int y = SCY - (amp * isin(b * t)) / 1024;
      if (t > 0) line(pass == 0 ? GColorDarkGreen : GColorGreen, px, py, x, y, pass == 0 ? 3 : 1);
      px = x; py = y;
      if (pass == 1 && t == (s_sec * 6) % 360) { bx = x; by = y; }
    }
  }
  R(GColorWhite, bx - 1, by - 1, 3, 3);                    // the electron beam's current spot
}

static void fourier(int harmonics, int phase) {
  const int amp = 56;
  int px = 0, py = 0;
  // ideal square wave (dim) first
  for (int x = 0; x < 2 * SCR; x++) {
    int ang = (x * 720) / (2 * SCR) + phase;
    int sq = isin(ang) >= 0 ? 1 : -1;
    R(GColorDarkGreen, SCX - SCR + x, SCY - sq * (amp * 1304 / 1024 * 3 / 4) - 0, 1, 1);
  }
  for (int pass = 0; pass < 2; pass++) {
    for (int x = 0; x < 2 * SCR; x++) {
      int ang = (x * 720) / (2 * SCR) + phase;
      int sum = 0;
      for (int k = 1; k <= 2 * harmonics - 1; k += 2) sum += isin(k * ang) / k;
      int y = SCY - (amp * sum / 1024) * 1304 / 1024;
      int xx = SCX - SCR + x;
      if (x > 0) line(pass == 0 ? GColorDarkGreen : GColorGreen, px, py, xx, y, pass == 0 ? 3 : 1);
      px = xx; py = y;
    }
  }
}

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

static void face_draw(void) {
  // instrument body and CRT bezel
  RR(GColorDarkGray, 38, 22, 184, 184, 28);
  RR(GColorLightGray, 40, 24, 180, 180, 26);
  RR(GColorDarkGray, 44, 28, 172, 172, 22);
  RR(GColorBlack, 47, 31, 166, 166, 14);
  graticule();
  int h12 = s_hour % 12;
  int a = 1 + h12 % 6, b = 1 + s_min / 10;
  int phase = (s_sec * 6) % 360;
  char buf[24];
  if (s_aux == 0) {
    lissajous(a, b, phase);
    snprintf(buf, sizeof(buf), "X:Y %d:%d", a, b);
  } else {
    int n = h12 == 0 ? 12 : h12;
    fourier(n, phase);
    snprintf(buf, sizeof(buf), "N=%d HARM", n);
  }
  // on-screen readouts
  micro(s_aux == 0 ? "LISSAJOUS" : "FOURIER", 52, 36, 1, GColorGreen);
  if (s_tick & 1) micro_r("TRIG'D", 208, 36, 1, GColorGreen);
  micro(buf, 52, 188, 1, GColorGreen);
  snprintf(buf, sizeof(buf), "%s %02d", DAYS[s_wday % 7], s_mday);
  micro_r(buf, 208, 188, 1, GColorGreen);
  // LED readout
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorDarkGray, 64, 207, 132, 38, 6);
  RR(GColorBlack, 66, 209, 128, 34, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  const int sc = 3;
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, sc, GColorRed, GColorBulgarianRose);
  }
  if (!(s_tick & 1) || !s_in_focus) {
    R(GColorRed, 128, 219, 3, 3);
    R(GColorRed, 128, 229, 3, 3);
  }
  // knobs and a power LED on the lower bezel
  disc(GColorDarkGray, 56, 214, 7); disc(GColorLightGray, 56, 214, 5); R(GColorBlack, 55, 209, 2, 5);
  disc(GColorDarkGray, 204, 214, 7); disc(GColorLightGray, 204, 214, 5); R(GColorBlack, 205, 211, 4, 2);
  disc(s_batt > 20 ? GColorGreen : GColorRed, 46, 205, 2);
}
