// Desk Pet - a tiny Tamagotchi-style watchface for Pebble Round 2 (gabbro).
// The pet's activity follows the time of day: sleeping, waking, coding,
// lunch, dinner, playing with the kids, and watching TV on weekends.
// All art is drawn with rectangles on a 32x24 "pixel" grid, so no assets.

#include <pebble.h>

typedef enum {
  ACT_SLEEP,
  ACT_WAKE,
  ACT_CODE,
  ACT_LUNCH,
  ACT_DINNER,
  ACT_PLAY,
  ACT_CHILL,
} Activity;

#define SCENE_W 32
#define SCENE_H 21
#define FLOOR_Y 18
#define STEP_GOAL 8000

typedef enum { MOOD_OK, MOOD_SAD, MOOD_HAPPY } Mood;

static Window *s_window;
static Layer *s_canvas;

static int s_hour, s_min, s_wday, s_mday;
static int s_batt = 100;
static int s_steps;
static Mood s_mood = MOOD_OK;
static uint32_t s_tick;
static char s_time_buf[8];
static char s_date_buf[16];

// Drawing state for the scene currently being rendered.
static GContext *g;
static int U;    // screen pixels per scene pixel
static int OX;   // scene origin x on screen
static int OY;   // scene origin y on screen

// ---------------------------------------------------------------- helpers

static void box(GColor c, int x, int y, int w, int h) {
  graphics_context_set_fill_color(g, c);
  graphics_fill_rect(g, GRect(OX + x * U, OY + y * U, w * U, h * U), 0, GCornerNone);
}

