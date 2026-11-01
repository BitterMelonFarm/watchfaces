// Renders main.c's scenes to raw frames: ./render <hour> <wday> <min> <steps> <batt> <tick> [species] > frame.bin
#include "pebble.h"
#define main app_main
#include "../../src/c/main.c"
#undef main

uint8_t fb[260][260];
static GColor fillc, textc;
static int g_steps;

void fb_text_log(const char *s, GFont f, GRect r, GColor c, GTextAlignment a) {
  fprintf(stderr, "T|%s|%s|%d|%d|%d|%d|%d|%d\n", s, f, r.origin.x, r.origin.y, r.size.w, r.size.h, c.argb, a);
}
void graphics_context_set_fill_color(GContext *c, GColor col) { fillc = col; }
void graphics_context_set_text_color(GContext *c, GColor col) { textc = col; }
void graphics_fill_rect(GContext *c, GRect r, uint16_t rad, GCornerMask m) {
  for (int y = r.origin.y; y < r.origin.y + r.size.h; y++)
    for (int x = r.origin.x; x < r.origin.x + r.size.w; x++)
      if (x >= 0 && y >= 0 && x < 260 && y < 260) fb[y][x] = fillc.argb;
}
void graphics_draw_text(GContext *c, const char *s, GFont f, GRect r, GTextOverflowMode o, GTextAlignment a, void *l) {
  fb_text_log(s, f, r, textc, a);
}
GFont fonts_get_system_font(const char *k) { return k; }
GRect layer_get_bounds(const Layer *l) { return GRect(0, 0, 260, 260); }
Layer *layer_create(GRect r) { return NULL; }
void layer_destroy(Layer *l) {}
void layer_set_update_proc(Layer *l, void (*p)(Layer *, GContext *)) {}
void layer_add_child(Layer *a, Layer *b) {}
void layer_mark_dirty(Layer *l) {}
Layer *window_get_root_layer(Window *w) { return NULL; }
Window *window_create(void) { return NULL; }
void window_destroy(Window *w) {}
void window_set_background_color(Window *w, GColor c) {}
void window_set_window_handlers(Window *w, WindowHandlers h) {}
void window_stack_push(Window *w, bool a) {}
bool clock_is_24h_style(void) { return true; }
BatteryChargeState battery_state_service_peek(void) { return (BatteryChargeState){100}; }
void tick_timer_service_subscribe(TimeUnits u, TickHandler h) {}
void tick_timer_service_unsubscribe(void) {}
void battery_state_service_subscribe(void (*h)(BatteryChargeState)) {}
void battery_state_service_unsubscribe(void) {}
void app_event_loop(void) {}
void accel_tap_service_subscribe(AccelTapHandler h) {}
void accel_tap_service_unsubscribe(void) {}
bool persist_exists(uint32_t k) { return false; }
int32_t persist_read_int(uint32_t k) { return 0; }
int persist_write_int(uint32_t k, int32_t v) { return 4; }
HealthServiceAccessibilityMask health_service_metric_accessible(HealthMetric m, time_t a, time_t b) { return HealthServiceAccessibilityMaskAvailable; }
uint32_t health_service_sum_today(HealthMetric m) { return g_steps; }
time_t time_start_of_today(void) { return 0; }
void health_service_events_subscribe(HealthEventHandler h, void *c) {}
void health_service_events_unsubscribe(void) {}

int main(int argc, char **argv) {
  if (argc < 7) return 1;
  s_hour = atoi(argv[1]); s_wday = atoi(argv[2]); s_min = atoi(argv[3]);
  g_steps = atoi(argv[4]); s_batt = atoi(argv[5]); s_tick = atoi(argv[6]);
  snprintf(s_time_buf, sizeof s_time_buf, "%02d:%02d", s_hour, s_min);
  snprintf(s_date_buf, sizeof s_date_buf, "%s Nov 2", (const char *[]){"Sun","Mon","Tue","Wed","Thu","Fri","Sat"}[s_wday]);
  if (argc > 7) load_species(atoi(argv[7]));
  else load_species(0);
  update_steps();
  canvas_update(NULL, NULL);
  fwrite(fb, 1, sizeof fb, stdout);
  return 0;
}
