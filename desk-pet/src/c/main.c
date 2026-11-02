// Desk Pet - a tiny virtual-pet watchface for Pebble Round 2 (gabbro).
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
  ACT_BRUSH,
  ACT_COMMUTE,
  ACT_MEETING,
  ACT_GAME,
  ACT_STORY,
  ACT_READ,
  ACT_WALK,
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

// The scene is a 64x42 grid of "fine" pixels. Older scene code still thinks in
// 32x21 "coarse" units (2x2 fine pixels each); box() bridges the two.
#define FINE_W (SCENE_W * 2)
#define FINE_H (SCENE_H * 2)
#define FLOOR_FINE (FLOOR_Y * 2)

static GColor s_floor_col;
static bool s_floor_ok;     // room() sets this so pets can cast shadows

// Lighten (d>0) or darken (d<0) a colour by stepping each 2-bit channel.
static GColor shade(GColor c, int d) {
  int r = (c.argb >> 4) & 3, gg = (c.argb >> 2) & 3, b = c.argb & 3;
  r += d; gg += d; b += d;
  if (r < 0) r = 0;
  if (r > 3) r = 3;
  if (gg < 0) gg = 0;
  if (gg > 3) gg = 3;
  if (b < 0) b = 0;
  if (b > 3) b = 3;
  return (GColor){.argb = (uint8_t)(0xC0 | (r << 4) | (gg << 2) | b)};
}

// Fill a rectangle in fine pixels, clipped to the scene.
static void fbox(GColor c, int x, int y, int w, int h) {
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > FINE_W) w = FINE_W - x;
  if (y + h > FINE_H) h = FINE_H - y;
  if (w <= 0 || h <= 0) return;
  graphics_context_set_fill_color(g, c);
  graphics_fill_rect(g, GRect(OX + x * U, OY + y * U, w * U, h * U), 0, GCornerNone);
}

// Fill a rectangle in coarse units. Props get a light top edge and dark bottom
// edge so flat boxes read as chunky 3D objects; big backdrops stay flat.
static void box(GColor c, int x, int y, int w, int h) {
  fbox(c, x * 2, y * 2, w * 2, h * 2);
  if (w >= 2 && h >= 2 && w < 30) {
    fbox(shade(c, 1), x * 2, y * 2, w * 2, 1);
    fbox(shade(c, -1), x * 2, y * 2 + h * 2 - 1, w * 2, 1);
    fbox(shade(c, -1), x * 2 + w * 2 - 1, y * 2 + 1, 1, h * 2 - 2);
  }
}

// 5x4 fine-pixel heart.
static void fheart(GColor c, int x, int y) {
  fbox(c, x, y, 2, 1);  fbox(c, x + 3, y, 2, 1);
  fbox(c, x, y + 1, 5, 1);
  fbox(c, x + 1, y + 2, 3, 1);
  fbox(c, x + 2, y + 3, 1, 1);
  fbox(GColorWhite, x + 1, y, 1, 1);
}

