// Shared framework for the lab watchfaces (Pebble Round 2 / gabbro, 260x260).
//
// A face includes this header and defines three hooks:
//     static void face_init(void);   // once, after settings are loaded
//     static void face_draw(void);   // draw one frame using the helpers below
//     static void face_tap(void);    // wrist flick (usually changes s_aux)
// and optionally  #define FACE_SECONDS 1  (animate once a second) before including.
//
// Everything is drawn with rectangles (no assets, no system fonts) so the host-side
// preview renderer in lab/tools matches the watch pixel for pixel.
#pragma once
#pragma GCC diagnostic ignored "-Wunused-function"
#include <pebble.h>

#ifndef FACE_SECONDS
#define FACE_SECONDS 0
#endif
#define SW_ 260
#define PERSIST_AUX 1
#define STEP_GOAL 8000

// ------------------------------------------------------------------ state
static int s_year = 2026, s_mon, s_mday = 1, s_wday, s_hour, s_min, s_sec;
static int s_batt = 100, s_steps;
static bool s_24h = true, s_in_focus = true;
static uint32_t s_tick;
static int s_aux;                       // face-specific mode, saved across restarts
static GContext *g;
static Window *s_window;
static Layer *s_canvas;

static void face_init(void);
static void face_draw(void);
static void face_tap(void);

// ------------------------------------------------------------------ integer helpers
static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int iabs(int a) { return a < 0 ? -a : a; }
static int isqrt(int n) {
  if (n <= 0) return 0;
  int x = n, y = (x + 1) / 2;
  while (y < x) { x = y; y = (x + n / x) / 2; }
  return x;
}
static uint32_t hash2(uint32_t a, uint32_t b) {
  uint32_t h = a * 2654435761u ^ (b + 0x9e3779b9u) * 40503u;
  h ^= h >> 13;
  h *= 0x5bd1e995u;
  h ^= h >> 15;
  return h;
}

// sin/cos in Q10 (1024 = 1.0) for whole degrees; angle 0 = +x axis (maths convention)
static const int16_t SIN_Q10[91] = {
  0, 18, 36, 54, 71, 89, 107, 125, 143, 160, 178, 195, 213,
  230, 248, 265, 282, 299, 316, 333, 350, 367, 384, 400, 416, 433,
  449, 465, 481, 496, 512, 527, 543, 558, 573, 587, 602, 616, 630,
  644, 658, 672, 685, 698, 711, 724, 737, 749, 761, 773, 784, 796,
  807, 818, 828, 839, 849, 859, 868, 878, 887, 896, 904, 912, 920,
  928, 935, 943, 949, 956, 962, 968, 974, 979, 984, 989, 994, 998,
  1002, 1005, 1008, 1011, 1014, 1016, 1018, 1020, 1022, 1023, 1023, 1024, 1024,
};
static int isin(int deg) {
  deg %= 360;
  if (deg < 0) deg += 360;
  if (deg <= 90) return SIN_Q10[deg];
  if (deg <= 180) return SIN_Q10[180 - deg];
  if (deg <= 270) return -SIN_Q10[deg - 180];
  return -SIN_Q10[360 - deg];
}
static int icos(int deg) { return isin(deg + 90); }
// point at radius r and clock angle (0 = 12 o'clock, clockwise)
static void clock_pt(int cx, int cy, int r, int deg, int *x, int *y) {
  *x = cx + (r * isin(deg) + (r >= 0 ? 512 : -512)) / 1024;
  *y = cy - (r * icos(deg) + (r >= 0 ? 512 : -512)) / 1024;
}
// point at radius r and maths angle (0 = +x, counter-clockwise, y up)
static void math_pt(int cx, int cy, int r, int deg, int *x, int *y) {
  *x = cx + (r * icos(deg) + (r >= 0 ? 512 : -512)) / 1024;
  *y = cy - (r * isin(deg) + (r >= 0 ? 512 : -512)) / 1024;
}

// ------------------------------------------------------------------ drawing
static void R(GColor c, int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  graphics_context_set_fill_color(g, c);
  graphics_fill_rect(g, GRect(x, y, w, h), 0, GCornerNone);
}
static void RR(GColor c, int x, int y, int w, int h, int rad) {
  graphics_context_set_fill_color(g, c);
  graphics_fill_rect(g, GRect(x, y, w, h), rad, GCornersAll);
}
static void PX(GColor c, int x, int y) { R(c, x, y, 1, 1); }