static void txt(const char *s, const char *font_key, GColor c, int x, int y, int w, int h) {
  graphics_context_set_text_color(g, c);
  graphics_draw_text(g, s, fonts_get_system_font(font_key),
                     GRect(OX + x * U, OY + y * U, w * U, h * U),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

static void heart_px(GColor c, int x, int y) {
  box(c, x, y, 1, 1);
  box(c, x + 2, y, 1, 1);
  box(c, x, y + 1, 3, 1);
  box(c, x + 1, y + 2, 1, 1);
}

// Pet palette: a soft pink blob with a deeper pink outline.
#define PET_BODY GColorMelon
#define PET_OUT  GColorShockingPink

static int s_bob;   // 1 = pet is standing and hops on odd frames

static void px(int x, int y, GColor c) { box(c, x, y, 1, 1); }

// The grown-up pet: 8 wide, 8 tall (plus ear nubs one row above).
// mouth: 0 = closed, 1 = open (chewing / talking)
static void pet(int x, int y, bool blink, int mouth) {
  if (s_bob && (s_tick & 1)) y -= 1;               // little hop
  // ear nubs
  px(x + 1, y - 1, PET_OUT);
  px(x + 6, y - 1, PET_OUT);
  // body: outline, then fill
  box(PET_OUT, x + 1, y, 6, 1);
  box(PET_OUT, x, y + 1, 8, 5);
  box(PET_OUT, x + 1, y + 6, 6, 1);
  box(PET_BODY, x + 1, y + 1, 6, 5);
  box(PET_BODY, x + 2, y + 6, 4, 1);
  // feet
  box(PET_OUT, x + 1, y + 7, 2, 1);
  box(PET_OUT, x + 5, y + 7, 2, 1);
  // big shiny eyes (2x2 each) or sleepy blink
  if (blink) {
    box(GColorBlack, x + 1, y + 3, 2, 1);
    box(GColorBlack, x + 5, y + 3, 2, 1);
  } else if (s_mood == MOOD_HAPPY) {
    px(x + 1, y + 3, GColorBlack);                 // happy ^ ^ eyes
    px(x + 2, y + 2, GColorBlack);
    px(x + 5, y + 2, GColorBlack);
    px(x + 6, y + 3, GColorBlack);
  } else {
    box(GColorBlack, x + 1, y + 2, 2, 2);
    box(GColorBlack, x + 5, y + 2, 2, 2);
    px(x + 1, y + 2, GColorWhite);                 // eye shine
    px(x + 5, y + 2, GColorWhite);
  }
  // blush
  px(x + 1, y + 4, GColorRed);
  px(x + 6, y + 4, GColorRed);
  // mouth
  if (mouth) {
    box(GColorRed, x + 3, y + 4, 2, 2);
  } else if (s_mood == MOOD_SAD) {
    box(GColorBlack, x + 3, y + 4, 2, 1);          // frown
    px(x + 2, y + 5, GColorBlack);
    px(x + 5, y + 5, GColorBlack);
  } else if (s_mood == MOOD_HAPPY) {
    px(x + 2, y + 4, GColorBlack);                 // big smile
    px(x + 5, y + 4, GColorBlack);
    box(GColorBlack, x + 3, y + 5, 2, 1);
  } else {
    px(x + 3, y + 4, GColorBlack);                 // tiny "w" mouth
    px(x + 4, y + 4, GColorBlack);
  }
  if (s_mood == MOOD_SAD) {
    box(GColorPictonBlue, x + 2, y + 4, 1, 1 + (int)(s_tick & 1));   // tear
    // little rain cloud overhead
    box(GColorLightGray, x + 1, y - 4, 6, 2);
    box(GColorLightGray, x + 2, y - 5, 3, 1);
    px(x + 2 + 2 * (int)(s_tick & 1), y - 2, GColorPictonBlue);
    px(x + 5 - 2 * (int)(s_tick & 1), y - 2, GColorPictonBlue);
  } else if (s_mood == MOOD_HAPPY) {
    // twinkling sparkles
    int sx = (s_tick & 1) ? x + 7 : x;
    int sy = y - 5;
    box(GColorYellow, sx, sy, 1, 3);
    box(GColorYellow, sx - 1, sy + 1, 3, 1);
    px(sx, sy + 1, GColorWhite);
  }
}

// A smaller kid blob: 5 wide, 4 tall, with a tuft of hair on top.
static void kid(int x, int y, GColor c, bool happy) {
  px(x + 2, y - 1, GColorRed);                     // hair tuft
  box(c, x + 1, y, 3, 1);
  box(c, x, y + 1, 5, 2);
  box(c, x + 1, y + 3, 3, 1);
  px(x + 1, y + 1, GColorBlack);
  px(x + 3, y + 1, GColorBlack);
  px(x, y + 2, GColorMelon);                       // blush
  px(x + 4, y + 2, GColorMelon);
  if (happy) {
    px(x + 2, y + 2, GColorRed);                   // open smile
  } else {
    px(x + 2, y + 2, GColorBlack);
  }
}

static void room(GColor wall, GColor floor_c) {
  box(wall, 0, 0, SCENE_W, FLOOR_Y);
  box(floor_c, 0, FLOOR_Y, SCENE_W, SCENE_H - FLOOR_Y);
}

// ----------------------------------------------------------------- scenes

static void scene_sleep(void) {
  GColor wall = GColorOxfordBlue;
  room(wall, GColorImperialPurple);
  // moon
  box(GColorPastelYellow, 24, 3, 4, 4);
  box(wall, 25, 2, 3, 3);
  // twinkling stars
  for (int i = 0; i < 4; i++) {
    static const int sx[4] = {5, 12, 18, 21};
    static const int sy[4] = {3, 6, 2, 8};
    if (((s_tick + i) % 3) != 0) {
      box(GColorWhite, sx[i], sy[i], 1, 1);
    }
  }
  // bed
  box(GColorWindsorTan, 5, 18, 22, 2);
  box(GColorWindsorTan, 5, 12, 1, 8);
  box(GColorWhite, 6, 12, 7, 3);                 // pillow
  box(PET_OUT, 7, 11, 1, 1);                     // ears
  box(PET_OUT, 11, 11, 1, 1);
  box(PET_BODY, 7, 12, 5, 3);                    // head
  box(GColorBlack, 8, 13, 1, 1);                 // closed eyes
  box(GColorBlack, 10, 13, 1, 1);
  box(GColorRed, 7, 14, 1, 1);                   // blush
  box(GColorRed, 11, 14, 1, 1);
  if (s_tick & 2) box(GColorCeleste, 12, 12, 2, 2);  // sleepy snot bubble
  else box(GColorCeleste, 12, 12, 1, 1);
  box(GColorCobaltBlue, 12, 13, 14, 5);          // blanket
  box(GColorBlueMoon, 12, 13, 14, 1);
  // floating z's
  static const char *zs[3] = {"z", "zz", "zzz"};
  txt(zs[(s_tick / 2) % 3], FONT_KEY_GOTHIC_24_BOLD, GColorWhite, 13, 4, 10, 8);
}

static void scene_wake(void) {
  room(GColorPastelYellow, GColorRajah);
  // window with rising sun
  box(GColorWhite, 2, 2, 9, 9);
  box(GColorCeleste, 3, 3, 7, 7);
  box(GColorYellow, 5, 5, 3, 3);
  box(GColorWhite, 6, 2, 1, 9);
  // stretching pet
  int pxx = 14, pyy = 10;
  s_bob = 1;
  pet(pxx, pyy, false, (s_tick & 1));
  s_bob = 0;
  if ((s_tick & 1) == 0) {
    box(PET_OUT, pxx - 1, pyy - 2, 1, 3);          // arms up
    box(PET_OUT, pxx + 8, pyy - 2, 1, 3);
  } else {
    box(PET_OUT, pxx - 2, pyy + 4, 2, 1);          // arms out
    box(PET_OUT, pxx + 8, pyy + 4, 2, 1);
  }
  // coffee waiting on the floor
  box(GColorWhite, 26, 15, 3, 3);
  box(GColorWindsorTan, 26, 15, 3, 1);
  box(GColorWhite, 25 + (s_tick & 1), 13, 1, 1);
}

static void scene_code(void) {
  room(GColorBabyBlueEyes, GColorDarkGray);
  // plant
  box(GColorIslamicGreen, 29, 10, 3, 6);
  box(GColorWindsorTan, 29, 16, 3, 2);
  // chair (back visible behind the pet)
  box(GColorBulgarianRose, 1, 8, 2, 8);
  box(GColorBulgarianRose, 2, 15, 8, 1);
  box(GColorDarkGray, 5, 16, 1, 3);
  box(GColorBlack, 3, 19, 6, 1);
  // pet sitting at the desk
  pet(2, 7, (s_tick % 7) == 0, 0);
  // monitor
  box(GColorDarkGray, 17, 12, 3, 2);
  box(GColorDarkGray, 12, 4, 13, 9);
  box(GColorBlack, 13, 5, 11, 7);
  static const GColor code_cols[3] = {GColorGreen, GColorCyan, GColorYellow};
  for (int i = 0; i < 3; i++) {
    int w = 2 + ((s_tick / 2 + i * 3) % 5);
    int x = 14 + ((i + s_tick / 6) % 3);
    box(code_cols[i], x, 6 + i * 2, w, 1);
  }
  if (s_tick & 1) {
    box(GColorWhite, 14, 11, 1, 1);               // blinking cursor
  }
  // desk, keyboard, typing hands
  box(GColorLightGray, 13, 13, 8, 1);
  box(PET_OUT, 10, 12 + (s_tick & 1), 3, 1);
  box(GColorWindsorTan, 10, 14, 19, 2);
  box(GColorWindsorTan, 11, 16, 2, 4);
  box(GColorWindsorTan, 26, 16, 2, 4);
  // coffee mug with steam
  box(GColorWhite, 26, 12, 2, 2);
  box(GColorWhite, 28, 12, 1, 1);
  box(GColorLightGray, 26 + (s_tick & 1), 10, 1, 1);
}

static void table_scene(bool family) {
  room(GColorPastelYellow, GColorRajah);
  int mouth = (s_tick & 1);
  // window
  box(GColorWhite, 12, 2, 8, 7);
  box(GColorCeleste, 13, 3, 6, 5);
  pet(2, 7, (s_tick % 7) == 0, mouth);
  if (family) {
    kid(19, 11, GColorCyan, mouth);
    kid(24, 11, GColorYellow, !mouth);
  }
  // table
  box(GColorWindsorTan, 6, 14, 24, 2);
  box(GColorWindsorTan, 7, 16, 2, 4);
  box(GColorWindsorTan, 27, 16, 2, 4);
  // plates and food
  box(GColorWhite, 11, 13, 5, 1);
  box(GColorChromeYellow, 12, 11, 3, 1);          // bun
  box(GColorWindsorTan, 12, 12, 3, 1);            // patty
  if (family) {
    box(GColorWhite, 19, 13, 4, 1);
    box(GColorRed, 20, 12, 2, 1);
    box(GColorWhite, 24, 13, 4, 1);
    box(GColorGreen, 25, 12, 2, 1);
  }
}

static void scene_play(void) {
  room(GColorMintGreen, GColorRajah);
  // window
  box(GColorWhite, 2, 2, 8, 7);
  box(GColorCeleste, 3, 3, 6, 5);
  // family
  s_bob = 1;
  pet(2, 10, (s_tick % 7) == 0, 0);
  s_bob = 0;
  int hop = (s_tick & 1);
  kid(24, 14 - hop, GColorCyan, true);
  kid(19, 13 + hop, GColorYellow, true);
  // ball arcing between pet and kids
  int t = s_tick % 8;
  int ph = t % 4;
  int bx = 11 + (t < 4 ? t * 2 : (8 - t) * 2);
  int by = 14 - (ph == 2 ? 3 : (ph % 2 ? 2 : 0));
  box(GColorRed, bx, by, 2, 2);
  // hearts drifting up
  int rise = (s_tick / 2) % 3;
  heart_px(GColorShockingPink, 22, 7 - rise);
  if (rise == 2) {
    heart_px(GColorMelon, 14, 4);
  }
}

static void scene_chill(void) {
  room(GColorBlueMoon, GColorImperialPurple);
  // TV with flickering screen
  static const GColor tv[3] = {GColorCyan, GColorMagenta, GColorYellow};
  box(GColorBlack, 19, 7, 11, 8);
  box(tv[s_tick % 3], 20, 8, 9, 6);
  box(GColorDarkGray, 22, 15, 5, 1);
  // sofa with pet
  box(GColorBulgarianRose, 1, 8, 15, 10);
  pet(5, 9, (s_tick % 9) == 0, 0);
  box(GColorDarkCandyAppleRed, 1, 14, 15, 4);
  box(GColorDarkCandyAppleRed, 0, 12, 2, 6);
  box(GColorDarkCandyAppleRed, 15, 12, 2, 6);
}

// ------------------------------------------------------------------ steps

static void update_steps(void) {
#if defined(PBL_HEALTH)
  HealthServiceAccessibilityMask m =
      health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL));
  s_steps = (m & HealthServiceAccessibilityMaskAvailable)
                ? (int)health_service_sum_today(HealthMetricStepCount) : 0;
#else
  s_steps = 0;
#endif
  // Expected pace: linear from 08:00 (0) to 21:00 (full goal).
  int mins = s_hour * 60 + s_min;
  int expected = STEP_GOAL * (mins - 8 * 60) / (13 * 60);
  if (expected < 0) expected = 0;
  if (expected > STEP_GOAL) expected = STEP_GOAL;
  if (s_steps >= STEP_GOAL) {
    s_mood = MOOD_HAPPY;
  } else if (s_hour >= 11 && s_steps < expected / 2) {
    s_mood = MOOD_SAD;                              // way behind pace
  } else {
    s_mood = MOOD_OK;
  }
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    update_steps();
    layer_mark_dirty(s_canvas);
  }
}
#endif

