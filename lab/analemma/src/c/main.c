// Analemma - the figure-8 the Sun traces if you photograph it at the same clock time every day for a year.
// Horizontal = equation of time (how far the Sun runs ahead of / behind the clock, +-15 minutes),
// vertical = solar declination (+-23.44 degrees). Both come from the standard approximations for the day of
// the year. Today's position is the glowing Sun; month starts are marked along the curve.
//
// Wrist tap: switch between the season-coloured curve and a mono "photographic plate".
#define FACE_SECONDS 0
#include "../../../common/face.h"

#define ACX 130
#define ACY 108
#define EOT_SCALE 5          // px per minute
#define DEC_SCALE 36         // tenths of a pixel per... see dec_px()

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static int day_of_year(int mon, int mday, int year) {
  static const int cum[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  int n = cum[mon % 12] + mday;
  bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  if (leap && mon >= 2) n++;
  return n;
}

// equation of time in hundredths of a minute, declination in hundredths of a degree
static int eot100(int n) {
  int B = (n - 81) * 360 / 365;
  return (987 * isin(2 * B) - 753 * icos(B) - 150 * isin(B)) / 1024;
}
static int dec100(int n) {
  int B = (n - 81) * 360 / 365;
  return 2344 * isin(B) / 1024;
}
static void apos(int n, int *x, int *y) {
  *x = ACX + eot100(n) * EOT_SCALE / 100;
  *y = ACY - dec100(n) * 33 / 1000;                 // ~3.3 px per degree
}

static GColor season(int n) {
  if (s_aux == 1) return GColorWhite;
  if (n < 80 || n >= 355) return GColorVividCerulean;     // winter
  if (n < 172) return GColorGreen;                        // spring
  if (n < 266) return GColorYellow;                       // summer
  return GColorOrange;                                    // autumn
}

static const char *MN[12] = {"J", "F", "M", "A", "M", "J", "J", "A", "S", "O", "N", "D"};
static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void face_draw(void) {
  // graticule: declination every 10 degrees, equation of time every 5 minutes
  for (int d = -20; d <= 20; d += 10) {
    int y = ACY - d * 33 / 10;
    R(d == 0 ? GColorLightGray : GColorDarkGray, ACX - 100, y, 201, 1);
    char b[8];
    snprintf(b, sizeof(b), "%+d", d);
    micro(b, ACX + 86, y - 3, 1, GColorLightGray);
  }
  for (int m = -15; m <= 15; m += 5) R(m == 0 ? GColorLightGray : GColorDarkGray, ACX + m * EOT_SCALE, ACY - 86, 1, 173);
  micro_c("EQUATION OF TIME (MIN)", 130, ACY + 92, 1, GColorLightGray);
  micro_c("ANALEMMA", 130, 12, 2, GColorCyan);
  // the figure-8 for the whole year
  int n_today = day_of_year(s_mon, s_mday, s_year);
  int px = 0, py = 0;
  for (int n = 1; n <= 366; n++) {
    int x, y;
    apos(n, &x, &y);
    if (n > 1) line(season(n), px, py, x, y, 2);
    px = x; py = y;
  }
  // month markers
  static const int cum[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
  for (int m = 0; m < 12; m++) {
    int x, y;
    apos(cum[m] + 1, &x, &y);
    disc(GColorWhite, x, y, 2);
    int dx = x - ACX, dy = y - ACY;
    int lx = x + (dx >= 0 ? 7 : -7), ly = y + (dy >= 0 ? 4 : -8);
    micro(MN[m], lx - 1, ly, 1, GColorLightGray);
  }
  // today's Sun
  int sx, sy;
  apos(n_today, &sx, &sy);
  disc(GColorBulgarianRose, sx, sy, 9);
  disc(GColorOrange, sx, sy, 7);
  disc(GColorYellow, sx, sy, 5);
  disc(GColorPastelYellow, sx - 1, sy - 1, 2);
  // readouts
  char buf[32];
  int e = eot100(n_today), d = dec100(n_today);
  snprintf(buf, sizeof(buf), "DEC %c%d.%d", d < 0 ? '-' : '+', iabs(d) / 100, (iabs(d) % 100) / 10);
  micro(buf, 34, 76, 1, GColorYellow);
  snprintf(buf, sizeof(buf), "EOT %c%d.%d M", e < 0 ? '-' : '+', iabs(e) / 100, (iabs(e) % 100) / 10);
  micro(buf, 34, 84, 1, GColorYellow);
  snprintf(buf, sizeof(buf), "DAY %d", n_today);
  micro(buf, 34, 92, 1, GColorLightGray);
  // time plate
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  RR(GColorDarkGray, 62, 207, 136, 38, 6);
  RR(GColorBlack, 64, 209, 132, 34, 5);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {77, 105, 138, 166};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    tube_digit(dg[i], xs[i], 211, 3, GColorYellow, GColorWindsorTan);
  }
  R(GColorYellow, 128, 219, 3, 3); R(GColorYellow, 128, 229, 3, 3);
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro(buf, 38, 247, 1, GColorLightGray);
}
