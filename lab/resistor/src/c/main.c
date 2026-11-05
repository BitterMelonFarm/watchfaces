// Resistor Clock - a PCB where the hour and minute are resistor values in the standard colour code
// (12 ohm = brown, red, black), and a binary-coded-decimal LED matrix shows HH:MM:SS.
//
// Wrist tap: cycle the LED colour (red / green / blue).
#define FACE_SECONDS 1
#include "../../../common/face.h"

static void face_init(void) { s_aux %= 3; }
static void face_tap(void) { s_aux = (s_aux + 1) % 3; }

static GColor band(int d) {
  switch (d) {
    case 0: return GColorBlack;
    case 1: return GColorWindsorTan;      // brown
    case 2: return GColorRed;
    case 3: return GColorOrange;
    case 4: return GColorYellow;
    case 5: return GColorGreen;
    case 6: return GColorBlueMoon;
    case 7: return GColorElectricUltramarine;   // violet
    case 8: return GColorLightGray;
    default: return GColorWhite;
  }
}

static void pad(int x, int y) {
  disc(GColorChromeYellow, x, y, 5);
  disc(GColorBlack, x, y, 2);
}

static void resistor(int x, int y, int w, int h, int val, const char *name) {
  int cy = y + h / 2;
  // leads and pads with copper traces fanning out to the board edge
  R(GColorLightGray, x - 26, cy, 28, 2);  R(GColorLightGray, x + w - 2, cy, 28, 2);
  pad(x - 30, cy + 1);  pad(x + w + 30, cy + 1);
  R(GColorGreen, x - 70, cy, 36, 2);  R(GColorGreen, x + w + 34, cy, 36, 2);
  // dog-bone body
  RR(GColorRajah, x, y, w, h, h / 2);
  R(GColorRajah, x + h / 2, y - 2, w - h, h + 4);
  R(GColorPastelYellow, x + h / 2, y, w - h, 2);                       // highlight
  R(GColorWindsorTan, x + h / 2, y + h - 1, w - h, 1);                  // shadow
  // colour bands: tens, ones, multiplier x1 (black) and a gold tolerance band
  int bx = x + h / 2 + 8;
  R(band(val / 10), bx, y - 2, 8, h + 4);
  R(band(val % 10), bx + 17, y - 2, 8, h + 4);
  R(band(0), bx + 34, y - 2, 8, h + 4);
  R(GColorChromeYellow, x + w - h / 2 - 12, y - 2, 6, h + 4);
  char buf[24];
  snprintf(buf, sizeof(buf), "%s = %d OHM", name, val);
  micro_c(buf, x + w / 2, y + h + 6, 1, GColorWhite);
}

static void led(int cx, int cy, bool on, GColor lit, GColor glow, GColor dim) {
  disc(GColorBlack, cx, cy, 6);
  if (on) {
    disc(glow, cx, cy, 6);
    disc(lit, cx, cy, 4);
    disc(GColorWhite, cx - 1, cy - 1, 1);
  } else {
    disc(dim, cx, cy, 4);
  }
}

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void face_draw(void) {
  // solder-mask green board with copper traces, vias and mounting holes
  R(GColorIslamicGreen, 0, 0, SW_, SW_);
  for (int i = 0; i < 9; i++) {
    int y = 40 + i * 24;
    R(GColorGreen, 10, y, 30, 1);  R(GColorGreen, 220, y + 8, 30, 1);
    if (i % 2) { R(GColorGreen, 40, y, 1, 12); R(GColorGreen, 220, y + 8, 1, 12); }
  }
  static const int hx[4] = {38, 222, 38, 222}, hy[4] = {70, 70, 190, 190};
  for (int i = 0; i < 4; i++) { ring(GColorChromeYellow, hx[i], hy[i], 7, 4); disc(GColorBlack, hx[i], hy[i], 3); }
  ring(GColorWhite, 130, 130, 127, 126);                                     // board outline (silkscreen)

  // silkscreen title
  micro_c("RESISTOR CLOCK", 130, 18, 2, GColorWhite);
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  resistor(78, 40, 104, 22, hour, "R1");
  resistor(78, 86, 104, 22, s_min, "R2");

  // BCD LED matrix: HH MM SS, columns of 8-4-2-1 (unused high bits are shown as bare pads)
  GColor lit = s_aux == 0 ? GColorRed : (s_aux == 1 ? GColorGreen : GColorCyan);
  GColor glow = s_aux == 0 ? GColorBulgarianRose : (s_aux == 1 ? GColorDarkGreen : GColorCobaltBlue);
  GColor dim = s_aux == 0 ? GColorDarkCandyAppleRed : (s_aux == 1 ? GColorDarkGreen : GColorOxfordBlue);
  static const int cx[6] = {57, 83, 117, 143, 177, 203};
  int digits[6] = {hour / 10, hour % 10, s_min / 10, s_min % 10, s_sec / 10, s_sec % 10};
  static const int bits[6] = {2, 4, 3, 4, 3, 4};
  static const char *hdr[3] = {"HH", "MM", "SS"};
  for (int p = 0; p < 3; p++) micro_c(hdr[p], (cx[p * 2] + cx[p * 2 + 1]) / 2, 128, 1, GColorWhite);
  for (int c = 0; c < 6; c++) {
    for (int r = 0; r < 4; r++) {
      int weight = 8 >> r;
      int y = 144 + r * 18;
      if (r < 4 - bits[c]) { disc(GColorChromeYellow, cx[c], y, 2); continue; }   // unused bit
      led(cx[c], y, (digits[c] & weight) != 0, lit, glow, dim);
    }
  }
  static const char *wt[4] = {"8", "4", "2", "1"};
  for (int r = 0; r < 4; r++) micro_c(wt[r], 30, 142 + r * 18, 1, GColorWhite);
  // heartbeat LED
  led(222, 30, (s_tick & 1) != 0, GColorGreen, GColorDarkGreen, GColorDarkGreen);
  micro("PWR", 202, 40, 1, GColorWhite);
  // date and battery on the silkscreen
  char buf[32];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 222, 2, GColorWhite);
  snprintf(buf, sizeof(buf), "V1.0  BAT %d%%", s_batt);
  micro_c(buf, 130, 238, 1, s_batt <= 20 ? GColorRed : GColorWhite);
}
