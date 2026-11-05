// Generic host-side renderer for lab faces.
//   gcc -DFACE_SRC='"path/to/face/src/c/main.c"' ... render.c
//   ./render hour min sec wday mday mon year steps batt tick aux focus h24
// Compiles the real face source against the stub Pebble API and dumps a 260x260 frame.
#include "pebble.h"
#define FACE_PREVIEW
#include FACE_SRC

uint8_t fb[SW][SW];
static GColor fillc;
static int g_steps;

void fb_text_log(const char *s, GFont f, GRect r, GColor c, GTextAlignment a) {}
void graphics_context_set_fill_color(GContext *c, GColor col) { fillc = col; }
void graphics_context_set_text_color(GContext *c, GColor col) {}
void graphics_fill_rect(GContext *c, GRect r, uint16_t rad, GCornerMask m) {
  for (int y = r.origin.y; y < r.origin.y + r.size.h; y++)
    for (int x = r.origin.x; x < r.origin.x + r.size.w; x++) {
      if (x < 0 || y < 0 || x >= SW || y >= SW) continue;
      if (m == GCornersAll && rad > 0) {
        int cx = x < r.origin.x + (int)rad ? r.origin.x + rad : (x >= r.origin.x + r.size.w - (int)rad ? r.origin.x + r.size.w - 1 - rad : x);
        int cy = y < r.origin.y + (int)rad ? r.origin.y + rad : (y >= r.origin.y + r.size.h - (int)rad ? r.origin.y + r.size.h - 1 - rad : y);
        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) > (int)(rad * rad)) continue;
      }
      fb[y][x] = fillc.argb;
    }
}
void graphics_draw_text(GContext *c, const char *s, GFont f, GRect r, GTextOverflowMode o, GTextAlignment a, void *l) {}
GFont fonts_get_system_font(const char *k) { return k; }
GRect layer_get_bounds(const Layer *l) { return GRect(0, 0, SW, SW); }
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
void app_focus_service_subscribe(AppFocusHandler h) {}
void app_focus_service_unsubscribe(void) {}
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
  if (argc < 14) return 1;
  s_hour = atoi(argv[1]); s_min = atoi(argv[2]); s_sec = atoi(argv[3]); s_wday = atoi(argv[4]);
  s_mday = atoi(argv[5]); s_mon = atoi(argv[6]); s_year = atoi(argv[7]);
  s_steps = atoi(argv[8]); s_batt = atoi(argv[9]); s_tick = (uint32_t)atoi(argv[10]);
  s_aux = atoi(argv[11]); s_in_focus = atoi(argv[12]) != 0; s_24h = atoi(argv[13]) != 0;
  face_init();
  canvas_update(NULL, NULL);
  fwrite(fb, 1, sizeof fb, stdout);
  return 0;
}
