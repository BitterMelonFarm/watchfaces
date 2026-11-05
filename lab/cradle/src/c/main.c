// Newton's Cradle - five balls, and conservation of momentum and energy. Every second the cradle clicks:
// n balls on one side swing out, and (on the next second) n balls on the other side. The number of lifted
// balls is the minute modulo four plus one, so 1 ball in -> 1 ball out, 2 in -> 2 out... exactly what
// momentum and energy conservation demand.
//
// Wrist tap: steel / brass / neon balls.
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define NB 5
#define PIVY 46
#define LEN 108
#define BR 12
#define SWING 36          // degrees

static void face_init(void) { s_aux %= 3; }
static void face_tap(void) { s_aux = (s_aux + 1) % 3; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

static void ball(int x, int y, int style) {
  GColor body = style == 0 ? GColorLightGray : (style == 1 ? GColorChromeYellow : GColorCyan);
  GColor dark = style == 0 ? GColorDarkGray : (style == 1 ? GColorWindsorTan : GColorCobaltBlue);
  GColor lite = style == 0 ? GColorWhite : (style == 1 ? GColorPastelYellow : GColorWhite);
  disc(dark, x + 1, y + 1, BR);
  disc(body, x, y, BR);
  disc(lite, x - 4, y - 4, 4);
  disc(GColorWhite, x - 5, y - 5, 1);
}

static void face_draw(void) {
  int lifted = 1 + s_min % 4;
  bool left_phase = (s_sec & 1) == 0;               // even seconds: left balls are out
  // frame
  RR(GColorDarkGray, 24, PIVY - 8, 212, 6, 3);
  R(GColorLightGray, 24, PIVY - 8, 212, 2);
  R(GColorDarkGray, 30, PIVY - 8, 5, 130);  R(GColorDarkGray, 225, PIVY - 8, 5, 130);
  R(GColorLightGray, 30, PIVY - 8, 1, 130);  R(GColorLightGray, 225, PIVY - 8, 1, 130);
  // dotted swing arcs
  for (int side = -1; side <= 1; side += 2) {
    for (int a = 0; a <= SWING; a += 4) {
      int px = 130 + side * 2 * (2 * BR) , x, y;
      (void)px;
      x = 130 + side * 2 * (2 * BR) - side * ((LEN * isin(a)) / 1024);
      y = PIVY + (LEN * icos(a)) / 1024;
      PX(GColorDarkGray, x, y);
    }
  }
  // balls and strings
  for (int i = 0; i < NB; i++) {
    int pivx = 130 + (i - 2) * (2 * BR);
    int ang = 0;
    if (left_phase && i < lifted) ang = SWING;                       // swung out to the left
    if (!left_phase && i >= NB - lifted) ang = -SWING;               // swung out to the right
    int bx = pivx - (LEN * isin(ang)) / 1024;
    int by = PIVY + (LEN * icos(ang)) / 1024;
    // two strings per ball, like the real thing (V shape seen from the front is a single line here)
    line(GColorLightGray, pivx, PIVY - 4, bx, by, 1);
    ball(bx, by, s_aux);
  }
  // labels
  char buf[40];
  snprintf(buf, sizeof(buf), "%d IN -> %d OUT", lifted, lifted);
  micro_c("NEWTON'S CRADLE", 130, 12, 1, GColorLightGray);
  micro_c(buf, 130, 22, 1, GColorCyan);
  snprintf(buf, sizeof(buf), "P = %d MV   E = %d/2 MV2", lifted, lifted);
  micro_c(buf, 130, 188, 1, GColorLightGray);
  if (s_tick & 1) micro_c("CLICK", 130, 172, 1, GColorYellow);
  // time plate
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorDarkGray, 62, 207, 136, 38, 6);
  RR(GColorBlack, 64, 209, 128, 34, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  GColor tube = s_aux == 0 ? GColorWhite : (s_aux == 1 ? GColorChromeYellow : GColorCyan);
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, 3, tube, GColorDarkGray);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(tube, 128, 219, 3, 3); R(tube, 128, 229, 3, 3); }
  snprintf(buf, sizeof(buf), "%s %02d", DAYS[s_wday % 7], s_mday);
  micro(buf, 38, 247, 1, GColorLightGray);
}
