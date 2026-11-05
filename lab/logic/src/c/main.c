// Logic Analyzer - the seconds and minute counters as digital timing diagrams over the last 30 seconds:
// bit 0..5 of SEC (cyan) and MIN (yellow), with a "NOW" cursor and decoded bus values (decimal + hex).
// Watch bit 0 toggle every second, bit 1 every two, bit 2 every four... a binary counter on your wrist.
//
// Wrist tap: show the HOUR/MIN counters instead of MIN/SEC.
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define NS 30            // samples shown
#define WX 66            // waveform left edge
#define SPW 5            // pixels per sample
#define ROW0 34
#define ROWH 12

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

// value of the "slow" counter (minutes, or hours) and the "fast" counter (seconds, or minutes) at an age
static void counters_at(int age_s, int *slow, int *fast) {
  int total = s_hour * 3600 + s_min * 60 + s_sec - age_s;
  if (total < 0) total += 86400;
  if (s_aux == 0) { *slow = (total / 60) % 60; *fast = total % 60; }
  else            { *slow = (total / 3600) % 24; *fast = (total / 60) % 60; }
}

static void face_draw(void) {
  const bool hm = s_aux == 1;
  micro_c(hm ? "LOGIC ANALYZER  HR/MIN" : "LOGIC ANALYZER  MIN/SEC", 130, 14, 1, GColorGreen);
  micro_c("TIMEBASE 1 S/DIV   TRIG: NOW", 130, 22, 1, GColorDarkGray);
  // grid: vertical divisions every 5 samples
  for (int i = 0; i <= NS; i += 5) {
    for (int y = ROW0 - 3; y < ROW0 + 12 * ROWH - 4; y += 3) PX(GColorDarkGray, WX + i * SPW, y);
  }
  // 12 channels: top 6 = slow counter bits 5..0, bottom 6 = fast counter bits 5..0
  for (int ch = 0; ch < 12; ch++) {
    int bit = 5 - (ch % 6);
    bool fast = ch >= 6;
    int y0 = ROW0 + ch * ROWH + (fast ? 4 : 0);
    GColor col = fast ? GColorCyan : GColorYellow;
    char lab[5];
    snprintf(lab, sizeof(lab), "%c%d", fast ? (hm ? 'M' : 'S') : (hm ? 'H' : 'M'), bit);
    micro(lab, 42, y0, 1, col);
    int prev = -1;
    for (int j = 0; j < NS; j++) {
      int slow_v, fast_v;
      counters_at(NS - 1 - j, &slow_v, &fast_v);
      int level = ((fast ? fast_v : slow_v) >> bit) & 1;
      int x = WX + j * SPW;
      int y = level ? y0 : y0 + 6;
      R(col, x, y, SPW + 1, 2);
      if (prev >= 0 && prev != level) R(col, x, y0, 2, 8);      // edge
      prev = level;
    }
  }
  // NOW cursor
  R(GColorFashionMagenta, WX + NS * SPW, ROW0 - 4, 1, 12 * ROWH + 4);
  micro("NOW", WX + NS * SPW - 6, ROW0 - 11, 1, GColorFashionMagenta);
  // bus decode
  int sv, fv;
  counters_at(0, &sv, &fv);
  char buf[40];
  snprintf(buf, sizeof(buf), "%s=%02d 0x%02X   %s=%02d 0x%02X", hm ? "HR" : "MIN", sv, sv, hm ? "MIN" : "SEC", fv, fv);
  RR(GColorDarkGray, 44, 184, 172, 14, 3);
  RR(GColorBlack, 46, 186, 168, 10, 2);
  micro_c(buf, 130, 188, 1, GColorGreen);
  // time plate
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorDarkGray, 64, 207, 132, 38, 6);
  RR(GColorBlack, 66, 209, 128, 34, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, 3, GColorGreen, GColorDarkGreen);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorGreen, 128, 219, 3, 3); R(GColorGreen, 128, 229, 3, 3); }
  snprintf(buf, sizeof(buf), "%d%%", s_batt);
  micro_r(buf, 214, 247, 1, s_batt <= 20 ? GColorRed : GColorGreen);
}
