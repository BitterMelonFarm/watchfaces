// Neon Grid - a cyberpunk watchface for Pebble Round 2 (gabbro).
// Everything (the big glitchy time, the micro labels, the skyline, the rain) is
// drawn with rectangles, so there are no assets and no dependence on system fonts.
//
// Wrist tap: cycle colour theme (NEON / BLADE / MATRIX) and fire a glitch burst.

#include <pebble.h>

#define STEP_GOAL 8000
#define NUM_THEMES 3
#define PERSIST_THEME 1

typedef struct {
  GColor sky[4];                       // horizontal sky bands, top to horizon
  GColor far_b, near_b;                // building silhouettes
  GColor pri, sec, acc, rain;          // text/neon colours
  GColor win_a, win_b;                 // lit window colours
  GColor panel;                        // HUD plate
  GColor glow;                         // bright band at the street-level horizon
} Theme;

static Theme s_th;
static int s_theme;

static void load_theme(int id) {
  switch (id) {
    default:
    case 0:  // NEON: cyan + hot magenta over a purple haze
      s_th = (Theme){{GColorBlack, GColorOxfordBlue, GColorImperialPurple, GColorIndigo},
                     GColorImperialPurple, GColorBlack,
                     GColorCyan, GColorFashionMagenta, GColorYellow, GColorTiffanyBlue,
                     GColorYellow, GColorShockingPink, GColorBlack, GColorPurple};
      break;
    case 1:  // BLADE: amber and teal under a red smog
      s_th = (Theme){{GColorBlack, GColorBulgarianRose, GColorDarkCandyAppleRed, GColorWindsorTan},
                     GColorBulgarianRose, GColorBlack,
                     GColorChromeYellow, GColorTiffanyBlue, GColorOrange, GColorWindsorTan,
                     GColorChromeYellow, GColorCyan, GColorBlack, GColorOrange};
      break;
    case 2:  // MATRIX: terminal green
      s_th = (Theme){{GColorBlack, GColorBlack, GColorBlack, GColorDarkGreen},
                     GColorDarkGreen, GColorBlack,
                     GColorGreen, GColorIslamicGreen, GColorMintGreen, GColorIslamicGreen,
                     GColorGreen, GColorMintGreen, GColorBlack, GColorIslamicGreen};
      break;
  }
}

static Window *s_window;
static Layer *s_canvas;

static int s_hour, s_min, s_sec, s_wday, s_mday, s_mon;
static int s_batt = 100;
static int s_steps;
static bool s_24h = true;
static bool s_in_focus = true;
static uint32_t s_tick;
static uint32_t s_glitch_until;        // tick until which a glitch burst runs

static GContext *g;

// ---------------------------------------------------------------- drawing helpers

static void R(GColor c, int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;
  graphics_context_set_fill_color(g, c);
  graphics_fill_rect(g, GRect(x, y, w, h), 0, GCornerNone);
}

static void RR(GColor c, int x, int y, int w, int h, int rad) {
  graphics_context_set_fill_color(g, c);
  graphics_fill_rect(g, GRect(x, y, w, h), rad, GCornersAll);
}

static uint32_t hash2(uint32_t a, uint32_t b) {
  uint32_t h = a * 2654435761u ^ (b + 0x9e3779b9u) * 40503u;
  h ^= h >> 13;
  h *= 0x5bd1e995u;
  h ^= h >> 15;
  return h;
}

// ---------------------------------------------------------------- fonts
// 3x5 micro font (15 bits, row-major) for labels.
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
  {'>', "100010001010100"}, {'<', "001010100010001"},
};

static int micro_width(const char *s, int sc) {
  int n = (int)strlen(s);
  return n ? n * 4 * sc - sc : 0;
}

static void micro(const char *s, int x, int y, int sc, GColor c) {
  for (; *s; s++, x += 4 * sc) {
    for (size_t i = 0; i < sizeof(MICRO) / sizeof(MICRO[0]); i++) {
      if (MICRO[i].ch != *s) continue;
      for (int r = 0; r < 5; r++) {
        for (int k = 0; k < 3; k++) {
          if (MICRO[i].bits[r * 3 + k] == '1') R(c, x + k * sc, y + r * sc, sc, sc);
        }
      }
      break;
    }
  }
}

static void micro_center(const char *s, int cx, int y, int sc, GColor c) {
  micro(s, cx - micro_width(s, sc) / 2, y, sc, c);
}

static void micro_right(const char *s, int rx, int y, int sc, GColor c) {
  micro(s, rx - micro_width(s, sc), y, sc, c);
}

// ---------------------------------------------------------------- background

