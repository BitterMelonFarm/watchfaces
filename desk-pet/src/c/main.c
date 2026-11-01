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
  ACT_BATH,
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
      s_sp = (Species){GColorMelon, GColorShockingPink, GColorRed, GColorMelon, EARS_NUB};
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
    case 5:  // baby Cthulhu: bat wings and a wiggly tentacle beard
      s_sp = (Species){GColorMayGreen, GColorIslamicGreen, GColorMelon, GColorIndigo, EARS_CTHULHU};
      break;
    case 6:  // shoggoth: purple goo with too many eyes
      s_sp = (Species){GColorPurple, GColorIndigo, GColorMagenta, GColorPurple, EARS_SHOGGOTH};
      break;
    case 7:  // deep one: teal fish-person with a crest, gills and tail fin
      s_sp = (Species){GColorTiffanyBlue, GColorCobaltBlue, GColorShockingPink, GColorTiffanyBlue, EARS_DEEP};
      break;
    case 8:  // great eye: a floating eyeball on tentacles
      s_sp = (Species){GColorWhite, GColorLightGray, GColorRed, GColorWhite, EARS_EYE};
      break;
  }
}

#define PET_BODY (s_sp.body)
#define PET_OUT  (s_sp.out)

static int s_bob;   // 1 = pet is standing and hops on odd frames

static void px(int x, int y, GColor c) { box(c, x, y, 1, 1); }