static void txt(const char *s, const char *font_key, GColor c, int x, int y, int w, int h) {
  graphics_context_set_text_color(g, c);
  graphics_draw_text(g, s, fonts_get_system_font(font_key),
                     GRect(OX + x * 2 * U, OY + y * 2 * U, w * 2 * U, h * 2 * U),
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

static void heart_px(GColor c, int x, int y) {
  box(c, x, y, 1, 1);
  box(c, x + 2, y, 1, 1);
  box(c, x, y + 1, 3, 1);
  box(c, x + 1, y + 2, 1, 1);
}

// ------------------------------------------------------------- pet species
// Flick your wrist (tap) to cycle through the pets; the choice is saved.

typedef enum {
  EARS_NUB, EARS_CAT, EARS_BUNNY, EARS_BEAR, EARS_FROG,
  EARS_CTHULHU, EARS_SHOGGOTH, EARS_DEEP, EARS_EYE,
} EarStyle;
#define NUM_SPECIES 9
#define PERSIST_SPECIES 1

typedef struct {
  GColor body, out, blush, snout;
  EarStyle ears;
} Species;

static Species s_sp;
static int s_species;

static void load_species(int id) {
  switch (id) {
    default:
    case 0:  // blob: pink blob
      s_sp = (Species){GColorMelon, GColorShockingPink, GColorSunsetOrange, GColorMelon, EARS_NUB};
      break;
    case 1:  // cat: orange tabby
      s_sp = (Species){GColorRajah, GColorOrange, GColorShockingPink, GColorRajah, EARS_CAT};
      break;
    case 2:  // bunny: white with long ears
      s_sp = (Species){GColorWhite, GColorLightGray, GColorShockingPink, GColorWhite, EARS_BUNNY};
      break;
    case 3:  // bear: brown with a creamy snout
      s_sp = (Species){GColorWindsorTan, GColorBulgarianRose, GColorMelon, GColorRajah, EARS_BEAR};
      break;
    case 4:  // frog: green with eye bumps and a wide grin
      s_sp = (Species){GColorGreen, GColorIslamicGreen, GColorShockingPink, GColorGreen, EARS_FROG};
      break;
    case 5:  // baby Cthulhu: bat wings (snout colour) and a wiggly tentacle beard
      s_sp = (Species){GColorMayGreen, GColorIslamicGreen, GColorMelon, GColorIndigo, EARS_CTHULHU};
      break;
    case 6:  // shoggoth: purple goo with too many eyes
      s_sp = (Species){GColorPurple, GColorIndigo, GColorMagenta, GColorPurple, EARS_SHOGGOTH};
      break;
    case 7:  // deep one: teal fish-person with a crest, gills and tail fin
      s_sp = (Species){GColorTiffanyBlue, GColorCobaltBlue, GColorShockingPink, GColorTiffanyBlue, EARS_DEEP};
      break;
    case 8:  // great eye: a floating eyeball on tentacles
      s_sp = (Species){GColorWhite, GColorIndigo, GColorRed, GColorWhite, EARS_EYE};
      break;
  }
}

#define PET_BODY (s_sp.body)
#define PET_OUT  (s_sp.out)

static int s_bob;   // 1 = pet is standing and hops on odd frames

static void px(int x, int y, GColor c) { box(c, x, y, 1, 1); }

// Rounded body silhouette of the 16x16 pet: first/last column of each row.
static const int8_t BODY_A[15] = {4, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};
static const int8_t BODY_B[15] = {11, 13, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12};

static uint32_t s_react_start, s_react_until;   // tap-to-pet reaction window (in ticks)
static bool s_sleeping;   // draw only the head, eyes shut (used by the bed scene)

// Fill fine pixels, but never below row ymax (lets the bed hide the body).
static void pf(GColor c, int x, int y, int w, int h, int ymax) {
  if (y >= ymax) return;
  if (y + h > ymax) h = ymax - y;
  fbox(c, x, y, w, h);
}

static int imax(int a, int b) { return a > b ? a : b; }
static int imin(int a, int b) { return a < b ? a : b; }

// The grown-up pet: a 16x16 fine-pixel sprite (8x8 coarse) plus ears/extras.
// mouth: 0 = closed, 1 = open (chewing / talking)
static void pet(int x, int y, bool blink, int mouth) {
  const bool sl = s_sleeping;
  const bool reacting = !sl && s_react_until > s_tick;
  const Mood md = sl ? MOOD_OK : (reacting ? MOOD_HAPPY : s_mood);
  if (sl) { blink = true; mouth = 0; }
  if (!sl && (s_bob || reacting) && (s_tick & 1)) y -= 1;        // little hop
  if (!sl && s_sp.ears == EARS_EYE && (s_tick & 2)) y -= 1;   // the eye bobs in the air
  const int X = x * 2, Y = y * 2;
  const int maxrow = sl ? 10 : 99;
#define F(c, dx, dy, w, h) pf((c), X + (dx), Y + (dy), (w), (h), Y + maxrow)
  const EarStyle k = s_sp.ears;
  const GColor out = s_sp.out, body = s_sp.body;
  const GColor hl = shade(body, 1), sh = shade(body, -1);
  const int t = (int)(s_tick & 1);

  // soft contact shadow on the floor
  int gap = FLOOR_FINE - (Y + 16);
  if (!sl && s_floor_ok && gap >= 0 && gap <= 2) {
    fbox(shade(s_floor_col, -1), X + 3 + gap, FLOOR_FINE + 1, 10 - 2 * gap, 1);
  }

  // ears / wings / crests, drawn first so the head overlaps them
  switch (k) {
    case EARS_CAT:
      F(out, 2, -2, 4, 3);  F(out, 2, -4, 2, 2);  F(s_sp.blush, 3, -1, 2, 1);
      F(out, 10, -2, 4, 3); F(out, 12, -4, 2, 2); F(s_sp.blush, 11, -1, 2, 1);
      break;
    case EARS_BUNNY:
      F(out, 3, -8, 4, 9);  F(s_sp.blush, 4, -7, 2, 6);
      F(out, 9, -8 + (int)((s_tick % 9) == 0), 4, 9 - (int)((s_tick % 9) == 0));
      F(s_sp.blush, 10, -7 + (int)((s_tick % 9) == 0), 2, 6);
      break;
    case EARS_BEAR:
      F(out, 1, -3, 4, 5);  F(s_sp.snout, 2, -2, 2, 2);
      F(out, 11, -3, 4, 5); F(s_sp.snout, 12, -2, 2, 2);
      break;
    case EARS_FROG:
      F(out, 2, -3, 5, 5);  F(out, 9, -3, 5, 5);
      break;
    case EARS_CTHULHU:
      if (X >= 5) {                                  // spread, flapping bat wings
        F(s_sp.snout, -4, 3 + t, 4, 2);  F(s_sp.snout, -5, 5 + t, 5, 3);  F(s_sp.snout, -3, 8 + t, 3, 2);
        F(shade(s_sp.snout, 1), -4, 5 + t, 2, 2);
      } else {                                       // folded against the wall
        F(s_sp.snout, -2, 3 + t, 2, 7);  F(shade(s_sp.snout, 1), -2, 5 + t, 1, 2);
      }
      if (X + 21 <= FINE_W) {
        F(s_sp.snout, 16, 3 + t, 4, 2);  F(s_sp.snout, 16, 5 + t, 5, 3);  F(s_sp.snout, 16, 8 + t, 3, 2);
        F(shade(s_sp.snout, 1), 18, 5 + t, 2, 2);
      } else {
        F(s_sp.snout, 16, 3 + t, 2, 7);  F(shade(s_sp.snout, 1), 17, 5 + t, 1, 2);
      }
      F(out, 3, -2, 2, 3);  F(out, 11, -2, 2, 3);   // tiny horns
      break;
    case EARS_SHOGGOTH:
      F(out, 3, -1, 3, 2);
      F(out, 7, -3 + t, 3, 4);  F(shade(out, 1), 8, -2 + t, 1, 1);
      F(out, 11, -1, 2, 2);
      break;
    case EARS_DEEP:
      F(out, 4, -3, 2, 4);  F(out, 6, -5, 4, 6);  F(out, 10, -3, 2, 4);
      F(s_sp.blush, 7, -4, 2, 2);
      break;
    case EARS_EYE:
      break;
    default:
      F(out, 2, -2, 3, 3);  F(out, 11, -2, 3, 3);
      F(hl, 3, -1, 1, 1);   F(hl, 12, -1, 1, 1);
      break;
  }

  // body: outline, then inner fill, then shading
  for (int r = 0; r < 15; r++) F(out, BODY_A[r], r, BODY_B[r] - BODY_A[r] + 1, 1);
  for (int r = 1; r < 14; r++) {
    int ia = imax(BODY_A[r], imax(BODY_A[r - 1], BODY_A[r + 1])) + 1;
    int ib = imin(BODY_B[r], imin(BODY_B[r - 1], BODY_B[r + 1])) - 1;
    F((r >= 12) ? sh : body, ia, r, ib - ia + 1, 1);
  }
  F(hl, 4, 2, 4, 1);  F(hl, 2, 3, 2, 1);  F(hl, 2, 4, 1, 2);       // glossy highlight
  F(sh, 12, 6, 2, 5);                                               // shadow side

  // feet (the eldritch ones get tentacles/drips instead)
  if (k < EARS_CTHULHU || k == EARS_DEEP) {
    F(out, 2, 15, 4, 1);  F(out, 10, 15, 4, 1);
    F(hl, 3, 14, 1, 1);   F(hl, 11, 14, 1, 1);
  } else if (k == EARS_SHOGGOTH) {
    for (int i = 2; i <= 12; i += 3) {
      if (((int)s_tick + i / 3) % 3 != 0) F(out, i, 15, 2, 1 + ((i / 3 + (int)s_tick) % 2));
    }
  } else if (k == EARS_EYE) {
    static const int tx[4] = {2, 5, 8, 11};
    for (int i = 0; i < 4; i++) {
      F(out, tx[i], 15, 2, 1);
      if (((int)s_tick + i) & 1) F(out, tx[i] + 1, 16, 1, 1);
    }
  }
  // little arms
  if (!sl && k != EARS_EYE && k != EARS_CTHULHU) {
    F(out, -1, 9, 1, 3);  F(out, 16, 9, 1, 3);
  }

  // species extras
  if (k == EARS_CAT) {
    F(GColorDarkGray, -3, 8, 3, 1);  F(GColorDarkGray, -3, 11, 3, 1);        // whiskers
    F(GColorDarkGray, 16, 8, 3, 1);  F(GColorDarkGray, 16, 11, 3, 1);
    F(out, 16, 12, 2, 4);  F(out, 18, 10 + 2 * t, 2, 3);             // wagging tail
    F(out, 7, 2, 1, 2);  F(out, 9, 2, 1, 2);  F(out, 5, 3, 1, 1);  F(out, 11, 3, 1, 1);  // stripes
    F(GColorWhite, 6, 12, 4, 2);                                      // white chin
  } else if (k == EARS_BUNNY) {
    F(GColorWhite, -3, 11, 3, 3);  F(GColorLightGray, -3, 13, 3, 1);  // cotton tail
  } else if (k == EARS_BEAR) {
    F(s_sp.snout, 4, 9, 8, 5);  F(shade(s_sp.snout, 1), 5, 9, 3, 1);
    F(GColorBlack, 7, 9, 2, 1);
  } else if (k == EARS_SHOGGOTH) {
    // extra eyes blinking all over the goo
    if (((int)s_tick + 0) % 4 != 0) { F(GColorYellow, 6, 1, 2, 2);  F(GColorBlack, 7, 2, 1, 1); }
    if (((int)s_tick + 1) % 4 != 0) { F(GColorGreen, 12, 2, 2, 2);  F(GColorBlack, 12, 3, 1, 1); }
    if (((int)s_tick + 2) % 4 != 0) { F(GColorYellow, 0, 5, 2, 2);  F(GColorBlack, 1, 6, 1, 1); }
    if (((int)s_tick + 3) % 4 != 0) { F(GColorGreen, 14, 9, 2, 2);  F(GColorBlack, 14, 10, 1, 1); }
    if (((int)s_tick + 1) % 5 != 0) { F(GColorYellow, 2, 12, 2, 2);  F(GColorBlack, 3, 13, 1, 1); }
  } else if (k == EARS_DEEP) {
    F(out, 16, 11, 2, 3);  F(out, 18, 9, 2, 3);  F(out, 18, 13, 2, 3);  F(shade(out, 1), 17, 12, 1, 1);
    F(out, 1, 9, 3, 1);  F(out, 1, 11, 3, 1);  F(out, 12, 9, 3, 1);  F(out, 12, 11, 3, 1);   // gills
  }

  // eyes
  if (k == EARS_EYE) {
    // one giant eyeball: iris, slit pupil, shine; blinks with an eyelid
    if (blink) {
      F(body, 3, 4, 10, 6);
      F(GColorBlack, 3, 9, 10, 1);  F(GColorBlack, 3, 10, 1, 1);  F(GColorBlack, 12, 10, 1, 1);
    } else {
      F(GColorMagenta, 4, 4, 8, 8);  F(shade(GColorMagenta, -1), 4, 10, 8, 2);
      F(GColorBlack, 6, 4, 4, 8);
      F(GColorWhite, 5, 5, 2, 2);
      F(GColorWhite, 8 + (int)((s_tick / 3) % 2), 8, 1, 1);
    }
    F(GColorRed, 2, 4, 1, 2);  F(GColorRed, 1, 5, 2, 1);   // bloodshot veins
    F(GColorRed, 13, 9, 1, 3);  F(GColorMelon, 3, 11, 2, 1);
  } else if (k == EARS_FROG) {
    // eyes sit up on the bumps
    F(GColorWhite, 3, -2, 3, 4);  F(GColorWhite, 10, -2, 3, 4);
    if (blink) {
      F(out, 3, -1, 3, 2);  F(out, 10, -1, 3, 2);
    } else {
      F(GColorBlack, 4, -1, 2, 3);  F(GColorBlack, 11, -1, 2, 3);
      F(GColorWhite, 4, -1, 1, 1);  F(GColorWhite, 11, -1, 1, 1);
    }
  } else if (k == EARS_DEEP && !blink && md != MOOD_HAPPY) {
    // big bulging fish eyes looking to the side
    F(GColorWhite, 2, 4, 5, 5);  F(GColorWhite, 9, 4, 5, 5);
    F(out, 3, 4, 3, 1);  F(out, 10, 4, 3, 1);
    F(GColorBlack, 5, 6, 2, 2);  F(GColorBlack, 12, 6, 2, 2);
    F(GColorWhite, 5, 6, 1, 1);  F(GColorWhite, 12, 6, 1, 1);
  } else if (blink) {
    F(GColorBlack, 3, 7, 3, 1);  F(GColorBlack, 10, 7, 3, 1);
    F(out, 2, 7, 1, 1);  F(out, 13, 7, 1, 1);
  } else if (md == MOOD_HAPPY) {
    F(GColorBlack, 3, 8, 1, 1);  F(GColorBlack, 4, 6, 2, 2);  F(GColorBlack, 6, 8, 1, 1);   // ^ ^
    F(GColorBlack, 9, 8, 1, 1);  F(GColorBlack, 10, 6, 2, 2);  F(GColorBlack, 12, 8, 1, 1);
  } else {
    F(GColorBlack, 3, 5, 3, 4);  F(GColorBlack, 10, 5, 3, 4);
    F(GColorWhite, 3, 5, 2, 1);  F(GColorWhite, 10, 5, 2, 1);        // big shine
    F(GColorWhite, 5, 8, 1, 1);  F(GColorWhite, 12, 8, 1, 1);        // little shine
  }

  // blush
  if (k != EARS_EYE && k != EARS_DEEP && k != EARS_BEAR && k != EARS_FROG) {
    F(s_sp.blush, 1, 10, 3, 1);   F(s_sp.blush, 12, 10, 3, 1);
    F(shade(s_sp.blush, 1), 2, 9, 1, 1);  F(shade(s_sp.blush, 1), 13, 9, 1, 1);
  } else if (k == EARS_FROG) {
    F(s_sp.blush, 2, 10, 2, 1);  F(s_sp.blush, 12, 10, 2, 1);
  }

  // mouth
  if (k == EARS_EYE) {
    // an eyeball has no mouth
  } else if (mouth) {
    F(GColorRed, 6, 10, 4, 3);  F(GColorShockingPink, 7, 12, 2, 1);  F(GColorBlack, 6, 10, 4, 1);
  } else if (md == MOOD_SAD) {
    F(GColorBlack, 6, 10, 4, 1);  F(GColorBlack, 5, 11, 1, 1);  F(GColorBlack, 10, 11, 1, 1);
  } else if (md == MOOD_HAPPY) {
    F(GColorBlack, 5, 10, 1, 1);  F(GColorBlack, 10, 10, 1, 1);  F(GColorBlack, 6, 11, 4, 1);
    F(GColorRed, 7, 11, 2, 1);
  } else if (k == EARS_FROG) {
    F(GColorBlack, 3, 11, 10, 1);  F(GColorBlack, 2, 10, 1, 1);  F(GColorBlack, 13, 10, 1, 1);
  } else if (k == EARS_SHOGGOTH) {
    F(GColorBlack, 3, 10, 10, 3);                               // toothy grin
    F(GColorWhite, 4, 10, 1, 1);  F(GColorWhite, 7, 10, 2, 1);  F(GColorWhite, 11, 10, 1, 1);
    F(GColorWhite, 5, 12, 1, 1);  F(GColorWhite, 10, 12, 1, 1);
  } else if (k == EARS_DEEP) {
    F(GColorShockingPink, 5, 11, 6, 2);  F(shade(GColorShockingPink, 1), 6, 11, 2, 1);
  } else {
    F(GColorBlack, 7, 10, 2, 1);                                // tiny "w" mouth
    F(GColorBlack, 6, 9, 1, 1);  F(GColorBlack, 9, 9, 1, 1);
    if (k == EARS_BUNNY) F(GColorWhite, 7, 11, 2, 1);
  }

  if (k == EARS_CTHULHU) {
    // wiggling tentacle beard that also serves as feet
    F(out, 3, 12, 2, 3 + t);   F(out, 5, 12, 2, 4 - t);
    F(out, 7, 12, 2, 3 + t);   F(out, 9, 12, 2, 4 - t);   F(out, 11, 12, 2, 3 + t);
    F(shade(out, 1), 3, 13, 1, 1);  F(shade(out, 1), 7, 13, 1, 1);  F(shade(out, 1), 11, 13, 1, 1);
  }

  if (md == MOOD_SAD) {
    F(GColorPictonBlue, 4, 9, 1, 2 + t);                  // tear
    F(GColorWhite, 4, 9, 1, 1);
    // little rain cloud overhead
    int cy = (k == EARS_BUNNY) ? -16 : -10;
    F(GColorLightGray, 3, cy + 2, 10, 3);  F(GColorLightGray, 5, cy, 6, 2);
    F(GColorWhite, 4, cy + 2, 3, 1);
    F(GColorPictonBlue, 5 + 4 * t, cy + 5, 1, 2);
    F(GColorPictonBlue, 10 - 4 * t, cy + 5, 1, 2);
  } else if (md == MOOD_HAPPY) {
    // twinkling sparkles
    int sx = t ? 16 : -3;
    int sy = (k == EARS_BUNNY) ? -12 : -9;
    F(GColorYellow, sx + 1, sy, 1, 5);  F(GColorYellow, sx - 1, sy + 2, 5, 1);
    F(GColorWhite, sx + 1, sy + 2, 1, 1);
    F(GColorYellow, sx + 6 - 2 * t, sy + 6, 1, 3);  F(GColorYellow, sx + 5 - 2 * t, sy + 7, 3, 1);
  }
  if (reacting || (sl && s_react_until > s_tick)) {
    // hearts float up from the pet after a tap
    int el = (int)(s_tick - s_react_start);
    for (int i = 0; i < 3; i++) {
      int hy = -4 - ((el * 3 + i * 5) % 14);
      fheart(i == 1 ? GColorRed : GColorShockingPink, X + 1 + i * 5, Y + hy);
    }
  }
#undef F
}

// A smaller kid blob: 10x8 fine pixels with a tuft of hair and shading.
static bool s_kid_sleep;   // kids draw with closed eyes when set

static void kid(int x, int y, GColor c, bool happy) {
  const int X = x * 2, Y = y * 2;
#define K(col, dx, dy, w, h) fbox((col), X + (dx), Y + (dy), (w), (h))
  static const int8_t A[8] = {2, 1, 0, 0, 0, 0, 1, 2};
  static const int8_t B[8] = {7, 8, 9, 9, 9, 9, 8, 7};
  GColor o = shade(c, -1);
  // tuft
  K(GColorRed, 4, -3, 2, 3);  K(GColorRed, 6, -2, 1, 1);  K(shade(GColorRed, 1), 4, -3, 1, 1);
  for (int r = 0; r < 8; r++) K(o, A[r], r, B[r] - A[r] + 1, 1);
  for (int r = 1; r < 7; r++) K(c, A[r] + 1, r, B[r] - A[r] - 1, 1);
  K(shade(c, 1), 2, 1, 3, 1);  K(shade(c, 1), 1, 2, 1, 2);
  // eyes with shine
  if (s_kid_sleep) {
    K(GColorBlack, 2, 4, 2, 1);  K(GColorBlack, 6, 4, 2, 1);        // sleepy lines
  } else {
    K(GColorBlack, 2, 3, 2, 2);  K(GColorBlack, 6, 3, 2, 2);
    K(GColorWhite, 2, 3, 1, 1);  K(GColorWhite, 6, 3, 1, 1);
  }
  // blush + mouth
  K(GColorMelon, 1, 5, 1, 1);  K(GColorMelon, 8, 5, 1, 1);
  if (s_kid_sleep) {
    K(GColorBlack, 4, 6, 2, 1);
  } else if (happy) {
    K(GColorRed, 4, 5, 2, 2);  K(GColorBlack, 4, 5, 2, 1);
  } else {
    K(GColorBlack, 4, 5, 2, 1);
  }
  // feet
  K(o, 2, 7, 2, 1);  K(o, 6, 7, 2, 1);
#undef K
}

// A small cloud drifting across a window (x0..x0+w-1 at row y0).
static void cloud(int x0, int y0, int w) {
  int cx = x0 + (int)((s_tick / 3) % (uint32_t)(w - 1));
  box(GColorWhite, cx, y0, 2, 1);
  px(cx + 1, y0 - 1, GColorWhite);
}

// Daytime window, drawn in two passes so a sun/cloud can sit behind the bars:
// window_glass() = frame + sky + hills, window_frame() = bars, sill, curtains.
static void window_glass(int x, int y, int w, int h) {
  int X = x * 2, Y = y * 2, W = w * 2, H = h * 2;
  fbox(GColorWhite, X, Y, W, H);
  fbox(GColorCeleste, X + 2, Y + 2, W - 4, H - 4);
  fbox(shade(GColorCeleste, -1), X + 2, Y + H - 7, W - 4, 5);
  fbox(GColorMayGreen, X + 2, Y + H - 5, W - 4, 3);                 // far hills
  fbox(GColorMayGreen, X + 4, Y + H - 7, 6, 2);
  fbox(GColorIslamicGreen, X + 2, Y + H - 3, W - 4, 1);
}

static void window_frame(int x, int y, int w, int h) {
  int X = x * 2, Y = y * 2, W = w * 2, H = h * 2;
  fbox(GColorWhite, X + W / 2 - 1, Y, 2, H);                        // bars
  fbox(GColorWhite, X, Y + H / 2 - 1, W, 2);
  fbox(GColorLightGray, X - 2, Y + H, W + 4, 2);                    // sill
  fbox(GColorDarkGray, X - 3, Y - 3, W + 6, 1);                     // curtain rod
  fbox(GColorMelon, X - 2, Y - 2, 4, H + 2);                        // curtains
  fbox(GColorMelon, X + W - 2, Y - 2, 4, H + 2);
  fbox(shade(GColorMelon, -1), X - 1, Y, 1, H);
  fbox(shade(GColorMelon, -1), X + W, Y, 1, H);
}

// Little wall clock whose hand shows the quarter hour.
static void wall_clock(int x, int y) {
  fbox(GColorDarkGray, x, y + 1, 6, 4);  fbox(GColorDarkGray, x + 1, y, 4, 6);
  fbox(GColorWhite, x + 1, y + 1, 4, 4);
  fbox(GColorBlack, x + 2, y + 2, 1, 1);
  static const int mx[4] = {2, 3, 2, 1};
  static const int my[4] = {1, 2, 3, 2};
  int q = (s_min / 15) % 4;
  fbox(GColorBlack, x + mx[q], y + my[q], 1, 1);
  fbox(GColorRed, x + 3, y + 4, 1, 1);
}

// Night window: dark sky with stars and a moon, then the shared curtains.
static void night_window(int x, int y, int w, int h) {
  int X = x * 2, Y = y * 2, W = w * 2, H = h * 2;
  fbox(GColorWhite, X, Y, W, H);
  fbox(GColorOxfordBlue, X + 2, Y + 2, W - 4, H - 4);
  fbox(shade(GColorOxfordBlue, 1), X + 2, Y + H - 6, W - 4, 4);
  fbox(GColorPastelYellow, X + W - 7, Y + 4, 3, 4);  fbox(GColorPastelYellow, X + W - 8, Y + 5, 5, 2);
  fbox(GColorOxfordBlue, X + W - 6, Y + 3, 3, 3);
  if ((s_tick + 1) % 3) fbox(GColorWhite, X + 4, Y + 5, 1, 1);
  if ((s_tick + 2) % 3) fbox(GColorWhite, X + 7, Y + 9, 1, 1);
  if (s_tick % 3) fbox(GColorWhite, X + W - 10, Y + 10, 1, 1);
  window_frame(x, y, w, h);
}

static void room(GColor wall, GColor floor_c) {
  // wallpaper with soft vertical stripes and a baseboard
  int bright = ((wall.argb >> 4) & 3) + ((wall.argb >> 2) & 3) + (wall.argb & 3);
  GColor stripe = shade(wall, bright >= 5 ? -1 : 1);
  fbox(wall, 0, 0, FINE_W, FLOOR_FINE);
  for (int x = 2; x < FINE_W; x += 8) fbox(stripe, x, 0, 2, FLOOR_FINE - 2);
  fbox(shade(wall, bright >= 5 ? -1 : 1), 0, FLOOR_FINE - 2, FINE_W, 2);
  fbox(shade(wall, bright >= 5 ? -2 : 2), 0, FLOOR_FINE - 1, FINE_W, 1);
  // wooden floor: planks with staggered joints
  fbox(floor_c, 0, FLOOR_FINE, FINE_W, FINE_H - FLOOR_FINE);
  fbox(shade(floor_c, 1), 0, FLOOR_FINE, FINE_W, 1);
  GColor line = shade(floor_c, -1);
  for (int row = 0; row < 3; row++) {
    int yy = FLOOR_FINE + 2 + row * 3;
    if (yy >= FINE_H) break;
    fbox(line, 0, yy, FINE_W, 1);
    for (int x = (row * 7) % 13; x < FINE_W; x += 13) fbox(line, x, yy - 2, 1, 2);
  }
  s_floor_col = floor_c;
  s_floor_ok = true;
}

// ----------------------------------------------------------------- scenes

static void scene_sleep(void) {
  GColor wall = GColorOxfordBlue;
  room(wall, GColorImperialPurple);
  const int t = (int)(s_tick & 1);
  // crescent moon with craters
  fbox(GColorPastelYellow, 49, 6, 6, 8);  fbox(GColorPastelYellow, 48, 7, 8, 6);
  fbox(wall, 52, 5, 6, 7);
  fbox(shade(GColorPastelYellow, -1), 49, 9, 1, 1);  fbox(shade(GColorPastelYellow, -1), 50, 12, 1, 1);
  // twinkling stars (some sparkle into a cross)
  static const int sx[5] = {8, 22, 34, 42, 58};
  static const int sy[5] = {6, 12, 4, 16, 12};
  for (int i = 0; i < 5; i++) {
    int ph = (int)((s_tick + (unsigned)i) % 4);
    if (ph == 0) continue;
    fbox(GColorWhite, sx[i], sy[i], 1, 1);
    if (ph == 2) { fbox(GColorWhite, sx[i] - 1, sy[i], 3, 1);  fbox(GColorWhite, sx[i], sy[i] - 1, 1, 3); }
  }
  // bed: posts, base and mattress
  fbox(GColorWindsorTan, 9, 22, 3, 18);  fbox(GColorRajah, 9, 21, 3, 1);
  fbox(GColorWindsorTan, 52, 28, 3, 12);  fbox(GColorRajah, 52, 27, 3, 1);
  fbox(GColorWindsorTan, 9, 36, 46, 3);  fbox(shade(GColorWindsorTan, 1), 9, 36, 46, 1);
  fbox(shade(GColorWindsorTan, -1), 9, 39, 46, 1);
  // pillow and the sleeping pet's head
  fbox(GColorWhite, 12, 25, 18, 5);  fbox(GColorLightGray, 12, 29, 18, 1);
  s_sleeping = true;
  pet(6, 8, true, 0);
  s_sleeping = false;
  if (s_tick & 2) fbox(GColorCeleste, 29, 22, 3, 3);   // sleepy snot bubble
  else fbox(GColorCeleste, 29, 23, 2, 2);
  // blanket with stars and a hem
  fbox(GColorCobaltBlue, 24, 26, 28, 10);
  fbox(GColorBlueMoon, 24, 26, 28, 2);
  fbox(shade(GColorCobaltBlue, -1), 24, 35, 28, 1);
  for (int x = 30; x < 50; x += 7) {
    fbox(GColorPastelYellow, x, 30, 1, 3);  fbox(GColorPastelYellow, x - 1, 31, 3, 1);
  }
  // plush teddy keeping watch at the foot of the bed
  fbox(GColorRajah, 46, 22, 5, 4);  fbox(GColorRajah, 45, 21, 2, 2);  fbox(GColorRajah, 49, 21, 2, 2);
  fbox(GColorBlack, 47, 23, 1, 1);  fbox(GColorBlack, 49, 23, 1, 1);
  fbox(GColorWindsorTan, 47, 24, 3, 1);
  (void)t;
  // floating z's
  static const char *zs[3] = {"z", "zz", "zzz"};
  txt(zs[(s_tick / 2) % 3], FONT_KEY_GOTHIC_24_BOLD, GColorWhite, 15, 3, 10, 8);
}

static void scene_wake(void) {
  room(GColorPastelYellow, GColorRajah);
  // window with rising sun
  window_glass(2, 2, 9, 9);
  box(GColorYellow, 7, 4, 3, 3);
  cloud(3, 4, 4);
  window_frame(2, 2, 9, 9);
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
  const int t = (int)(s_tick & 1);
  // wall clock that really shows the quarter hour, with a red tick
  fbox(GColorDarkGray, 35, 2, 6, 4);  fbox(GColorDarkGray, 36, 1, 4, 6);
  fbox(GColorWhite, 36, 2, 4, 4);
  fbox(GColorBlack, 37, 3, 1, 1);
  static const int mx[4] = {37, 38, 37, 36};
  static const int my[4] = {2, 3, 4, 3};
  int q = (s_min / 15) % 4;
  fbox(GColorBlack, mx[q], my[q], 1, 1);
  fbox(GColorRed, 38, 5, 1, 1);
  // landscape poster
  fbox(GColorWindsorTan, 53, 2, 10, 12);
  fbox(GColorWhite, 54, 3, 8, 10);
  fbox(GColorCeleste, 55, 4, 6, 4);
  fbox(GColorGreen, 55, 8, 6, 4);
  fbox(GColorYellow, 58, 5, 2, 2);
  fbox(GColorIslamicGreen, 55, 10, 3, 2);
  // plant with leaves
  fbox(GColorIslamicGreen, 61, 24, 1, 6);
  fbox(GColorGreen, 59, 25, 2, 2);  fbox(GColorGreen, 62, 24, 2, 2);
  fbox(GColorIslamicGreen, 59, 22, 2, 2);  fbox(GColorIslamicGreen, 62, 27, 2, 2);
  fbox(GColorWindsorTan, 60, 30, 4, 6);  fbox(shade(GColorWindsorTan, 1), 60, 30, 4, 1);
  // chair: back, seat, stem and wheels
  fbox(GColorBulgarianRose, 1, 16, 4, 16);  fbox(shade(GColorBulgarianRose, 1), 1, 16, 4, 1);
  fbox(GColorBulgarianRose, 3, 30, 17, 3);  fbox(shade(GColorBulgarianRose, 1), 3, 30, 17, 1);
  fbox(GColorDarkGray, 10, 33, 2, 3);
  fbox(GColorDarkGray, 5, 36, 12, 1);
  fbox(GColorBlack, 5, 37, 2, 1);  fbox(GColorBlack, 15, 37, 2, 1);
  // pet sitting at the desk
  pet(2, 7, (s_tick % 7) == 0, 0);
  // monitor: bezel, screen with syntax-coloured code, status LED, stand
  fbox(GColorDarkGray, 24, 8, 26, 18);  fbox(shade(GColorDarkGray, 1), 24, 8, 26, 1);
  fbox(GColorBlack, 26, 10, 22, 13);
  static const GColor c1[3] = {GColorGreen, GColorCyan, GColorYellow};
  static const GColor c2[3] = {GColorMagenta, GColorWhite, GColorLightGray};
  for (int i = 0; i < 5; i++) {
    int x = 27 + (int)(((unsigned)i + s_tick / 6) % 3) * 2;
    int w1 = 2 + (int)((s_tick / 2 + (unsigned)i) % 3);
    fbox(c1[i % 3], x, 11 + i * 2, w1, 1);
    fbox(c2[(i + 1) % 3], x + w1 + 1, 11 + i * 2, 2 + (int)((s_tick / 3 + (unsigned)i * 2) % 4), 1);
  }
  if (t) fbox(GColorWhite, 27, 21, 2, 1);                           // blinking cursor
  fbox(GColorGreen, 48, 24, 1, 1);                                  // power LED
  fbox(GColorDarkGray, 36, 26, 4, 1);  fbox(GColorDarkGray, 34, 27, 8, 1);
  // keyboard, mouse and the pet's typing arms
  fbox(GColorLightGray, 24, 26, 12, 2);
  for (int kx = 25; kx < 35; kx += 2) fbox(GColorWhite, kx, 26, 1, 1);
  fbox(GColorDarkGray, 25, 27, 10, 1);
  fbox(GColorWhite, 42, 27, 3, 1);  fbox(GColorLightGray, 42, 27, 1, 1);
  fbox(PET_OUT, 19, 25 + t, 7, 2);
  // desk with a drawer
  fbox(GColorWindsorTan, 20, 28, 40, 3);
  fbox(shade(GColorWindsorTan, 1), 20, 28, 40, 1);
  fbox(shade(GColorWindsorTan, -1), 20, 30, 40, 1);
  fbox(GColorWindsorTan, 22, 31, 3, 5);  fbox(GColorWindsorTan, 55, 31, 3, 5);
  fbox(shade(GColorWindsorTan, -1), 40, 31, 12, 4);
  fbox(GColorWindsorTan, 41, 32, 10, 2);  fbox(GColorRajah, 45, 33, 2, 1);
  // coffee mug with steam
  fbox(GColorWhite, 52, 23, 5, 5);  fbox(GColorWindsorTan, 53, 23, 3, 1);
  fbox(GColorWhite, 57, 24, 1, 3);  fbox(GColorLightGray, 52, 27, 5, 1);
  fbox(GColorLightGray, 53 + t, 21, 1, 1);  fbox(GColorLightGray, 55 - t, 19, 1, 1);
}


static void scene_brush(void) {
  room(GColorCeleste, GColorLightGray);
  const int t = (int)(s_tick & 1);
  // bathroom tiles with a decorative border row
  for (int x = 8; x < FINE_W; x += 8) fbox(GColorBabyBlueEyes, x, 0, 1, FLOOR_FINE);
  for (int y = 8; y < FLOOR_FINE; y += 8) fbox(GColorBabyBlueEyes, 0, y, FINE_W, 1);
  for (int x = 0; x < FINE_W; x += 8) fbox(GColorPictonBlue, x + 3, 16, 2, 2);
  // bath mat on the floor
  fbox(GColorMagenta, 4, 38, 24, 3);  fbox(shade(GColorMagenta, 1), 4, 38, 24, 1);
  // towel on a rail
  fbox(GColorDarkGray, 58, 8, 5, 1);
  fbox(GColorYellow, 58, 9, 5, 14);  fbox(shade(GColorYellow, -1), 58, 18, 5, 1);  fbox(GColorWhite, 59, 12, 3, 1);
  // mirror with a shiny glint
  fbox(GColorDarkGray, 30, 2, 24, 18);  fbox(shade(GColorDarkGray, 1), 30, 2, 24, 1);
  fbox(GColorWhite, 32, 4, 20, 14);
  fbox(GColorCeleste, 34, 5, 4, 1);  fbox(GColorCeleste, 33, 6, 1, 3);  fbox(GColorCeleste, 36, 8, 1, 2);
  // pedestal sink with faucet, running water and a cup of brushes
  fbox(GColorLightGray, 40, 30, 8, 6);  fbox(GColorWhite, 38, 29, 12, 1);
  fbox(GColorWhite, 32, 24, 24, 5);  fbox(GColorLightGray, 32, 28, 24, 1);  fbox(shade(GColorWhite, 0), 32, 24, 24, 1);
  fbox(GColorDarkGray, 43, 19, 3, 5);  fbox(GColorDarkGray, 41, 19, 7, 2);
  fbox(GColorPictonBlue, 44, 22, 1, 2 + t);
  fbox(GColorCyan, 52, 20, 3, 4);  fbox(GColorPictonBlue, 53, 18, 1, 3);  fbox(GColorMagenta, 52, 19, 1, 2);
  // the pet brushing, with foam and bubbles
  pet(5, 10, (s_tick % 9) == 0, 1);
  fbox(GColorWhite, 14, 29, 8, 3);  fbox(GColorWhite, 13, 30, 10, 1);
  fbox(GColorCyan, 22 + t, 30, 9, 2);  fbox(GColorWhite, 21 + t, 29, 2, 4);
  fbox(PET_OUT, 31, 29, 4, 3);
  for (int i = 0; i < 3; i++) {
    int by = 26 - (int)((s_tick + (unsigned)i * 2) % 6) * 2;
    fbox(GColorWhite, 22 + i * 4, by, 2, 2);  fbox(GColorCeleste, 22 + i * 4, by, 1, 1);
  }
}

static void scene_commute(void) {
  bool evening = s_hour >= 16;
  fbox(evening ? GColorMelon : GColorCeleste, 0, 0, FINE_W, 32);
  fbox(evening ? GColorRajah : GColorBabyBlueEyes, 0, 20, FINE_W, 12);     // sky gradient
  int sy = evening ? 18 : 4;
  fbox(evening ? GColorOrange : GColorYellow, 9, sy, 6, 8);  fbox(evening ? GColorOrange : GColorYellow, 8, sy + 1, 8, 6);
  if (!evening && (s_tick & 1)) { fbox(GColorYellow, 11, sy - 2, 2, 1);  fbox(GColorYellow, 11, sy + 9, 2, 1); }
  cloud(10, 3, 10);
  // office building: roof, antenna, windows and door
  fbox(GColorDarkGray, 44, 6, 18, 26);  fbox(shade(GColorDarkGray, 1), 44, 6, 18, 1);
  fbox(GColorBlack, 52, 3, 1, 3);  fbox(GColorRed, 52, 2, 1, 1);
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      bool lit = ((r * 3 + c + (int)(s_tick / 4)) % 3) == 0;
      fbox(lit ? GColorYellow : GColorBabyBlueEyes, 46 + c * 4, 9 + r * 5, 3, 3);
    }
  }
  fbox(GColorWindsorTan, 51, 27, 4, 5);  fbox(GColorRajah, 54, 29, 1, 1);
  // bus stop with a bench
  fbox(GColorDarkGray, 6, 14, 1, 18);
  fbox(GColorRed, 3, 8, 7, 6);  fbox(GColorWhite, 4, 10, 5, 2);  fbox(GColorBlack, 5, 11, 1, 1);  fbox(GColorBlack, 7, 11, 1, 1);
  fbox(GColorWindsorTan, 12, 27, 9, 1);  fbox(GColorWindsorTan, 13, 28, 1, 4);  fbox(GColorWindsorTan, 19, 28, 1, 4);
  // pavement, curb, road with dashes and a zebra crossing
  fbox(GColorLightGray, 0, 32, FINE_W, 4);
  for (int x = 6; x < FINE_W; x += 12) fbox(shade(GColorLightGray, -1), x, 32, 1, 4);
  fbox(GColorDarkGray, 0, 36, FINE_W, 6);  fbox(shade(GColorDarkGray, 1), 0, 36, FINE_W, 1);
  for (int i = 0; i < 6; i++) {
    int x = i * 12 - (int)((s_tick * 3) % 12);
    if (x >= 0) fbox(GColorYellow, x, 39, 6, 1);
  }
  // a little car drives past
  int cx = (int)((s_tick * 4) % 80) - 14;
  fbox(GColorRed, cx, 36, 14, 4);  fbox(GColorRed, cx + 3, 33, 8, 3);
  fbox(GColorCeleste, cx + 4, 34, 3, 2);  fbox(GColorCeleste, cx + 8, 34, 2, 2);
  fbox(GColorYellow, cx + 13, 37, 1, 1);
  fbox(GColorBlack, cx + 2, 40, 3, 2);  fbox(GColorBlack, cx + 9, 40, 3, 2);
  // pet walking with a briefcase
  int hop = (int)(s_tick & 1);
  s_bob = 1;
  pet(11, 9, (s_tick % 7) == 0, 0);
  s_bob = 0;
  fbox(PET_OUT, 38, 26 - hop, 2, 2);
  fbox(GColorWindsorTan, 38, 28 - hop, 6, 5);  fbox(GColorRajah, 38, 28 - hop, 6, 1);
  fbox(GColorBlack, 40, 27 - hop, 2, 1);
}