static void disc(GColor c, int cx, int cy, int r) {
  for (int dy = -r; dy <= r; dy++) {
    int dx = isqrt(r * r + r - dy * dy);
    R(c, cx - dx, cy + dy, 2 * dx + 1, 1);
  }
}
// annulus between radii ri (inner, exclusive) and ro (outer, inclusive)
static void ring(GColor c, int cx, int cy, int ro, int ri) {
  for (int dy = -ro; dy <= ro; dy++) {
    int dxo = isqrt(ro * ro + ro - dy * dy);
    if (iabs(dy) < ri) {
      int dxi = isqrt(ri * ri + ri - dy * dy);
      R(c, cx - dxo, cy + dy, dxo - dxi, 1);
      R(c, cx + dxi + 1, cy + dy, dxo - dxi, 1);
    } else {
      R(c, cx - dxo, cy + dy, 2 * dxo + 1, 1);
    }
  }
}
static void line(GColor c, int x0, int y0, int x1, int y1, int th) {
  int dx = iabs(x1 - x0), dy = -iabs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  int o = th / 2;
  for (;;) {
    R(c, x0 - o, y0 - o, th, th);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}
static void tri(GColor c, int x0, int y0, int x1, int y1, int x2, int y2) {
  int t;
  if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
  if (y1 > y2) { t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
  if (y0 > y1) { t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
  int total = y2 - y0;
  if (total == 0) {
    R(c, imin(x0, imin(x1, x2)), y0, imax(x0, imax(x1, x2)) - imin(x0, imin(x1, x2)) + 1, 1);
    return;
  }
  for (int y = y0; y <= y2; y++) {
    bool second = y > y1 || y1 == y0;
    int seg = second ? y2 - y1 : y1 - y0;
    int xa = x0 + (x2 - x0) * (y - y0) / total;
    int xb = second ? x1 + (x2 - x1) * (y - y1) / (seg ? seg : 1)
                    : x0 + (x1 - x0) * (y - y0) / (seg ? seg : 1);
    if (xa > xb) { t = xa; xa = xb; xb = t; }
    R(c, xa, y, xb - xa + 1, 1);
  }
}
// thick arc from clock angle a0 to a1 (degrees, clockwise), radius r, thickness th
static void arc(GColor c, int cx, int cy, int r, int th, int a0, int a1) {
  int steps = (a1 - a0) * 3;
  for (int i = 0; i <= steps; i++) {
    int x, y;
    clock_pt(cx, cy, r, a0 + (i * 1) / 3, &x, &y);
    R(c, x - th / 2, y - th / 2, th, th);
  }
}

// 3x5 micro font, 15 bits row-major
typedef struct { char ch; const char *bits; } Glyph;
static const Glyph MICRO[] = {
  {'0', "111101101101111"}, {'1', "010110010010111"}, {'2', "111001111100111"},
  {'3', "111001111001111"}, {'4', "101101111001001"}, {'5', "111100111001111"},
  {'6', "111100111101111"}, {'7', "111001001001001"}, {'8', "111101111101111"},
  {'9', "111101111001111"}, {'A', "010101111101101"}, {'B', "110101110101110"},
  {'C', "011100100100011"}, {'D', "110101101101110"}, {'E', "111100110100111"},
  {'F', "111100110100100"}, {'G', "011100101101011"}, {'H', "101101111101101"},
  {'I', "111010010010111"}, {'J', "001001001101010"}, {'K', "101101110101101"},
  {'L', "100100100100111"}, {'M', "101111111101101"}, {'N', "110101101101101"},
  {'O', "010101101101010"}, {'P', "110101110100100"}, {'Q', "010101101110011"},
  {'R', "110101110101101"}, {'S', "011100010001110"}, {'T', "111010010010010"},
  {'U', "101101101101111"}, {'V', "101101101101010"}, {'W', "101101111111101"},
  {'X', "101101010101101"}, {'Y', "101101010010010"}, {'Z', "111001010100111"},
  {':', "000010000010000"}, {'%', "101001010100101"}, {'/', "001001010100100"},
  {'-', "000000111000000"}, {'.', "000000000000010"}, {'_', "000000000000111"},
  {'>', "100010001010100"}, {'<', "001010100010001"}, {'+', "000010111010000"},
  {'=', "000111000111000"}, {'*', "101010111010101"}, {'(', "010100100100010"},
  {')', "010001001001010"}, {'\'', "010010000000000"}, {'^', "010101000000000"},
};
static int micro_width(const char *s, int sc) {
  int n = (int)strlen(s);
  return n ? n * 4 * sc - sc : 0;
}
static void micro(const char *s, int x, int y, int sc, GColor c) {
  for (; *s; s++, x += 4 * sc) {
    char ch = *s;
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 32);
    for (size_t i = 0; i < sizeof(MICRO) / sizeof(MICRO[0]); i++) {
      if (MICRO[i].ch != ch) continue;
      for (int r = 0; r < 5; r++)
        for (int k = 0; k < 3; k++)
          if (MICRO[i].bits[r * 3 + k] == '1') R(c, x + k * sc, y + r * sc, sc, sc);
      break;
    }
  }
}
static void micro_c(const char *s, int cx, int y, int sc, GColor c) { micro(s, cx - micro_width(s, sc) / 2, y, sc, c); }
static void micro_r(const char *s, int rx, int y, int sc, GColor c) { micro(s, rx - micro_width(s, sc), y, sc, c); }

// 7-segment neon-tube digit on a 7x11 grid (cell = sc px): glow halo, coloured tube, white core
static const uint8_t SEG7[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
static const int8_t SEGR7[7][4] = {{1, 0, 5, 1}, {6, 1, 1, 4}, {6, 6, 1, 4}, {1, 10, 5, 1},
                                   {0, 6, 1, 4}, {0, 1, 1, 4}, {1, 5, 5, 1}};
static void tube_digit(int d, int x, int y, int sc, GColor tube, GColor glow) {
  for (int pass = 0; pass < 3; pass++) {
    for (int i = 0; i < 7; i++) {
      if (!(SEG7[d] & (1 << i))) continue;
      int rx = x + SEGR7[i][0] * sc, ry = y + SEGR7[i][1] * sc;
      int rw = SEGR7[i][2] * sc, rh = SEGR7[i][3] * sc;
      if (pass == 0) R(glow, rx - 3, ry - 3, rw + 6, rh + 6);
      else if (pass == 1) R(tube, rx - 1, ry - 1, rw + 2, rh + 2);
      else R(GColorWhite, rx + 1, ry + 1, rw - 2, rh - 2);
    }
  }
}

// ------------------------------------------------------------------ calendar helpers
// days from 1970-01-01 for a civil date (month 1..12)
static int days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}
// seconds since J2000 (2000-01-01 12:00), treating the local clock as UTC (fine for visuals)
static int32_t secs_since_j2000(void) {
  int days = days_from_civil(s_year, s_mon + 1, s_mday) - 10957;   // 2000-01-01 is day 10957
  return days * 86400 + (s_hour - 12) * 3600 + s_min * 60 + s_sec;
}

// ------------------------------------------------------------------ framework
static void update_time(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_year = t->tm_year + 1900;
  s_mon = t->tm_mon;
  s_mday = t->tm_mday;
  s_wday = t->tm_wday;
  s_hour = t->tm_hour;
  s_min = t->tm_min;
  s_sec = t->tm_sec;
  s_24h = clock_is_24h_style();
}

static void update_steps(void) {
#if defined(PBL_HEALTH)
  HealthServiceAccessibilityMask m =
      health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL));
  s_steps = (m & HealthServiceAccessibilityMaskAvailable)
                ? (int)health_service_sum_today(HealthMetricStepCount) : 0;
#else
  s_steps = 0;
#endif
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    update_steps();
    layer_mark_dirty(s_canvas);
  }
}
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_tick++;
  s_sec = tick_time->tm_sec;
  if (units_changed & MINUTE_UNIT) {
    update_time();
    update_steps();
  }
  layer_mark_dirty(s_canvas);
}