// The grown-up pet: 8 wide, 8 tall plus ears/extras around it.
// mouth: 0 = closed, 1 = open (chewing / talking)
static void pet(int x, int y, bool blink, int mouth) {
  if (s_bob && (s_tick & 1)) y -= 1;               // little hop
  if (s_sp.ears == EARS_EYE && (s_tick & 2)) y -= 1;   // the eye bobs in the air
  GColor out = s_sp.out, body = s_sp.body;
  // ears / bumps (drawn first so the head overlaps them)
  switch (s_sp.ears) {
    case EARS_CAT:
      box(out, x + 1, y - 1, 2, 1);  px(x + 1, y - 2, out);
      box(out, x + 5, y - 1, 2, 1);  px(x + 6, y - 2, out);
      break;
    case EARS_BUNNY:
      box(out, x + 1, y - 3, 2, 3);  px(x + 2, y - 2, s_sp.blush);
      box(out, x + 5, y - 3, 2, 3);  px(x + 5, y - 2, s_sp.blush);
      break;
    case EARS_BEAR:
      box(out, x, y - 1, 2, 2);
      box(out, x + 6, y - 1, 2, 2);
      break;
    case EARS_FROG:
      box(out, x + 1, y - 1, 2, 1);
      box(out, x + 5, y - 1, 2, 1);
      break;
    case EARS_CTHULHU: {
      int fl = (int)(s_tick & 1);                      // flapping bat wings
      box(s_sp.snout, x - 2, y + 1 + fl, 2, 3);      // wings use the 'snout' colour
      box(s_sp.snout, x + 8, y + 1 + fl, 2, 3);
      px(x - 1, y + fl, s_sp.snout);
      px(x + 8, y + fl, s_sp.snout);
      px(x + 2, y - 1, out);                           // tiny horns
      px(x + 5, y - 1, out);
      break;
    }
    case EARS_SHOGGOTH:
      box(out, x + 2, y - 1, 2, 1);                    // goo lumps
      px(x + 5, y - 1, out);
      px(x + 6, y - 2 + (int)(s_tick & 1), out);
      break;
    case EARS_DEEP:
      px(x + 2, y - 1, out);                           // dorsal crest
      box(out, x + 3, y - 2, 2, 1);
      px(x + 5, y - 1, out);
      break;
    case EARS_EYE:
      break;
    default:
      px(x + 1, y - 1, out);
      px(x + 6, y - 1, out);
      break;
  }
  // body: outline, then fill
  box(out, x + 1, y, 6, 1);
  box(out, x, y + 1, 8, 5);
  box(out, x + 1, y + 6, 6, 1);
  box(body, x + 1, y + 1, 6, 5);
  box(body, x + 2, y + 6, 4, 1);
  // feet (the eldritch ones get tentacles/drips instead)
  if (s_sp.ears < EARS_CTHULHU || s_sp.ears == EARS_DEEP) {
    box(out, x + 1, y + 7, 2, 1);
    box(out, x + 5, y + 7, 2, 1);
  } else if (s_sp.ears == EARS_SHOGGOTH) {
    for (int i = 1; i <= 6; i++) {
      if (((int)s_tick + i) % 3 != 0) px(x + i, y + 7, out);   // dripping goo
    }
  } else if (s_sp.ears == EARS_EYE) {
    static const int tx[4] = {1, 3, 4, 6};
    for (int i = 0; i < 4; i++) {
      if (((int)s_tick + i) & 1) px(x + tx[i], y + 7, out);    // dangling tendrils
      px(x + tx[i], y + 6, out);
    }
  }
  // extras: whiskers + wagging tail, bunny pom-pom, bear snout
  if (s_sp.ears == EARS_CAT) {
    px(x - 1, y + 3, GColorDarkGray);  px(x - 1, y + 5, GColorDarkGray);
    px(x + 8, y + 3, GColorDarkGray);  px(x + 8, y + 5, GColorDarkGray);
    box(out, x + 8, y + 6, 1, 2);
    px(x + 9, y + 5 + (int)(s_tick & 1), out);
  } else if (s_sp.ears == EARS_BUNNY) {
    px(x - 1, y + 6, GColorWhite);
    px(x - 1, y + 5, GColorWhite);
  } else if (s_sp.ears == EARS_BEAR) {
    box(s_sp.snout, x + 2, y + 4, 4, 2);
  } else if (s_sp.ears == EARS_SHOGGOTH) {
    // extra eyes blinking all over the goo
    if (((int)s_tick + 0) % 4 != 0) px(x + 3, y + 1, GColorYellow);
    if (((int)s_tick + 1) % 4 != 0) px(x + 6, y + 1, GColorGreen);
    if (((int)s_tick + 2) % 4 != 0) px(x, y + 3, GColorYellow);
    if (((int)s_tick + 3) % 4 != 0) px(x + 7, y + 4, GColorGreen);
    if (((int)s_tick + 1) % 5 != 0) px(x + 4, y + 6, GColorYellow);
  } else if (s_sp.ears == EARS_DEEP) {
    // tail fin on the right and gill slits on the cheeks
    box(out, x + 8, y + 5, 1, 3);
    px(x + 9, y + 4, out);
    px(x + 9, y + 7, out);
    px(x + 8, y + 3, out);
  }
  // big shiny eyes (2x2 each) or sleepy blink
  if (s_sp.ears == EARS_EYE) {
    // one giant eyeball: iris, slit pupil, shine; blinks with an eyelid
    if (blink) {
      box(body, x + 2, y + 2, 4, 3);
      box(GColorBlack, x + 2, y + 4, 4, 1);
    } else {
      box(GColorMagenta, x + 2, y + 2, 4, 4);
      box(GColorBlack, x + 3, y + 2, 2, 4);
      px(x + 2, y + 2, GColorWhite);
      px(x + 3 + (int)((s_tick / 3) % 2), y + 3, GColorWhite);
    }
  } else if (s_sp.ears == EARS_DEEP && !blink && s_mood != MOOD_HAPPY) {
    // big bulging fish eyes looking to the side
    box(GColorWhite, x + 1, y + 2, 2, 2);
    box(GColorWhite, x + 5, y + 2, 2, 2);
    px(x + 2, y + 3, GColorBlack);
    px(x + 6, y + 3, GColorBlack);
  } else if (blink) {
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
  if (s_sp.ears == EARS_EYE) {
    px(x + 1, y + 2, GColorRed);                    // bloodshot veins
    px(x + 6, y + 5, GColorRed);
    px(x + 1, y + 5, GColorMelon);
  } else if (s_sp.ears == EARS_DEEP) {
    px(x + 1, y + 4, out);                          // gill slits
    px(x + 1, y + 5, out);
    px(x + 6, y + 4, out);
    px(x + 6, y + 5, out);
  } else if (s_sp.ears != EARS_BEAR) {
    px(x + 1, y + 4, s_sp.blush);
    px(x + 6, y + 4, s_sp.blush);
  }
  // mouth
  if (s_sp.ears == EARS_EYE) {
    // an eyeball has no mouth
  } else if (mouth) {
    box(GColorRed, x + 3, y + 4, 2, 2);
  } else if (s_mood == MOOD_SAD) {
    box(GColorBlack, x + 3, y + 4, 2, 1);          // frown
    px(x + 2, y + 5, GColorBlack);
    px(x + 5, y + 5, GColorBlack);
  } else if (s_mood == MOOD_HAPPY) {
    px(x + 2, y + 4, GColorBlack);                 // big smile
    px(x + 5, y + 4, GColorBlack);
    box(GColorBlack, x + 3, y + 5, 2, 1);
  } else if (s_sp.ears == EARS_FROG) {
    box(GColorBlack, x + 2, y + 5, 4, 1);          // wide froggy grin
  } else if (s_sp.ears == EARS_SHOGGOTH) {
    box(GColorBlack, x + 2, y + 5, 4, 1);          // toothy grin
    px(x + 3, y + 5, GColorWhite);
    px(x + 5, y + 5, GColorWhite);
  } else if (s_sp.ears == EARS_DEEP) {
    box(GColorShockingPink, x + 3, y + 5, 2, 1);   // pouty fish lips
  } else {
    px(x + 3, y + 4, GColorBlack);                 // tiny "w" mouth / nose
    px(x + 4, y + 4, GColorBlack);
  }
  if (s_sp.ears == EARS_CTHULHU) {
    // wiggling tentacle beard that also serves as feet
    int w = (int)(s_tick & 1);
    box(out, x + 2, y + 5, 1, 2 + w);
    box(out, x + 3, y + 5, 1, 3 - w);
    box(out, x + 4, y + 5, 1, 2 + w);
    box(out, x + 5, y + 5, 1, 3 - w);
  }
  if (s_mood == MOOD_SAD) {
    box(GColorPictonBlue, x + 2, y + 4, 1, 1 + (int)(s_tick & 1));   // tear
    // little rain cloud overhead
    int cy = y - (s_sp.ears == EARS_BUNNY ? 7 : 4);
    box(GColorLightGray, x + 1, cy, 6, 2);
    box(GColorLightGray, x + 2, cy - 1, 3, 1);
    px(x + 2 + 2 * (int)(s_tick & 1), cy + 2, GColorPictonBlue);
    px(x + 5 - 2 * (int)(s_tick & 1), cy + 2, GColorPictonBlue);
  } else if (s_mood == MOOD_HAPPY) {
    // twinkling sparkles
    int sx = (s_tick & 1) ? x + 8 : x - 1;
    int sy = y - (s_sp.ears == EARS_BUNNY ? 6 : 5);
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

// A small cloud drifting across a window (x0..x0+w-1 at row y0).
static void cloud(int x0, int y0, int w) {
  int cx = x0 + (int)((s_tick / 3) % (uint32_t)(w - 1));
  box(GColorWhite, cx, y0, 2, 1);
  px(cx + 1, y0 - 1, GColorWhite);
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
  cloud(3, 4, 7);
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

static void scene_bath(void) {
  room(GColorCeleste, GColorLightGray);
  for (int i = 4; i < SCENE_W; i += 4) box(GColorBabyBlueEyes, i, 0, 1, FLOOR_Y);
  for (int j = 4; j < FLOOR_Y; j += 4) box(GColorBabyBlueEyes, 0, j, SCENE_W, 1);
  // steam
  px(8 + (int)(s_tick % 3), 4, GColorWhite);
  px(20 - (int)(s_tick % 3), 3, GColorWhite);
  // pet in the tub
  pet(12, 6, (s_tick % 7) == 0, 0);
  box(GColorLightGray, 3, 11, 26, 1);
  box(GColorWhite, 4, 12, 24, 7);
  box(GColorPictonBlue, 5, 12, 22, 1);
  box(GColorBabyBlueEyes, 5, 13, 22, 4);
  box(GColorLightGray, 6, 19, 3, 1);                      // tub feet
  box(GColorLightGray, 23, 19, 3, 1);
  // bubbles
  for (int i = 0; i < 5; i++) {
    int bx = 6 + i * 5 + (int)((s_tick / 2 + i) % 2);
    int by = 11 - (int)((s_tick + i) % 3);
    if (i == 2) continue;                                   // leave room for the pet
    px(bx, by, GColorWhite);
    box(GColorWhite, bx + 1, 12, 2, 1);
  }
  // rubber duck bobbing
  int bobd = (int)(s_tick & 1);
  box(GColorYellow, 22, 10 + bobd, 3, 2);
  px(23, 9 + bobd, GColorYellow);
  px(25, 10 + bobd, GColorOrange);
  px(23, 9 + bobd, GColorYellow);
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
  box(GColorWhite, 12, 2, 8, 7);
  box(GColorCeleste, 13, 3, 6, 5);
  cloud(13, 5, 5);
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
  box(GColorWhite, 12, 2, 8, 7);
  box(GColorCeleste, 13, 3, 6, 5);
  cloud(13, 5, 5);
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
  if (hour == 20) return ACT_BATH;
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
  U = w / 43;
  if (U < 1) U = 1;
  OX = (w - SCENE_W * U) / 2;
  OY = h * 74 / 260;

  graphics_context_set_fill_color(ctx, GColorShockingPink);
  graphics_fill_rect(ctx, GRect(OX - U, OY - U, (SCENE_W + 2) * U, (SCENE_H + 2) * U),
                     3 * U, GCornersAll);

  s_bob = 0;
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
    case ACT_BATH:   scene_bath();        break;
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