static void scene_meeting(void) {
  room(GColorBabyBlueEyes, GColorDarkGray);
  const int t = (int)(s_tick & 1);
  wall_clock(57, 3);
  // video-call wall screen with four lively tiles
  fbox(GColorDarkGray, 8, 2, 44, 22);  fbox(shade(GColorDarkGray, 1), 8, 2, 44, 1);
  fbox(GColorBlack, 10, 4, 40, 18);
  static const GColor av[4] = {GColorCyan, GColorYellow, GColorShockingPink, GColorGreen};
  int speaker = (int)((s_tick / 3) % 4);
  for (int i = 0; i < 4; i++) {
    int tx = 11 + (i % 2) * 20, ty = 5 + (i / 2) * 9;
    bool on = (i == speaker);
    fbox(on ? GColorGreen : GColorDarkGray, tx, ty, 18, 8);
    fbox(GColorBlack, tx + 1, ty + 1, 16, 6);
    fbox(shade(av[i], -1), tx + 5, ty + 5, 8, 2);                       // shoulders
    fbox(av[i], tx + 7, ty + 2, 4, 4);                                  // head
    fbox(GColorBlack, tx + 8, ty + 3, 1, 1);  fbox(GColorBlack, tx + 10, ty + 3, 1, 1);
    fbox(on && t ? GColorRed : GColorBlack, tx + 8, ty + 5, 3, 1);       // talking mouth
    if (on) { fbox(GColorGreen, tx + 2, ty + 5 - t, 1, 1 + t);  fbox(GColorGreen, tx + 14, ty + 4 + t, 1, 2 - t); }
  }
  // potted plant in the corner
  fbox(GColorIslamicGreen, 3, 20, 2, 10);  fbox(GColorGreen, 1, 22, 3, 2);  fbox(GColorGreen, 5, 18, 3, 2);
  fbox(GColorWindsorTan, 1, 30, 5, 6);  fbox(shade(GColorWindsorTan, 1), 1, 30, 5, 1);
  // the pet taking part in the meeting
  pet(12, 9, (s_tick % 7) == 0, (int)(((s_tick / 2) % 3) == 0));
  // conference table with papers, pen, mug and a laptop
  fbox(GColorWindsorTan, 6, 32, 52, 4);  fbox(shade(GColorWindsorTan, 1), 6, 32, 52, 1);
  fbox(shade(GColorWindsorTan, -1), 6, 35, 52, 1);
  fbox(GColorBulgarianRose, 8, 36, 48, 6);  fbox(shade(GColorBulgarianRose, -1), 8, 41, 48, 1);
  fbox(GColorWhite, 12, 30, 9, 2);  fbox(GColorLightGray, 13, 31, 7, 1);
  fbox(GColorRed, 22, 31, 4, 1);                                           // pen
  fbox(GColorDarkGray, 36, 28, 9, 4);  fbox(GColorCyan, 37, 29, 7, 2);      // laptop
  fbox(GColorWhite, 50, 28, 4, 4);  fbox(GColorWindsorTan, 51, 28, 2, 1);   // coffee
  fbox(GColorWhite, 54, 29, 1, 2);
  fbox(GColorLightGray, 51 + t, 25, 1, 1);  fbox(GColorLightGray, 52 - t, 23, 1, 1);
}