// ------------------------------------------------------------- activities

static Activity pick_activity(int hour, int wday) {
  bool weekend = (wday == 0 || wday == 6);
  if (hour < 7 || hour >= 22) return ACT_SLEEP;
  if (hour < 9) return ACT_WAKE;
  if (hour == 12) return ACT_LUNCH;
  if (hour == 17) return ACT_DINNER;
  if (hour >= 18) return ACT_PLAY;
  // 9-11, 13-16
  if (weekend) return (hour >= 13) ? ACT_CHILL : ACT_PLAY;
  return ACT_CODE;
}

// ---------------------------------------------------------------- drawing

static void heart_raw(GContext *ctx, int x, int y, int u, GColor c) {
  graphics_context_set_fill_color(ctx, c);
  graphics_fill_rect(ctx, GRect(x, y, u, u), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + 2 * u, y, u, u), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x, y + u, 3 * u, u), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(x + u, y + 2 * u, u, u), 0, GCornerNone);
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int w = b.size.w, h = b.size.h;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  // time
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, s_time_buf, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD),
                     GRect(0, h * 20 / 260, w, h * 52 / 260),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // scene frame + scene
  g = ctx;
  U = w / 43;
  if (U < 1) U = 1;
  OX = (w - SCENE_W * U) / 2;
  OY = h * 74 / 260;

  graphics_context_set_fill_color(ctx, GColorShockingPink);
  graphics_fill_rect(ctx, GRect(OX - U, OY - U, (SCENE_W + 2) * U, (SCENE_H + 2) * U),
                     3 * U, GCornersAll);

  switch (pick_activity(s_hour, s_wday)) {
    case ACT_SLEEP:  scene_sleep();       break;
    case ACT_WAKE:   scene_wake();        break;
    case ACT_CODE:   scene_code();        break;
    case ACT_LUNCH:  table_scene(false);  break;
    case ACT_DINNER: table_scene(true);   break;
    case ACT_PLAY:   scene_play();        break;
    case ACT_CHILL:  scene_chill();       break;
  }

  // date
  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, s_date_buf, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(0, h * 208 / 260, w, h * 22 / 260),
                     GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  // step meter: bar fills toward the daily goal, gold once reached
  int bw = w * 90 / 260, bh = h * 5 / 260;
  int bx = (w - bw) / 2, by = h * 231 / 260;
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(bx, by, bw, bh), 0, GCornerNone);
  int fill = s_steps >= STEP_GOAL ? bw : bw * s_steps / STEP_GOAL;
  graphics_context_set_fill_color(ctx, s_steps >= STEP_GOAL ? GColorYellow : GColorGreen);
  graphics_fill_rect(ctx, GRect(bx, by, fill, bh), 0, GCornerNone);

  // battery hearts (each heart = 20%)
  int hu = 3;
  int spacing = 14;
  int x0 = (w - (4 * spacing + 3 * hu)) / 2;
  int y0 = h * 241 / 260;
  int full = (s_batt + 19) / 20;
  for (int i = 0; i < 5; i++) {
    heart_raw(ctx, x0 + i * spacing, y0, hu,
              i < full ? GColorRed : GColorDarkGray);
  }
}

// ------------------------------------------------------------------ events

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hour = t->tm_hour;
  s_min = t->tm_min;
  s_wday = t->tm_wday;
  s_mday = t->tm_mday;

  strftime(s_time_buf, sizeof(s_time_buf), clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  if (!clock_is_24h_style() && s_time_buf[0] == '0') {
    memmove(s_time_buf, s_time_buf + 1, strlen(s_time_buf));
  }
  strftime(s_date_buf, sizeof(s_date_buf), "%a %b %e", t);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_tick++;
  if (units_changed & MINUTE_UNIT) {
    update_time();
    update_steps();
  }
  layer_mark_dirty(s_canvas);
}

static void battery_handler(BatteryChargeState state) {
  s_batt = state.charge_percent;
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

  // One tick per second drives the 1 fps "Tamagotchi" animation.
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
#endif
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
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