typedef struct { int16_t x, w, h; } Bldg;
static const Bldg FAR_B[] = {
  {0, 24, 92}, {30, 18, 110}, {54, 26, 84}, {86, 20, 118}, {112, 24, 96},
  {142, 18, 124}, {166, 26, 88}, {198, 20, 112}, {224, 20, 98}, {250, 10, 106},
};
static const Bldg NEAR_B[] = {
  {0, 26, 60}, {32, 22, 78}, {60, 30, 48}, {96, 24, 90}, {126, 28, 64},
  {160, 22, 98}, {188, 26, 56}, {220, 24, 82}, {248, 12, 70},
};
#define BASE_Y 250

static void draw_city(void) {
  // sky bands with a hazy horizon
  R(s_th.sky[0], 0, 0, 260, 60);
  R(s_th.sky[1], 0, 60, 260, 50);
  R(s_th.sky[2], 0, 110, 260, 40);
  R(s_th.sky[3], 0, 150, 260, 50);
  R(s_th.glow, 0, 200, 260, 60);                               // neon glow behind the street
  R(s_th.sky[3], 0, 200, 260, 2);
  // far skyline: dim silhouettes with a few faint windows
  for (size_t i = 0; i < sizeof(FAR_B) / sizeof(FAR_B[0]); i++) {
    const Bldg *b = &FAR_B[i];
    int top = BASE_Y - b->h;
    R(s_th.far_b, b->x, top, b->w, b->h);
    for (int wy = top + 6; wy < BASE_Y - 4; wy += 8) {
      for (int wx = b->x + 3; wx < b->x + b->w - 2; wx += 6) {
        if (hash2((uint32_t)(b->x * 31 + wx), (uint32_t)(wy + s_tick / 9)) % 11 == 0) R(s_th.sky[3], wx, wy, 2, 2);
      }
    }
  }
  // hover-cars slipping through the alleys (hidden when they pass behind a tower)
  for (int i = 0; i < 3; i++) {
    int x = (int)(((s_tick * (5 + 3 * (uint32_t)i)) + 97 * (uint32_t)i) % 300) - 24;
    if (i == 1) x = 260 - x;                                    // this one flies the other way
    int y = 204 + i * 14;
    int dir = (i == 1) ? -1 : 1;
    R(s_th.near_b, x, y, 10, 3);
    R(s_th.acc, dir > 0 ? x + 10 : x - 2, y, 2, 1);               // headlight
    R(s_th.sec, dir > 0 ? x - 6 : x + 10, y + 1, 6, 1);           // light trail
  }
  // near skyline: darker, with brighter windows
  for (size_t i = 0; i < sizeof(NEAR_B) / sizeof(NEAR_B[0]); i++) {
    const Bldg *b = &NEAR_B[i];
    int top = BASE_Y - b->h;
    R(s_th.near_b, b->x, top, b->w, b->h);
    R(s_th.rain, b->x, top, b->w, 1);                          // roof edge light
    for (int wy = top + 5; wy < BASE_Y - 6; wy += 7) {
      for (int wx = b->x + 3; wx < b->x + b->w - 3; wx += 6) {
        uint32_t hv = hash2((uint32_t)(b->x * 17 + wx), (uint32_t)(wy * 3 + s_tick / 7));
        if (hv % 9 == 0) R((hv >> 8) & 1 ? s_th.win_a : s_th.win_b, wx, wy, 3, 3);
      }
    }
  }
  // neon billboards that flicker
  if (hash2(1, s_tick) % 9 != 0) {
    R(s_th.panel, 93, 176, 24, 13);
    R(s_th.sec, 93, 176, 24, 1);
    R(s_th.sec, 93, 188, 24, 1);
    micro("BAR", 96, 179, 2, s_th.sec);
  }
  if (hash2(2, s_tick) % 11 != 0) {
    R(s_th.panel, 154, 190, 38, 13);
    R(s_th.pri, 154, 190, 38, 1);
    R(s_th.pri, 154, 202, 38, 1);
    micro("RAMEN", 156, 193, 2, s_th.pri);
  }
  if (hash2(3, s_tick) % 8 != 0) {                               // vertical hotel sign
    R(s_th.acc, 61, 182, 3, 30);
    R(s_th.panel, 62, 184, 1, 26);
  }
  // wet street shine
  R(s_th.near_b, 0, 244, 260, 16);
  R(s_th.sec, 70, 246, 36, 1);
  R(s_th.pri, 150, 248, 44, 1);
}

static void draw_rain(void) {
  for (int i = 0; i < 34; i++) {
    int x = (int)(hash2((uint32_t)i, 5) % 260);
    int y = (int)(((uint32_t)i * 97 + s_tick * 23) % 290) - 12;
    R(s_th.rain, x, y, 1, 7);
    R(s_th.rain, x - 1, y + 7, 1, 2);
  }
}