static void scene_game(void) {
  room(GColorOxfordBlue, GColorImperialPurple);
  const int t = (int)(s_tick & 1);
  // wall poster
  fbox(GColorRed, 6, 6, 9, 11);  fbox(GColorYellow, 9, 8, 3, 3);  fbox(GColorWhite, 8, 13, 5, 1);
  // console stand, TV with a space-shooter, and the console
  fbox(GColorWindsorTan, 30, 30, 32, 6);  fbox(shade(GColorWindsorTan, 1), 30, 30, 32, 1);
  fbox(GColorDarkGray, 38, 28, 12, 2);  if (t) fbox(GColorRed, 48, 29, 1, 1);
  fbox(GColorDarkGray, 32, 6, 28, 22);  fbox(shade(GColorDarkGray, 1), 32, 6, 28, 1);
  fbox(GColorBlack, 34, 8, 24, 18);
  for (int i = 0; i < 6; i++) fbox(GColorWhite, 35 + (i * 7) % 22, 9 + (i * 5) % 15, 1, 1);   // stars
  for (int r = 0; r < 2; r++) {
    for (int c = 0; c < 4; c++) {
      int ax = 36 + c * 5 + (int)((s_tick / 2) % 3);
      int ay = 10 + r * 4;
      GColor ac = r ? GColorMagenta : GColorGreen;
      fbox(ac, ax, ay, 4, 2);  fbox(ac, ax + 1, ay - 1, 2, 1);
      fbox(ac, ax, ay + 2, 1, 1);  fbox(ac, ax + 3, ay + 2, 1, 1);
    }
  }
  int shipx = 38 + (int)(s_tick % 5) * 3;
  fbox(GColorCyan, shipx, 22, 5, 2);  fbox(GColorCyan, shipx + 2, 21, 1, 1);
  fbox(GColorYellow, shipx + 2, 20 - 2 * (int)(s_tick % 4), 1, 2);
  fbox(shade(GColorImperialPurple, 1), 30, 36, 32, 1);                  // screen glow on the floor
  // bean bag and pet with controller
  pet(3, 9, (s_tick % 9) == 0, 0);
  fbox(GColorPictonBlue, 22, 25 + t, 7, 2);                               // arms
  fbox(GColorDarkGray, 28, 25 + t, 6, 3);  fbox(GColorRed, 31, 26 + t, 1, 1);  fbox(GColorGreen, 33, 26 + t, 1, 1);
  fbox(GColorBlueMoon, 2, 30, 26, 6);  fbox(GColorBlueMoon, 4, 28, 22, 2);
  fbox(shade(GColorBlueMoon, 1), 4, 28, 22, 1);  fbox(shade(GColorBlueMoon, -1), 2, 35, 26, 1);
  fbox(shade(GColorBlueMoon, 1), 6, 31, 4, 1);
  // pizza box on the floor
  fbox(GColorWhite, 30, 38, 8, 2);  fbox(GColorRed, 31, 38, 6, 1);
}

