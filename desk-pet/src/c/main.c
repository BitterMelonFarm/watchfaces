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
  const Mood md = sl ? MOOD_OK : s_mood;
  if (sl) { blink = true; mouth = 0; }
  if (!sl && s_bob && (s_tick & 1)) y -= 1;        // little hop
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
  // bed with the sleeping pet's head on the pillow
  box(GColorWindsorTan, 5, 18, 22, 2);
  box(GColorWindsorTan, 5, 12, 1, 8);
  fbox(GColorWhite, 12, 25, 18, 5);
  fbox(GColorLightGray, 12, 29, 18, 1);
  s_sleeping = true;
  pet(6, 8, true, 0);
  s_sleeping = false;
  if (s_tick & 2) fbox(GColorCeleste, 29, 23, 3, 3);   // sleepy snot bubble
  else fbox(GColorCeleste, 29, 24, 2, 2);
  box(GColorCobaltBlue, 12, 13, 14, 5);          // blanket
  box(GColorBlueMoon, 12, 13, 14, 1);
  box(GColorWhite, 15, 15, 2, 1);                // blanket stitching
  box(GColorWhite, 21, 15, 2, 1);
  // floating z's
  static const char *zs[3] = {"z", "zz", "zzz"};
  txt(zs[(s_tick / 2) % 3], FONT_KEY_GOTHIC_24_BOLD, GColorWhite, 13, 4, 10, 8);
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


// Box clamped to the scene (for things that slide in from the edges).
static void cbox(GColor c, int x, int y, int w, int h) {
  if (x < 0) { w += x; x = 0; }
  if (x + w > SCENE_W) w = SCENE_W - x;
  if (w > 0 && h > 0) box(c, x, y, w, h);
}

static void scene_brush(void) {
  room(GColorCeleste, GColorLightGray);
  for (int i = 4; i < SCENE_W; i += 4) box(GColorBabyBlueEyes, i, 0, 1, FLOOR_Y);   // tiles
  for (int j = 4; j < FLOOR_Y; j += 4) box(GColorBabyBlueEyes, 0, j, SCENE_W, 1);
  // towel on a rail
  box(GColorDarkGray, 29, 4, 2, 1);
  box(GColorYellow, 29, 5, 2, 7);
  // mirror
  box(GColorDarkGray, 15, 1, 12, 9);
  box(GColorWhite, 16, 2, 10, 7);
  box(GColorCeleste, 17, 3, 2, 1);
  px(17, 4, GColorCeleste);
  // sink
  box(GColorLightGray, 20, 15, 4, 3);
  box(GColorWhite, 16, 12, 12, 3);
  box(GColorDarkGray, 21, 10, 2, 2);
  // pet brushing
  pet(5, 10, (s_tick % 9) == 0, 1);
  box(GColorWhite, 7, 14, 4, 2);                          // toothpaste foam
  box(GColorCyan, 9 + (int)(s_tick & 1), 14, 4, 1);        // toothbrush
  box(PET_OUT, 13, 14, 2, 1);                            // hand
  // bubbles floating up
  px(14, 9 - (int)(s_tick % 3), GColorWhite);
  px(3, 8 + (int)((s_tick + 1) % 3), GColorWhite);
}

static void scene_commute(void) {
  bool evening = s_hour >= 16;
  box(evening ? GColorMelon : GColorCeleste, 0, 0, SCENE_W, 16);
  box(evening ? GColorOrange : GColorYellow, 4, evening ? 10 : 2, 4, 3);
  cloud(10, 3, 10);
  // office building with blinking windows
  box(GColorDarkGray, 22, 3, 8, 13);
  for (int r = 0; r < 4; r++) {
    for (int c = 0; c < 4; c++) {
      bool lit = ((r * 3 + c + (int)(s_tick / 4)) % 3) == 0;
      box(lit ? GColorYellow : GColorBabyBlueEyes, 23 + c * 2, 4 + r * 3, 1, 2);
    }
  }
  // bus stop sign
  box(GColorDarkGray, 3, 7, 1, 9);
  box(GColorRed, 2, 5, 3, 3);
  // pavement and road
  box(GColorLightGray, 0, 16, SCENE_W, 2);
  box(GColorDarkGray, 0, 18, SCENE_W, 3);
  for (int i = 0; i < 5; i++) {
    int x = i * 8 - (int)((s_tick * 2) % 8);
    if (x >= 0) box(GColorYellow, x, 19, 4, 1);
  }
  // a little car drives past
  int cx = (int)((s_tick * 2) % 40) - 7;
  cbox(GColorRed, cx, 18, 7, 2);
  cbox(GColorRed, cx + 1, 17, 4, 1);
  cbox(GColorCeleste, cx + 2, 17, 2, 1);
  cbox(GColorBlack, cx + 1, 20, 2, 1);
  cbox(GColorBlack, cx + 4, 20, 2, 1);
  // pet walking with a briefcase
  int hop = (int)(s_tick & 1);
  s_bob = 1;
  pet(11, 9, (s_tick % 7) == 0, 0);
  s_bob = 0;
  px(19, 13 - hop, PET_OUT);
  box(GColorWindsorTan, 19, 14 - hop, 3, 3);
}

