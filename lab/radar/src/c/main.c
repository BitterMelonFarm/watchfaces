// Radar - an air-traffic-control scope. The sweep arm turns once a minute (6 degrees per second) trailing
// a fading phosphor wedge; targets drift slowly through the day and light up as the sweep crosses them,
// then fade. Freshly painted blips carry a callsign tag.
//
// Wrist tap: cycle the phosphor colour (green / amber / ice blue).
#define FACE_SECONDS 1
#include "../../../common/face.h"

#define CX 130
#define CY 130
#define RR_ 118
#define NBLIPS 14

typedef struct { GColor c[5]; GColor ring, tick; } Pal;
static Pal PL;

static void load_pal(void) {
  switch (s_aux) {
    default:
    case 0:
      PL = (Pal){{GColorGreen, GColorIslamicGreen, GColorDarkGreen, GColorDarkGreen, GColorBlack}, GColorDarkGreen, GColorIslamicGreen};
      break;
    case 1:
      PL = (Pal){{GColorChromeYellow, GColorOrange, GColorWindsorTan, GColorBulgarianRose, GColorBlack}, GColorBulgarianRose, GColorWindsorTan};
      break;
    case 2:
      PL = (Pal){{GColorCyan, GColorVividCerulean, GColorCobaltBlue, GColorOxfordBlue, GColorBlack}, GColorOxfordBlue, GColorCobaltBlue};
      break;
  }
}
static void face_init(void) { s_aux %= 3; load_pal(); }
static void face_tap(void) { s_aux = (s_aux + 1) % 3; load_pal(); }

static void blip_pos(int i, int *x, int *y, int *ang) {
  int mod = s_hour * 60 + s_min;
  uint32_t h = hash2((uint32_t)i, 77);
  int a0 = (int)(h % 360), r0 = 30 + (int)((h >> 9) % 70);
  int da = 1 + (int)((h >> 17) % 3);                          // degrees of azimuth drift per 10 minutes
  int a = (a0 + (mod / 10) * da * ((h >> 5) & 1 ? 1 : -1) + 3600) % 360;
  int r = r0 + 10 * isin(mod * 2 + (int)(h % 360)) / 1024;
  clock_pt(CX, CY, r, a, x, y);
  *ang = a;
}

static void wedge(int a0, int a1, GColor c) {
  int x0, y0, x1, y1;
  clock_pt(CX, CY, RR_, a0, &x0, &y0);
  clock_pt(CX, CY, RR_, a1, &x1, &y1);
  tri(c, CX, CY, x0, y0, x1, y1);
}

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
static const char *AIR[8] = {"AC", "UA", "BA", "LH", "DL", "AF", "KL", "SQ"};

static void face_draw(void) {
  int sweep = (s_sec * 6) % 360;
  // phosphor wedge: 10 sectors of 6 degrees behind the sweep, from dim to bright
  for (int k = 9; k >= 0; k--) {
    int shade = k < 2 ? 1 : (k < 5 ? 2 : (k < 8 ? 3 : 4));
    int a1 = sweep - k * 6, a0 = a1 - 6;
    wedge(a0, a1, PL.c[shade]);
  }
  // range rings, cross-hairs and azimuth ticks
  for (int r = 30; r <= 90; r += 30) ring(PL.ring, CX, CY, r + 1, r);
  ring(PL.tick, CX, CY, RR_ + 1, RR_);
  R(PL.ring, CX - RR_, CY, 2 * RR_ + 1, 1);
  R(PL.ring, CX, CY - RR_, 1, 2 * RR_ + 1);
  for (int a = 0; a < 360; a += 10) {
    int x0, y0, x1, y1, len = (a % 30 == 0) ? 9 : 5;
    clock_pt(CX, CY, RR_, a, &x0, &y0);
    clock_pt(CX, CY, RR_ - len, a, &x1, &y1);
    line(a % 90 == 0 ? PL.c[0] : PL.tick, x0, y0, x1, y1, 1);
  }
  // the sweep arm
  int sx, sy;
  clock_pt(CX, CY, RR_, sweep, &sx, &sy);
  line(PL.c[0], CX, CY, sx, sy, 2);
  // targets: brightness depends on how long ago the sweep painted them
  for (int i = 0; i < NBLIPS; i++) {
    int x, y, ang;
    blip_pos(i, &x, &y, &ang);
    int delta = ((sweep - ang) % 360 + 360) % 360;
    if (delta >= 66) { PX(PL.c[3], x, y); continue; }
    GColor c = delta < 8 ? GColorWhite : (delta < 24 ? PL.c[0] : (delta < 40 ? PL.c[1] : (delta < 54 ? PL.c[2] : PL.c[3])));
    R(c, x - 1, y - 1, 3, 3);
    if (delta < 30) { R(c, x - 3, y, 7, 1); R(c, x, y - 3, 1, 7); }
    if (delta < 40) {                                       // data tag with callsign
      uint32_t h = hash2((uint32_t)i, 5);
      char buf[12];
      snprintf(buf, sizeof(buf), "%s%03d", AIR[h % 8], (int)(100 + (h >> 8) % 900));
      int tx = x + 6, ty = y - 8;
      if (tx + 22 > 236) tx = x - 28;
      micro(buf, tx, ty, 1, c);
      line(PL.c[2], x, y, tx, ty + 5, 1);
    }
  }
  // own-ship marker
  R(PL.c[0], CX - 3, CY, 7, 1);  R(PL.c[0], CX, CY - 3, 1, 7);
  // top plate: time
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(PL.c[1], 62, 18, 136, 40, 6);
  RR(GColorBlack, 64, 20, 132, 36, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 22, 3, PL.c[0], PL.c[2]);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(PL.c[0], 128, 30, 3, 3); R(PL.c[0], 128, 40, 3, 3); }
  // bottom plate: date, battery, scan counter
  RR(PL.c[1], 70, 204, 120, 38, 6);
  RR(GColorBlack, 72, 206, 116, 34, 5);
  char buf[32];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 211, 2, PL.c[0]);
  snprintf(buf, sizeof(buf), "AZ %03d  BAT %d%%", sweep, s_batt);
  micro_c(buf, 130, 227, 1, s_batt <= 20 ? GColorRed : PL.c[1]);
  // cardinal labels
  micro("E", 238, 127, 1, PL.c[0]);
  micro("W", 18, 127, 1, PL.c[0]);
}