static void scene_story(void) {
  // Bedtime story: the pet reads to two tucked-in kids; they nod off at :30.
  bool asleep = s_min >= 30;
  room(GColorDukeBlue, GColorBulgarianRose);
  const int t = (int)(s_tick & 1);
  night_window(14, 2, 6, 5);
  // kids' nightlight on a little table
  for (int r = 0; r < 7; r++) { int w = 3 + r * 2; fbox(shade(GColorDukeBlue, 1), 34 - w / 2, 24 + r, w, 1); }
  fbox(GColorWindsorTan, 32, 31, 5, 5);  fbox(shade(GColorWindsorTan, 1), 32, 31, 5, 1);
  fbox(GColorYellow, 33, 26, 3, 4);  fbox(GColorPastelYellow, 34, 27, 1, 2);
  // the kids' bed: headboard, pillows, little heads
  fbox(GColorWindsorTan, 2, 16, 3, 22);  fbox(GColorRajah, 2, 15, 3, 1);
  fbox(GColorWhite, 4, 23, 13, 5);  fbox(GColorWhite, 17, 23, 13, 5);
  s_kid_sleep = asleep || ((s_tick % 9) == 0);          // awake kids blink now and then
  kid(4, 11, GColorCyan, true);
  kid(10, 11, GColorYellow, true);
  s_kid_sleep = false;
  // blankets: stripes for one, dots for the other
  fbox(GColorCobaltBlue, 4, 27, 13, 9);  fbox(GColorBlueMoon, 4, 27, 13, 2);
  for (int x = 6; x < 16; x += 4) fbox(GColorWhite, x, 30, 1, 5);
  fbox(GColorShockingPink, 17, 27, 13, 9);  fbox(shade(GColorShockingPink, 1), 17, 27, 13, 2);
  for (int x = 19; x < 29; x += 4) { fbox(GColorWhite, x, 31, 2, 2);  fbox(GColorWhite, x + 2, 34, 1, 1); }
  fbox(shade(GColorCobaltBlue, -1), 4, 35, 13, 1);  fbox(shade(GColorShockingPink, -1), 17, 35, 13, 1);
  fbox(GColorWindsorTan, 2, 36, 29, 3);  fbox(shade(GColorWindsorTan, 1), 2, 36, 29, 1);
  // round thought bubble above the beds: dreams of the story, or zzz once asleep
  static const int bx[11] = {10, 8, 7, 6, 6, 6, 6, 6, 7, 8, 10};
  static const int bw[11] = {8, 12, 14, 16, 16, 16, 16, 16, 14, 12, 8};
  for (int r = 0; r < 11; r++) fbox(GColorWhite, bx[r], 3 + r, bw[r], 1);
  fbox(GColorLightGray, 8, 13, 12, 1);
  fbox(GColorWhite, 11, 15, 3, 2);  fbox(GColorWhite, 9, 18, 2, 2);
  if (asleep) {
    static const char *zs[3] = {"z", "zz", "zzz"};
    txt(zs[(s_tick / 2) % 3], FONT_KEY_GOTHIC_24_BOLD, GColorDarkGray, 4, 1, 8, 7);
  } else {
    switch ((s_tick / 4) % 3) {
      case 0:
        fbox(GColorYellow, 13, 5, 2, 8);  fbox(GColorYellow, 10, 8, 8, 2);  fbox(GColorWhite, 13, 8, 2, 2);
        break;
      case 1:
        fheart(GColorShockingPink, 11, 6);  fheart(GColorRed, 15, 8);
        break;
      default:
        fbox(GColorPastelYellow, 11, 5, 6, 8);  fbox(GColorPastelYellow, 10, 6, 8, 6);
        fbox(GColorWhite, 14, 5, 5, 6);
        break;
    }
  }
  // armchair with the pet reading aloud
  fbox(GColorOrange, 40, 14, 22, 22);  fbox(shade(GColorOrange, 1), 40, 14, 22, 1);
  fbox(shade(GColorOrange, -1), 46, 18, 2, 2);  fbox(shade(GColorOrange, -1), 54, 18, 2, 2);   // tufted buttons
  pet(22, 7, (s_tick % 7) == 0, asleep ? 0 : (int)((s_tick / 2) & 1));
  fbox(GColorChromeYellow, 40, 32, 22, 4);  fbox(shade(GColorChromeYellow, 1), 40, 32, 22, 1);
  fbox(GColorOrange, 38, 24, 4, 12);  fbox(shade(GColorOrange, 1), 38, 24, 4, 1);
  fbox(GColorOrange, 60, 24, 4, 12);  fbox(shade(GColorOrange, 1), 60, 24, 4, 1);
  fbox(GColorWindsorTan, 41, 36, 2, 2);  fbox(GColorWindsorTan, 59, 36, 2, 2);
  // big storybook with text lines and a page turn
  fbox(GColorWhite, 46, 26, 6, 6);  fbox(GColorWhite, 52, 26, 6, 6);
  fbox(GColorDarkGray, 52, 26, 1, 6);  fbox(GColorLightGray, 46, 31, 12, 1);
  for (int r = 0; r < 2; r++) { fbox(GColorLightGray, 47, 27 + r * 2, 4, 1);  fbox(GColorLightGray, 53, 27 + r * 2, 4, 1); }
  if ((s_tick / 4) & 1) { fbox(GColorWhite, 52, 24, 5, 2);  fbox(GColorLightGray, 53, 25, 3, 1); }
  if (!asleep) { fbox(GColorYellow, 58 - t * 2, 22 - t, 1, 1);  fbox(GColorYellow, 60, 25 + t, 1, 1); }   // story sparkles
}