// MATRIX theme: falling columns of code instead of rain.
static void draw_code_rain(void) {
  static const char GLYPHS[] = "0123456789ABCDEFXZ";
  for (int i = 0; i < 26; i++) {
    int x = 4 + i * 10;
    int head = (int)((hash2((uint32_t)i, 9) % 300 + s_tick * (7 + (uint32_t)(i % 5) * 2)) % 320) - 30;
    for (int k = 0; k < 9; k++) {
      int y = head - k * 7;
      if (y < -6 || y > 258) continue;
      char buf[2] = {GLYPHS[hash2((uint32_t)(i * 13 + k), s_tick / 2 + (uint32_t)k) % (sizeof(GLYPHS) - 1)], 0};
      micro(buf, x, y, 1, k == 0 ? GColorWhite : (k < 3 ? s_th.pri : (k < 6 ? s_th.sec : s_th.rain)));
    }
  }
}

static void draw_scanlines(void) {
  for (int y = 1; y < 260; y += 4) R(s_th.sky[0], 0, y, 260, 1);
  int sweep = (int)((s_tick * 13) % 280) - 10;                  // slow scanning bar
  R(s_th.rain, 0, sweep, 260, 1);
  R(s_th.sky[1], 0, sweep + 1, 260, 1);
}

// ---------------------------------------------------------------- time
// Neon-tube digits: chamfered 7-segment shapes on a 7x11 cell grid, each segment
// drawn as a dim glow halo, a coloured tube and a white-hot core.

static bool glitching(void) {
  return (s_tick % 8) == 0 || s_glitch_until > s_tick;
}

static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
typedef struct { int8_t x, y, w, h; } SegRect;
static const SegRect SEGR[7] = {      // a, b, c, d, e, f, g
  {1, 0, 5, 1}, {6, 1, 1, 4}, {6, 6, 1, 4}, {1, 10, 5, 1},
  {0, 6, 1, 4}, {0, 1, 1, 4}, {1, 5, 5, 1},
};

// pass 0 = glow halo, 1 = tube colour, 2 = white core
static void seg_pass(int d, int x, int y, int sc, int pass, GColor tube) {
  for (int i = 0; i < 7; i++) {
    if (!(SEG[d] & (1 << i))) continue;
    int rx = x + SEGR[i].x * sc, ry = y + SEGR[i].y * sc;
    int rw = SEGR[i].w * sc, rh = SEGR[i].h * sc;
    bool horiz = SEGR[i].w > SEGR[i].h;
    if (pass == 0) R(s_th.rain, rx - 3, ry - 3, rw + 6, rh + 6);
    else if (pass == 1) R(tube, rx - 1, ry - 1, rw + 2, rh + 2);
    else if (horiz) R(GColorWhite, rx + 1, ry + 1, rw - 2, rh - 2);
    else R(GColorWhite, rx + 1, ry + 1, rw - 2, rh - 2);
  }
}

static void tube_digit(int d, int x, int y, int sc, bool glow) {
  if (glow) seg_pass(d, x, y, sc, 0, s_th.pri);
  seg_pass(d, x, y, sc, 1, s_th.pri);
  seg_pass(d, x, y, sc, 2, s_th.pri);
}

static void tube_colon(int cx, int y, bool glow) {
  for (int k = 0; k < 2; k++) {
    int yy = y + (k ? 28 : 12);
    if (glow) R(s_th.rain, cx - 5, yy - 2, 10, 10);
    R(s_th.pri, cx - 3, yy, 6, 6);
    R(GColorWhite, cx - 1, yy + 2, 2, 2);
  }
}

