// Host-side stub of the Pebble API: just enough to run src/c/main.c on a PC
// and rasterize frames into a 260x260 framebuffer (see render.c / preview_lib.py).
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

typedef struct { uint8_t argb; } GColor;
#define GC(r,g,b) ((GColor){(uint8_t)(0xC0 | ((r)<<4) | ((g)<<2) | (b))})
#define GColorArmyGreen GC(1,1,0)
#define GColorBabyBlueEyes GC(2,2,3)
#define GColorBlack GC(0,0,0)
#define GColorBlue GC(0,0,3)
#define GColorBlueMoon GC(0,1,3)
#define GColorBrass GC(2,2,1)
#define GColorBrightGreen GC(1,3,0)
#define GColorBrilliantRose GC(3,1,2)
#define GColorBulgarianRose GC(1,0,0)
#define GColorCadetBlue GC(1,2,2)
#define GColorCeleste GC(2,3,3)
#define GColorChromeYellow GC(3,2,0)
#define GColorCobaltBlue GC(0,1,2)
#define GColorCyan GC(0,3,3)
#define GColorDarkCandyAppleRed GC(2,0,0)
#define GColorDarkGray GC(1,1,1)
#define GColorDarkGreen GC(0,1,0)
#define GColorDukeBlue GC(0,0,2)
#define GColorElectricBlue GC(1,3,3)
#define GColorElectricUltramarine GC(1,0,3)
#define GColorFashionMagenta GC(3,0,2)
#define GColorFolly GC(3,0,1)
#define GColorGreen GC(0,3,0)
#define GColorIcterine GC(3,3,1)
#define GColorImperialPurple GC(1,0,1)
#define GColorInchworm GC(2,3,1)
#define GColorIndigo GC(1,0,2)
#define GColorIslamicGreen GC(0,2,0)
#define GColorJaegerGreen GC(0,2,1)
#define GColorJazzberryJam GC(2,0,1)
#define GColorKellyGreen GC(1,2,0)
#define GColorLavenderIndigo GC(2,1,3)
#define GColorLiberty GC(1,1,2)
#define GColorLightGray GC(2,2,2)
#define GColorLimerick GC(2,2,0)
#define GColorMagenta GC(3,0,3)
#define GColorMalachite GC(0,3,1)
#define GColorMayGreen GC(1,2,1)
#define GColorMediumAquamarine GC(1,3,2)
#define GColorMediumSpringGreen GC(0,3,2)
#define GColorMelon GC(3,2,2)
#define GColorMidnightGreen GC(0,1,1)
#define GColorMintGreen GC(2,3,2)
#define GColorOrange GC(3,1,0)
#define GColorOxfordBlue GC(0,0,1)
#define GColorPastelYellow GC(3,3,2)
#define GColorPictonBlue GC(1,2,3)
#define GColorPurple GC(2,0,2)
#define GColorPurpureus GC(2,1,2)
#define GColorRajah GC(3,2,1)
#define GColorRed GC(3,0,0)
#define GColorRichBrilliantLavender GC(3,2,3)
#define GColorRoseVale GC(2,1,1)
#define GColorScreaminGreen GC(1,3,1)
#define GColorShockingPink GC(3,1,3)
#define GColorSpringBud GC(2,3,0)
#define GColorSunsetOrange GC(3,1,1)
#define GColorTiffanyBlue GC(0,2,2)
#define GColorVeryLightBlue GC(1,1,3)
#define GColorVividCerulean GC(0,2,3)
#define GColorVividViolet GC(2,0,3)
#define GColorWhite GC(3,3,3)
#define GColorWindsorTan GC(2,1,0)
#define GColorYellow GC(3,3,0)

typedef struct { int x, y; } GPoint;
typedef struct { int w, h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;
#define GRect(x,y,w,h) ((GRect){{(x),(y)},{(w),(h)}})
typedef struct GContext GContext;
typedef struct Layer Layer;
typedef struct Window Window;
typedef const char *GFont;
typedef enum { GCornerNone, GCornersAll } GCornerMask;
typedef enum { GTextOverflowModeFill } GTextOverflowMode;
typedef enum { GTextAlignmentLeft, GTextAlignmentCenter } GTextAlignment;
typedef struct { uint8_t charge_percent; } BatteryChargeState;
typedef enum { MINUTE_UNIT = 2, SECOND_UNIT = 1 } TimeUnits;
typedef void (*TickHandler)(struct tm *, TimeUnits);
typedef struct { void (*load)(Window *); void (*unload)(Window *); } WindowHandlers;

#define FONT_KEY_GOTHIC_24_BOLD "GOTHIC_24_BOLD"
#define FONT_KEY_GOTHIC_18_BOLD "GOTHIC_18_BOLD"
#define FONT_KEY_BITHAM_42_BOLD "BITHAM_42_BOLD"
#define PBL_HEALTH 1

// Framebuffer + text log, implemented in render.c.
#ifndef SW
#define SW 260
#endif
extern uint8_t fb[SW][SW];
void fb_text_log(const char *s, GFont f, GRect r, GColor c, GTextAlignment a);

void graphics_context_set_fill_color(GContext *, GColor);
void graphics_context_set_text_color(GContext *, GColor);
void graphics_fill_rect(GContext *, GRect, uint16_t, GCornerMask);
void graphics_draw_text(GContext *, const char *, GFont, GRect, GTextOverflowMode, GTextAlignment, void *);
GFont fonts_get_system_font(const char *);
GRect layer_get_bounds(const Layer *);
Layer *layer_create(GRect);
void layer_destroy(Layer *);
void layer_set_update_proc(Layer *, void (*)(Layer *, GContext *));
void layer_add_child(Layer *, Layer *);
void layer_mark_dirty(Layer *);
Layer *window_get_root_layer(Window *);
Window *window_create(void);
void window_destroy(Window *);
void window_set_background_color(Window *, GColor);
void window_set_window_handlers(Window *, WindowHandlers);
void window_stack_push(Window *, bool);
bool clock_is_24h_style(void);
BatteryChargeState battery_state_service_peek(void);
void tick_timer_service_subscribe(TimeUnits, TickHandler);
void tick_timer_service_unsubscribe(void);
void battery_state_service_subscribe(void (*)(BatteryChargeState));
void battery_state_service_unsubscribe(void);
void app_event_loop(void);

typedef enum { HealthMetricStepCount } HealthMetric;
typedef enum { HealthServiceAccessibilityMaskAvailable = 1 } HealthServiceAccessibilityMask;
typedef enum { HealthEventMovementUpdate, HealthEventSignificantUpdate } HealthEventType;
typedef void (*HealthEventHandler)(HealthEventType, void *);
HealthServiceAccessibilityMask health_service_metric_accessible(HealthMetric, time_t, time_t);
uint32_t health_service_sum_today(HealthMetric);
time_t time_start_of_today(void);
void health_service_events_subscribe(HealthEventHandler, void *);
void health_service_events_unsubscribe(void);

typedef enum { ACCEL_AXIS_X, ACCEL_AXIS_Y, ACCEL_AXIS_Z } AccelAxisType;
typedef void (*AccelTapHandler)(AccelAxisType, int32_t);
void accel_tap_service_subscribe(AccelTapHandler);
void accel_tap_service_unsubscribe(void);
bool persist_exists(uint32_t);
int32_t persist_read_int(uint32_t);
int persist_write_int(uint32_t, int32_t);
typedef void (*AppFocusHandler)(bool);
void app_focus_service_subscribe(AppFocusHandler);
void app_focus_service_unsubscribe(void);