static void scene_meeting(void) {
  room(GColorBabyBlueEyes, GColorDarkGray);
  // video-call screen on the wall
  box(GColorDarkGray, 5, 1, 22, 10);
  box(GColorBlack, 6, 2, 20, 8);
  static const GColor avatar[4] = {GColorCyan, GColorYellow, GColorShockingPink, GColorGreen};
  int speaker = (int)((s_tick / 3) % 4);
  for (int i = 0; i < 4; i++) {
    int tx = 7 + (i % 2) * 9, ty = 3 + (i / 2) * 3;
    box(i == speaker ? GColorGreen : GColorDarkGray, tx, ty, 8, 3);
    box(avatar[i], tx + 3, ty, 2, 2);
    box(avatar[i], tx + 2, ty + 2, 4, 1);
  }
  // pet talking in the meeting
  pet(12, 9, (s_tick % 7) == 0, (int)(((s_tick / 2) % 3) == 0));
  // table in front
  box(GColorWindsorTan, 3, 16, 26, 2);
  box(GColorBulgarianRose, 4, 18, 24, 3);
  box(GColorWhite, 6, 15, 4, 1);                          // papers
  box(GColorWhite, 25, 14, 2, 2);                         // coffee
  px(25 + (int)(s_tick & 1), 12, GColorLightGray);
}

static void scene_game(void) {
  room(GColorOxfordBlue, GColorImperialPurple);
  // TV running a tiny space-shooter
  box(GColorDarkGray, 16, 3, 14, 11);
  box(GColorBlack, 17, 4, 12, 9);
  for (int r = 0; r < 2; r++) {
    for (int c = 0; c < 4; c++) {
      box(r ? GColorMagenta : GColorGreen, 18 + c * 3 + (int)((s_tick / 2) % 2), 5 + r * 2, 2, 1);
    }
  }
  int shipx = 19 + (int)(s_tick % 5) * 2;
  box(GColorCyan, shipx, 12, 3, 1);
  px(shipx + 1, 11, GColorCyan);
  px(shipx + 1, 11 - 1 - (int)(s_tick % 4), GColorYellow);   // laser
  box(GColorDarkGray, 21, 14, 4, 1);
  // bean bag and pet with controller
  pet(3, 9, (s_tick % 9) == 0, 0);
  box(GColorDarkGray, 11, 14 + (int)(s_tick & 1), 3, 2);   // controller
  box(GColorBlueMoon, 1, 15, 13, 3);
  box(GColorBlueMoon, 3, 14, 9, 1);
}

static void scene_story(void) {
  // Bedtime story: the pet reads to two tucked-in kids; they nod off at :30.
  bool asleep = s_min >= 30;
  room(GColorDukeBlue, GColorBulgarianRose);
  // night window
  box(GColorWhite, 18, 2, 7, 6);
  box(GColorOxfordBlue, 19, 3, 5, 4);
  px(21, 4, GColorPastelYellow);
  if ((s_tick + 1) % 3) px(22, 6, GColorWhite);
  if ((s_tick + 2) % 3) px(19, 5, GColorWhite);
  box(GColorWhite, 21, 2, 1, 6);
  // the kids' bed: headboard, pillows, little heads, blanket
  box(GColorWindsorTan, 1, 8, 1, 11);
  box(GColorWhite, 2, 11, 6, 3);
  box(GColorWhite, 8, 11, 6, 3);
  s_kid_sleep = asleep || ((s_tick % 9) == 0);          // awake kids blink now and then
  kid(3, 10, GColorCyan, true);
  kid(9, 10, GColorYellow, true);
  s_kid_sleep = false;
  box(GColorCobaltBlue, 2, 13, 13, 5);
  box(GColorBlueMoon, 2, 13, 13, 1);
  box(GColorWhite, 4, 15, 2, 1);                         // blanket stitching
  box(GColorWhite, 10, 15, 2, 1);
  box(GColorWindsorTan, 1, 17, 15, 2);
  // thought bubble above the beds: dreams of the story, or zzz once asleep
  box(GColorWhite, 3, 2, 9, 6);
  box(GColorWhite, 5, 8, 1, 1);
  box(GColorWhite, 4, 9, 1, 1);
  if (asleep) {
    static const char *zs[3] = {"z", "zz", "zzz"};
    txt(zs[(s_tick / 2) % 3], FONT_KEY_GOTHIC_24_BOLD, GColorDarkGray, 4, 1, 8, 7);
  } else {
    switch ((s_tick / 4) % 3) {
      case 0:
        box(GColorYellow, 7, 3, 1, 4);
        box(GColorYellow, 6, 4, 3, 1);
        break;
      case 1:
        heart_px(GColorShockingPink, 6, 4);
        break;
      default:
        box(GColorPastelYellow, 6, 3, 3, 4);
        box(GColorWhite, 7, 3, 2, 3);
        break;
    }
  }
  // armchair with the pet reading aloud
  box(GColorOrange, 20, 7, 11, 11);
  pet(22, 7, (s_tick % 7) == 0, asleep ? 0 : (int)((s_tick / 2) & 1));
  box(GColorChromeYellow, 20, 16, 11, 2);                // seat cushion
  box(GColorOrange, 19, 12, 2, 6);                       // arms
  box(GColorOrange, 30, 12, 2, 6);
  // big storybook
  box(GColorWhite, 23, 13, 3, 3);
  box(GColorWhite, 26, 13, 3, 3);
  box(GColorDarkGray, 26, 13, 1, 3);
  px(24, 14, GColorLightGray);
  px(27, 14, GColorLightGray);
  if ((s_tick / 4) & 1) px(26, 12, GColorWhite);         // page turn
}