static void draw_time(void) {
  int hour = s_hour;
  if (!s_24h) {
    hour = hour % 12;
    if (hour == 0) hour = 12;
  }
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  if (!s_24h && dg[0] == 0) dg[0] = -1;                         // no leading zero in 12h mode
  const int sc = 4, ty = 60;
  const int xs[4] = {52, 88, 144, 180};
  bool gl = glitching();
  int torn = gl ? (int)(s_tick % 4) : -1;                       // which digit slips sideways
  // chromatic-aberration ghosts while glitching
  if (gl) {
    for (int i = 0; i < 4; i++) {
      if (dg[i] < 0) continue;
      int off = (i == torn) ? ((s_tick & 1) ? 6 : -6) : 0;
      seg_pass(dg[i], xs[i] + off - 3, ty, sc, 1, s_th.sec);
      seg_pass(dg[i], xs[i] + off + 3, ty, sc, 1, s_th.acc);
    }
  }
  for (int i = 0; i < 4; i++) {
    if (dg[i] < 0) continue;
    int off = (gl && i == torn) ? ((s_tick & 1) ? 6 : -6) : 0;
    tube_digit(dg[i], xs[i] + off, ty, sc, true);
  }
  if (!(s_tick & 1) || !s_in_focus) tube_colon(130, ty, true);   // blinking colon
  // HUD brackets around the time
  R(s_th.pri, 33, 56, 3, 52);  R(s_th.pri, 33, 56, 9, 3);  R(s_th.pri, 33, 105, 9, 3);
  R(s_th.pri, 224, 56, 3, 52); R(s_th.pri, 218, 56, 9, 3); R(s_th.pri, 218, 105, 9, 3);
  if (!s_24h) micro_right(s_hour >= 12 ? "PM" : "AM", 218, 46, 2, s_th.acc);
  // seconds ruler
  for (int i = 0; i < 60; i++) {
    int x = 55 + (i * 150) / 60;
    bool major = (i % 5) == 0;
    R(i <= s_sec ? (major ? s_th.acc : s_th.pri) : s_th.rain, x, major ? 111 : 112, 1, major ? 5 : 3);
  }
}

// ---------------------------------------------------------------- HUD

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                               "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void seg_bar(int x, int y, int lit, int total, GColor on, GColor on_hi, GColor off) {
  for (int i = 0; i < total; i++) {
    R(i < lit ? (i >= total - 6 ? on_hi : on) : off, x + i * 8, y, 7, 6);
  }
}

static void draw_hud(void) {
  char buf[16];
  // title and date along the top
  micro_center("NEON GRID", 130, 17, 2, s_th.rain);
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_center(buf, 130, 32, 3, s_th.pri);
  // data plate: neon border, dark fill
  RR(s_th.sec, 36, 120, 188, 56, 5);
  RR(s_th.panel, 38, 122, 184, 52, 4);
  // steps
  micro("STEP", 46, 127, 2, s_th.pri);
  snprintf(buf, sizeof(buf), "%d", s_steps);
  micro_right(buf, 214, 127, 2, s_steps >= STEP_GOAL ? s_th.acc : GColorWhite);
  int lit = s_steps >= STEP_GOAL ? 20 : (s_steps * 20) / STEP_GOAL;
  bool goal = s_steps >= STEP_GOAL;
  seg_bar(46, 139, lit, 20, goal ? s_th.acc : s_th.pri, s_th.acc, s_th.rain);
  // power
  micro("PWR", 46, 152, 2, s_th.pri);
  snprintf(buf, sizeof(buf), "%d%%", s_batt);
  micro_right(buf, 214, 152, 2, s_batt <= 20 ? s_th.sec : GColorWhite);
  int bl = (s_batt + 4) / 5;
  bool low = s_batt <= 20;
  if (!(low && (s_tick & 1))) seg_bar(46, 164, bl, 20, low ? s_th.sec : s_th.pri, low ? s_th.sec : s_th.acc, s_th.rain);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  g = ctx;
  R(s_th.sky[0], 0, 0, 260, 260);
  draw_city();
  if (s_theme == 2) draw_code_rain(); else draw_rain();
  draw_scanlines();
  draw_time();
  draw_hud();
}

// ---------------------------------------------------------------- events

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hour = t->tm_hour;
  s_min = t->tm_min;
  s_sec = t->tm_sec;
  s_wday = t->tm_wday;
  s_mday = t->tm_mday;
  s_mon = t->tm_mon;
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
    s_glitch_until = s_tick + 3;                                // glitch burst on every new minute
  }
  layer_mark_dirty(s_canvas);
}

// Animate once a second only while visible and the battery is healthy.
static void update_tick_rate(void) {
  TimeUnits u = (s_in_focus && s_batt > 15) ? SECOND_UNIT : MINUTE_UNIT;
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
  s_theme = (s_theme + 1) % NUM_THEMES;
  load_theme(s_theme);
  persist_write_int(PERSIST_THEME, s_theme);
  s_glitch_until = s_tick + 4;
  layer_mark_dirty(s_canvas);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
}

static void init(void) {
  s_theme = persist_exists(PERSIST_THEME) ? persist_read_int(PERSIST_THEME) % NUM_THEMES : 0;
  if (s_theme < 0) s_theme = 0;
  load_theme(s_theme);

  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  update_time();
  update_steps();
  s_batt = battery_state_service_peek().charge_percent;

  update_tick_rate();
  app_focus_service_subscribe(focus_handler);
  battery_state_service_subscribe(battery_handler);
  accel_tap_service_subscribe(tap_handler);
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
#endif
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  app_focus_service_unsubscribe();
  battery_state_service_unsubscribe();
  accel_tap_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