static void scene_read(void) {
  room(GColorImperialPurple, GColorBulgarianRose);
  const int t = (int)(s_tick & 1);
  // night window with curtains
  night_window(4, 2, 9, 8);
  // bookshelf
  fbox(GColorWindsorTan, 30, 18, 20, 1);  fbox(shade(GColorWindsorTan, 1), 30, 18, 20, 1);
  static const GColor bk[6] = {GColorRed, GColorGreen, GColorYellow, GColorCyan, GColorMagenta, GColorOrange};
  static const int bh[6] = {7, 6, 8, 5, 7, 6};
  for (int i = 0; i < 6; i++) { fbox(bk[i], 31 + i * 3, 18 - bh[i], 2, bh[i]);  fbox(shade(bk[i], 1), 31 + i * 3, 18 - bh[i], 1, bh[i]); }
  // lamp glow cone (behind the bed)
  for (int r = 0; r < 14; r++) { int w = 6 + r * 2; fbox(shade(GColorImperialPurple, 1), 55 - w / 2, 22 + r, w, 1); }
  // bed: headboard, pillows, pet sitting up, blanket, book
  fbox(GColorWindsorTan, 6, 14, 4, 24);  fbox(GColorRajah, 6, 13, 4, 1);
  fbox(GColorWhite, 10, 22, 14, 10);  fbox(GColorWhite, 14, 20, 10, 4);
  fbox(GColorLightGray, 10, 31, 14, 1);
  pet(7, 5, (s_tick % 3) == 0, 0);
  fbox(GColorCobaltBlue, 10, 24, 40, 12);
  fbox(GColorBlueMoon, 10, 24, 40, 2);
  for (int x = 14; x < 48; x += 8) fbox(shade(GColorCobaltBlue, 1), x, 28, 3, 6);   // blanket stripes
  fbox(shade(GColorCobaltBlue, -1), 10, 35, 40, 1);
  fbox(GColorWindsorTan, 10, 36, 40, 4);  fbox(shade(GColorWindsorTan, 1), 10, 36, 40, 1);
  // open book with text lines, spine and a page flip
  fbox(GColorWhite, 28, 20, 6, 8);  fbox(GColorWhite, 34, 20, 6, 8);
  fbox(GColorDarkGray, 34, 20, 1, 8);  fbox(GColorLightGray, 28, 27, 12, 1);
  for (int r = 0; r < 3; r++) { fbox(GColorLightGray, 29, 21 + r * 2, 4, 1);  fbox(GColorLightGray, 35, 21 + r * 2, 4, 1); }
  if ((s_tick / 4) & 1) { fbox(GColorWhite, 34, 18, 5, 2);  fbox(GColorLightGray, 35, 19, 3, 1); }
  // bedside table and lamp
  fbox(GColorWindsorTan, 50, 28, 12, 8);  fbox(shade(GColorWindsorTan, 1), 50, 28, 12, 1);
  fbox(GColorRajah, 55, 32, 2, 1);
  fbox(GColorDarkGray, 55, 23, 2, 5);
  fbox(GColorYellow, 52, 16, 8, 7);  fbox(GColorPastelYellow, 53, 17, 6, 1);  fbox(shade(GColorYellow, -1), 52, 22, 8, 1);
  if (t) fbox(GColorWhite, 56, 14, 1, 1);
}