// Animate once a second only while the face is visible and the battery healthy.
static void update_tick_rate(void) {
  TimeUnits u = (FACE_SECONDS && s_in_focus && s_batt > 15) ? SECOND_UNIT : MINUTE_UNIT;
  tick_timer_service_subscribe(u, tick_handler);
}
static void focus_handler(bool in_focus) {
  s_in_focus = in_focus;
  update_tick_rate();
  layer_mark_dirty(s_canvas);
}
static void battery_handler(BatteryChargeState state) {
  s_batt = state.charge_percent;
  update_tick_rate();
  layer_mark_dirty(s_canvas);
}
static void tap_handler(AccelAxisType axis, int32_t direction) {
  face_tap();
  persist_write_int(PERSIST_AUX, s_aux);
  layer_mark_dirty(s_canvas);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  g = ctx;
  R(GColorBlack, 0, 0, SW_, SW_);
  face_draw();
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}
static void window_unload(Window *window) { layer_destroy(s_canvas); }

static void face_boot(void) {
  s_aux = persist_exists(PERSIST_AUX) ? (int)persist_read_int(PERSIST_AUX) : 0;
  if (s_aux < 0) s_aux = 0;
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){.load = window_load, .unload = window_unload});
  window_stack_push(s_window, true);
  update_time();
  update_steps();
  s_batt = battery_state_service_peek().charge_percent;
  face_init();
  update_tick_rate();
  app_focus_service_subscribe(focus_handler);
  battery_state_service_subscribe(battery_handler);
  accel_tap_service_subscribe(tap_handler);
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
#endif
}
static void face_shutdown(void) {
  tick_timer_service_unsubscribe();
  app_focus_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}

#ifndef FACE_PREVIEW
int main(void) {
  face_boot();
  app_event_loop();
  face_shutdown();
}
#endif