static void scene_read(void) {
  room(GColorImperialPurple, GColorBulgarianRose);
  // night window
  box(GColorWhite, 4, 2, 9, 8);
  box(GColorOxfordBlue, 5, 3, 7, 6);
  px(9, 4, GColorPastelYellow);
  px(7, 6, GColorWhite);
  px(10, 7, GColorWhite);
  box(GColorWhite, 8, 2, 1, 8);
  // bedside lamp
  box(GColorWindsorTan, 25, 13, 6, 5);
  box(GColorYellow, 26, 8, 4, 3);
  box(GColorDarkGray, 27, 11, 2, 2);
  // bed: headboard, pet sitting up, blanket, book
  box(GColorWindsorTan, 3, 7, 2, 12);
  pet(7, 5, (s_tick % 3) == 0, 0);
  box(GColorWhite, 5, 11, 6, 5);                          // pillow behind
  box(GColorCobaltBlue, 5, 12, 20, 6);
  box(GColorBlueMoon, 5, 12, 20, 1);
  box(GColorWindsorTan, 5, 18, 20, 2);
  box(GColorWhite, 14, 10, 3, 4);                         // open book
  box(GColorWhite, 17, 10, 3, 4);
  box(GColorDarkGray, 17, 10, 1, 4);
  if ((s_tick / 4) & 1) px(17, 9, GColorWhite);              // page flip
  px(15, 11, GColorLightGray);
  px(18, 12, GColorLightGray);
}

static void scene_walk(void) {
  box(GColorCeleste, 0, 0, SCENE_W, 14);
  box(GColorGreen, 0, 14, SCENE_W, 7);
  box(GColorRajah, 0, 18, SCENE_W, 3);
  box(GColorYellow, 3, 2, 3, 3);
  cloud(9, 4, 12);
  // tree
  box(GColorWindsorTan, 25, 9, 2, 6);
  box(GColorIslamicGreen, 22, 3, 8, 7);
  box(GColorIslamicGreen, 23, 2, 6, 1);
  px(24, 5, GColorRed);
  px(27, 7, GColorRed);
  // flowers
  static const GColor fl[6] = {GColorRed, GColorYellow, GColorShockingPink,
                               GColorWhite, GColorRed, GColorYellow};
  static const int fx[6] = {1, 9, 20, 28, 15, 5};
  static const int fy[6] = {16, 15, 17, 16, 14, 14};
  for (int i = 0; i < 6; i++) px(fx[i], fy[i], fl[i]);
  // butterfly
  int bx = 4 + (int)(s_tick % 14) * 2;
  int by = 6 + (int)((s_tick / 2) % 2);
  cbox(GColorMagenta, bx, by, 1, 1);
  cbox(GColorShockingPink, bx - 1, by - 1, 1, 1);
  cbox(GColorShockingPink, bx + 1, by - 1, 1, 1);
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
  // table
  box(GColorWindsorTan, 6, 14, 24, 2);
  box(GColorWindsorTan, 7, 16, 2, 4);
  box(GColorWindsorTan, 27, 16, 2, 4);
  // plates and food
  box(GColorWhite, 11, 13, 5, 1);
  box(GColorChromeYellow, 12, 11, 3, 1);          // bun
  box(GColorWindsorTan, 12, 12, 3, 1);            // patty
  px(13, 10 - (int)(s_tick & 1), GColorLightGray);  // steam
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
  switch (pick_activity(s_hour, s_min, s_wday)) {
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

static void tap_handler(AccelAxisType axis, int32_t direction) {
  s_species = (s_species + 1) % NUM_SPECIES;
  load_species(s_species);
  persist_write_int(PERSIST_SPECIES, s_species);
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

  s_species = persist_exists(PERSIST_SPECIES) ? persist_read_int(PERSIST_SPECIES) % NUM_SPECIES : 0;
  load_species(s_species);
  update_time();
  update_steps();
  s_batt = battery_state_service_peek().charge_percent;

  // One tick per second drives the 1 fps "Tamagotchi" animation.
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  accel_tap_service_subscribe(tap_handler);
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
#endif
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
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