static void scene_walk(void) {
  const int t = (int)(s_tick & 1);
  fbox(GColorCeleste, 0, 0, FINE_W, 28);
  fbox(GColorBabyBlueEyes, 0, 18, FINE_W, 10);
  // sun with twinkling rays
  fbox(GColorYellow, 7, 4, 6, 6);  fbox(GColorYellow, 6, 5, 8, 4);
  if (t) { fbox(GColorYellow, 9, 1, 2, 2);  fbox(GColorYellow, 9, 12, 2, 2);  fbox(GColorYellow, 3, 6, 2, 2);  fbox(GColorYellow, 15, 6, 2, 2); }
  else   { fbox(GColorYellow, 4, 2, 2, 2);  fbox(GColorYellow, 14, 2, 2, 2);  fbox(GColorYellow, 4, 11, 2, 2);  fbox(GColorYellow, 14, 11, 2, 2); }
  cloud(18, 4, 12);
  cloud(34, 7, 10);
  // rolling hills, meadow and a dirt path
  fbox(GColorMayGreen, 0, 24, FINE_W, 6);  fbox(GColorMayGreen, 8, 22, 18, 3);  fbox(GColorMayGreen, 38, 21, 14, 4);
  fbox(GColorGreen, 0, 28, FINE_W, 14);
  for (int x = 3; x < FINE_W; x += 9) { fbox(GColorIslamicGreen, x, 30 + (x % 3), 1, 2);  fbox(GColorIslamicGreen, x + 1, 31 + (x % 3), 1, 1); }
  fbox(GColorRajah, 0, 36, FINE_W, 6);  fbox(shade(GColorRajah, 1), 0, 36, FINE_W, 1);
  for (int x = 4; x < FINE_W; x += 11) fbox(GColorWindsorTan, x, 38 + (x % 3), 2, 1);
  // leafy tree with apples
  fbox(GColorWindsorTan, 50, 18, 4, 14);  fbox(shade(GColorWindsorTan, -1), 53, 18, 1, 14);
  fbox(GColorIslamicGreen, 42, 6, 20, 14);  fbox(GColorIslamicGreen, 44, 4, 16, 2);
  fbox(GColorGreen, 44, 6, 6, 3);  fbox(GColorGreen, 52, 10, 6, 3);  fbox(GColorGreen, 45, 13, 4, 2);
  fbox(GColorRed, 47, 11, 2, 2);  fbox(GColorRed, 55, 8, 2, 2);  fbox(GColorRed, 58, 14, 2, 2);
  // flowers with stems
  static const GColor fl[6] = {GColorRed, GColorYellow, GColorShockingPink, GColorWhite, GColorRed, GColorYellow};
  static const int fx[6] = {2, 18, 40, 56, 30, 10};
  static const int fy[6] = {32, 30, 34, 33, 29, 29};
  for (int i = 0; i < 6; i++) {
    fbox(GColorIslamicGreen, fx[i], fy[i] + 2, 1, 3);
    fbox(fl[i], fx[i] - 1, fy[i], 3, 1);  fbox(fl[i], fx[i], fy[i] - 1, 1, 3);
    fbox(GColorYellow, fx[i], fy[i], 1, 1);
  }
  // butterfly with flapping wings
  int bx = 8 + (int)(s_tick % 14) * 4;
  int by = 12 + (int)((s_tick / 2) % 2) * 2;
  fbox(GColorMagenta, bx, by, 1, 2);
  fbox(GColorShockingPink, bx - 2, by - (t ? 1 : 0), 2, 2);  fbox(GColorShockingPink, bx + 1, by - (t ? 1 : 0), 2, 2);
  // pet out for a walk with a kid in tow
  s_bob = 1;
  pet(14, 10, (s_tick % 7) == 0, 0);
  s_bob = 0;
  kid(6, 13 + (int)((s_tick + 1) & 1), GColorCyan, true);
}

static void table_scene(bool family) {
  room(GColorPastelYellow, GColorRajah);
  int mouth = (s_tick & 1);
  // window
  window_glass(12, 2, 8, 7);
  cloud(13, 4, 5);
  window_frame(12, 2, 8, 7);
  pet(2, 7, (s_tick % 7) == 0, mouth);
  if (family) {
    kid(19, 11, GColorCyan, mouth);
    kid(24, 11, GColorYellow, !mouth);
  }
  // table with a gingham cloth, plates and food
  const int t = (int)(s_tick & 1);
  fbox(GColorWindsorTan, 14, 31, 3, 5);  fbox(GColorWindsorTan, 55, 31, 3, 5);
  fbox(GColorRed, 12, 28, 48, 4);
  for (int x = 12; x < 60; x += 4) { fbox(GColorWhite, x, 28, 2, 2);  fbox(GColorWhite, x + 2, 30, 2, 2); }
  fbox(shade(GColorRed, 1), 12, 28, 48, 1);  fbox(shade(GColorRed, -1), 12, 31, 48, 1);
  // burger on a plate
  fbox(GColorWhite, 21, 26, 12, 2);  fbox(GColorLightGray, 21, 27, 12, 1);
  fbox(GColorWindsorTan, 23, 23, 8, 2);                                    // patty
  fbox(GColorGreen, 23, 22, 8, 1);                                         // lettuce
  fbox(GColorChromeYellow, 23, 19, 8, 3);  fbox(GColorChromeYellow, 24, 18, 6, 1);   // top bun
  fbox(GColorWhite, 25, 19, 1, 1);  fbox(GColorWhite, 28, 19, 1, 1);        // sesame
  fbox(GColorChromeYellow, 23, 25, 8, 1);                                  // bottom bun
  fbox(GColorLightGray, 26 + t, 15, 1, 2);  fbox(GColorLightGray, 28 - t, 13, 1, 2);   // steam
  // juice glass
  fbox(GColorOrange, 36, 24, 4, 4);  fbox(GColorWhite, 36, 23, 4, 1);  fbox(GColorWhite, 36, 24, 1, 4);
  if (family) {
    fbox(GColorWhite, 41, 26, 8, 2);  fbox(GColorRed, 43, 24, 4, 2);  fbox(GColorYellow, 43, 23, 4, 1);   // spaghetti
    fbox(GColorWhite, 51, 26, 8, 2);  fbox(GColorGreen, 52, 24, 6, 2);  fbox(GColorRed, 54, 23, 2, 1);    // salad
  }
}

static void scene_play(void) {
  room(GColorMintGreen, GColorRajah);
  // window
  window_glass(12, 2, 8, 7);
  cloud(13, 4, 5);
  window_frame(12, 2, 8, 7);
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
    heart_px(GColorMelon, 6, 3);
  }
}

static void scene_chill(void) {
  room(GColorBlueMoon, GColorImperialPurple);
  const int t = (int)(s_tick % 3);
  // rug
  fbox(GColorIndigo, 6, 38, 50, 3);  fbox(shade(GColorIndigo, 1), 6, 38, 50, 1);
  // TV on a console, showing a little cartoon with scanlines
  fbox(GColorWindsorTan, 34, 32, 28, 4);  fbox(shade(GColorWindsorTan, 1), 34, 32, 28, 1);
  fbox(GColorDarkGray, 36, 10, 24, 20);  fbox(shade(GColorDarkGray, 1), 36, 10, 24, 1);
  fbox(GColorCyan, 38, 12, 20, 8);  fbox(GColorGreen, 38, 20, 20, 8);
  fbox(GColorYellow, 53, 13, 3, 3);                                 // cartoon sun
  fbox(GColorWhite, 40 + (int)((s_tick * 2) % 14), 19, 2, 2);        // runner
  for (int yy = 12; yy < 28; yy += 2) fbox(shade(yy < 20 ? GColorCyan : GColorGreen, -1), 38, yy, 20, 1);
  if (t == 0) fbox(GColorWhite, 38, 12, 20, 1);                      // flicker
  fbox(GColorRed, 57, 28, 1, 1);
  // sofa: back, pet, seat, arms
  fbox(GColorBulgarianRose, 2, 16, 30, 20);  fbox(shade(GColorBulgarianRose, 1), 2, 16, 30, 1);
  fbox(GColorMelon, 6, 22, 5, 5);  fbox(shade(GColorMelon, -1), 6, 26, 5, 1);   // throw pillow
  pet(5, 9, (s_tick % 9) == 0, 0);
  fbox(GColorDarkCandyAppleRed, 2, 28, 30, 8);  fbox(shade(GColorDarkCandyAppleRed, 1), 2, 28, 30, 1);
  fbox(shade(GColorDarkCandyAppleRed, -1), 16, 29, 1, 7);
  fbox(GColorDarkCandyAppleRed, 0, 24, 5, 12);  fbox(GColorDarkCandyAppleRed, 29, 24, 5, 12);
  fbox(shade(GColorDarkCandyAppleRed, 1), 0, 24, 5, 1);  fbox(shade(GColorDarkCandyAppleRed, 1), 29, 24, 5, 1);
  // popcorn bowl on the cushion
  fbox(GColorWhite, 21, 27, 7, 3);  fbox(GColorRed, 21, 28, 7, 1);
  fbox(GColorPastelYellow, 22, 25, 2, 2);  fbox(GColorWhite, 24, 24, 2, 2);  fbox(GColorPastelYellow, 26, 25, 2, 2);
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

static Activity pick_activity(int hour, int min, int wday) {
  bool weekend = (wday == 0 || wday == 6);
  if (hour < 7 || hour >= 22) return ACT_SLEEP;
  if (hour == 7) return (min < 30) ? ACT_WAKE : ACT_BRUSH;
  if (hour == 20) return ACT_STORY;
  if (hour == 21) return ACT_READ;
  if (weekend) {
    if (hour == 8 || hour == 12) return ACT_LUNCH;     // breakfast / lunch
    if (hour <= 11) return ACT_WALK;
    if (hour <= 14) return ACT_CHILL;
    if (hour <= 16) return ACT_GAME;
    if (hour == 18) return ACT_DINNER;
    return ACT_PLAY;
  }
  if (hour == 8) return ACT_COMMUTE;
  if (hour == 12) return ACT_LUNCH;
  if (hour == 17) return ACT_COMMUTE;
  if (hour == 18) return ACT_DINNER;
  if (hour >= 19) return ACT_PLAY;
  if ((hour == 11 || hour == 15) && min < 30) return ACT_MEETING;
  return ACT_CODE;
}

// ---------------------------------------------------------------- drawing

// ------------------------------------------------- eldritch scene quirks
// Only the eldritch pets bring their own little twists to a scene.
static void eldritch_quirk(Activity a) {
  const int t = (int)(s_tick & 1);
  switch (s_sp.ears) {
    case EARS_CTHULHU:
      if (a == ACT_CODE) {                        // a tentacle curls out of the coffee mug
        for (int j = 0; j < 7; j++) {
          fbox(GColorMayGreen, 53 + (((j + (int)s_tick) % 3) == 0 ? 1 : 0), 22 - j, 2, 1);
        }
        fbox(GColorIslamicGreen, 54, 15, 2, 1);
      } else if (a == ACT_SLEEP) {                // a tentacle wiggles out of the blanket
        for (int j = 0; j < 6; j++) fbox(GColorMayGreen, 42 + (((j + t) & 1) ? 1 : 0), 25 - j, 2, 1);
        fbox(GColorIslamicGreen, 42 + t, 19, 2, 1);
      }
      break;
    case EARS_SHOGGOTH:
      if (a == ACT_CODE) {                        // eyes open up on the monitor, goo drips from it
        int pdx = (int)((s_tick / 2) % 3) - 1;
        fbox(GColorYellow, 31, 14, 4, 3);  fbox(GColorYellow, 39, 14, 4, 3);
        fbox(GColorBlack, 32 + pdx + 1, 15, 1, 2);  fbox(GColorBlack, 40 + pdx + 1, 15, 1, 2);
        fbox(GColorPurple, 28, 8, 8, 2);  fbox(GColorPurple, 30, 10, 2, 2 + t);
        fbox(GColorPurple, 44, 8, 4, 2);
      } else if (a == ACT_SLEEP) {                // a purple puddle spreads under the bed
        fbox(GColorPurple, 16, 40, 28, 2);  fbox(GColorIndigo, 20, 41, 20, 1);
        fbox(GColorMagenta, 24, 40, 2, 1);  fbox(GColorMagenta, 34 + t, 40, 2, 1);
        fbox(GColorPurple, 38, 36, 2, 4 + t);                 // drip off the mattress
      }
      break;
    case EARS_DEEP:
      if (a == ACT_CODE) {                        // a goldfish bowl replaces the mug
        fbox(GColorBabyBlueEyes, 50, 21, 9, 7);  fbox(GColorWhite, 51, 20, 7, 1);
        fbox(GColorPictonBlue, 51, 23, 7, 4);
        fbox(GColorOrange, 52 + (int)((s_tick / 2) % 3), 24 + t, 2, 1);
        fbox(GColorRed, 51 + (int)((s_tick / 2) % 3), 24 + t, 1, 1);
        fbox(GColorWhite, 55, 22 - t, 1, 1);
        fbox(GColorGreen, 57, 25, 1, 2);
      } else if (a == ACT_BRUSH) {                // a fish leaps out of the sink
        int ph = (int)(s_tick % 6);
        int fx = 34 + ph * 4, fy = 24 - (ph * (6 - ph) * 2) / 3;
        fbox(GColorOrange, fx, fy, 5, 3);  fbox(GColorWhite, fx + 1, fy + 2, 3, 1);
        fbox(GColorRed, fx - 2, fy - 1, 2, 1);  fbox(GColorRed, fx - 2, fy + 2, 2, 1);
        fbox(GColorBlack, fx + 3, fy, 1, 1);
        if (ph == 0 || ph == 5) {                       // splash
          fbox(GColorWhite, fx - 1, 22, 1, 1);  fbox(GColorWhite, fx + 5, 21, 1, 1);  fbox(GColorWhite, fx + 2, 20, 1, 1);
        }
      }
      break;
    case EARS_EYE:
      if (a == ACT_CODE) {                        // the monitor shows a giant watching eye
        fbox(GColorBlack, 26, 10, 22, 13);
        fbox(GColorWhite, 30, 12, 14, 9);  fbox(GColorLightGray, 30, 12, 14, 1);
        int ix = 35 + (int)((s_tick / 3) % 3) - 1;
        if ((s_tick % 7) == 0) {
          fbox(GColorBlack, 30, 16, 14, 1);
        } else {
          fbox(GColorMagenta, ix, 13, 5, 7);  fbox(GColorBlack, ix + 1, 13, 3, 7);
          fbox(GColorWhite, ix, 13, 1, 2);
        }
        fbox(GColorRed, 31, 13, 2, 1);  fbox(GColorRed, 42, 19, 1, 2);
      } else if (a == ACT_SLEEP) {                // the moon has an eye that blinks
        if ((s_tick % 5) == 0) {
          fbox(GColorBlack, 49, 10, 6, 1);
        } else {
          fbox(GColorWhite, 49, 8, 6, 5);  fbox(GColorMagenta, 51, 9, 2, 3);  fbox(GColorBlack, 51, 10, 2, 1);
        }
      }
      break;
    default:
      break;
  }
}

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
  U = w / 86;
  if (U < 1) U = 1;
  OX = (w - FINE_W * U) / 2;
  OY = h * 74 / 260;

  graphics_context_set_fill_color(ctx, GColorShockingPink);
  graphics_fill_rect(ctx, GRect(OX - 2 * U, OY - 2 * U, (FINE_W + 4) * U, (FINE_H + 4) * U),
                     6 * U, GCornersAll);

  s_bob = 0;
  s_floor_ok = false;
  Activity act = pick_activity(s_hour, s_min, s_wday);
  switch (act) {
    case ACT_SLEEP:  scene_sleep();       break;
    case ACT_WAKE:   scene_wake();        break;
    case ACT_CODE:   scene_code();        break;
    case ACT_LUNCH:  table_scene(false);  break;
    case ACT_DINNER: table_scene(true);   break;
    case ACT_PLAY:   scene_play();        break;
    case ACT_CHILL:  scene_chill();       break;
    case ACT_BRUSH:  scene_brush();       break;
    case ACT_COMMUTE: scene_commute();    break;
    case ACT_MEETING: scene_meeting();    break;
    case ACT_GAME:   scene_game();        break;
    case ACT_STORY:  scene_story();       break;
    case ACT_READ:   scene_read();        break;
    case ACT_WALK:   scene_walk();        break;
  }
  eldritch_quirk(act);

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
  strftime(s_date_buf, sizeof(s_date_buf), "%a %b ", t);
  size_t dl = strlen(s_date_buf);
  snprintf(s_date_buf + dl, sizeof(s_date_buf) - dl, "%d", t->tm_mday);   // no padding space
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_tick++;
  if (units_changed & MINUTE_UNIT) {
    update_time();
    update_steps();
  }
  layer_mark_dirty(s_canvas);
}

static uint32_t s_last_tap;
static bool s_tapped_once;

// One flick pets the pet (hearts + a happy hop); a second flick within two
// seconds switches to the next pet instead.
static void tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_tapped_once && s_tick - s_last_tap <= 2) {
    s_species = (s_species + 1) % NUM_SPECIES;
    load_species(s_species);
    persist_write_int(PERSIST_SPECIES, s_species);
    s_react_until = 0;
    s_tapped_once = false;
  } else {
    s_react_start = s_tick;
    s_react_until = s_tick + 5;
    s_last_tap = s_tick;
    s_tapped_once = true;
  }
  layer_mark_dirty(s_canvas);
}

// Animate once a second only while the face is visible and the battery is
// healthy; otherwise tick once a minute to save power.
static bool s_in_focus = true;

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

  s_species = persist_exists(PERSIST_SPECIES) ? persist_read_int(PERSIST_SPECIES) % NUM_SPECIES : 0;
  if (s_species < 0) s_species = 0;   // ignore a corrupt saved value
  load_species(s_species);
  update_time();
  update_steps();
  s_batt = battery_state_service_peek().charge_percent;

  // One tick per second drives the 1 fps retro animation (see update_tick_rate).
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
