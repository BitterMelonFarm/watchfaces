// Tide Rail - a calm, shallow sea with a little steam train crossing a causeway (original art, in the spirit of
// gentle pastoral animation). The train is the minute hand: it creeps across the sea once an hour. The sky
// follows the real time of day, the sun or moon throws a glittering path on the water, the causeway lamps show
// your battery, and a buoy floats up for every 1,000 steps. The hour is on a little station board.
//
// Wrist tap: calm sea <-> choppy sea (more wave motion).
#define FACE_SECONDS 1
#include "../../../../lab/common/face.h"
#include "../../../../lab/common/paint.h"

#define HORIZON 128
#define TRACK_Y 176

static void face_init(void) { s_aux %= 2; }
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void train(int x, int y, bool lit, GColor body, GColor roof, GColor smoke) {
  // coaches (left), then the locomotive (right, facing right)
  for (int c = 0; c < 2; c++) {
    int cx = x + c * 30;
    RR(body, cx, y - 20, 27, 18, 3);
    R(roof, cx, y - 22, 27, 4);
    for (int w = 0; w < 3; w++) R(lit ? GColorYellow : GColorCeleste, cx + 3 + w * 8, y - 16, 5, 7);
    R(GColorBlack, cx + 13, y - 3, 3, 1);                    // coupling
    disc(GColorBlack, cx + 6, y - 1, 3);  disc(GColorBlack, cx + 20, y - 1, 3);
  }
  int lx = x + 60;
  RR(body, lx, y - 17, 24, 15, 3);                            // boiler
  R(roof, lx + 14, y - 26, 10, 10);                           // cab
  R(lit ? GColorYellow : GColorCeleste, lx + 17, y - 23, 4, 5);
  R(GColorBlack, lx + 3, y - 24, 5, 8);                       // chimney
  R(GColorBlack, lx + 2, y - 25, 7, 2);
  disc(GColorBlack, lx + 5, y - 1, 3);  disc(GColorBlack, lx + 13, y - 1, 3);  disc(GColorBlack, lx + 21, y - 1, 3);
  R(GColorRed, lx + 22, y - 12, 3, 2);                        // buffer lamp
  for (int i = 0; i < 4; i++) {                               // smoke puffs drifting back over the coaches
    int sway = 2 * isin((int)s_tick * 60 + i * 80) / 1024;
    disc(i < 2 ? GColorWhite : smoke, lx + 5 - i * 11 + sway, y - 32 - i * 3, 3 + i);
  }
}

static void face_draw(void) {
  const int m = s_hour * 60 + s_min;
  const bool night = (m >= 1260 || m < 300);
  const bool glow = night || m >= 1100;
  Sky k = sky_for(m);
  // sky and sea
  gradient(0, 70, k.top, k.mid);
  gradient(70, HORIZON, k.mid, k.hor);
  GColor deep = night ? GColorOxfordBlue : (m >= 1020 ? GColorIndigo : GColorTiffanyBlue);
  GColor shallow = night ? GColorDukeBlue : (m >= 1020 ? GColorPurpureus : GColorCadetBlue);
  gradient(HORIZON, 205, k.hor, shallow);
  gradient(205, 260, shallow, deep);
  // sun / moon and its glittering path on the water
  int sx, sy;
  if (!night) {
    int p = (m - 360) * 180 / 750;  p = p < 0 ? 0 : (p > 180 ? 180 : p);
    math_pt(130, HORIZON + 10, 54, 180 - p, &sx, &sy);
    disc(m >= 1020 ? GColorOrange : GColorChromeYellow, sx, sy, 15);
    disc(m >= 1020 ? GColorChromeYellow : GColorPastelYellow, sx, sy, 11);
  } else {
    int mm = (m >= 1260 ? m - 1260 : m + 180);
    math_pt(130, HORIZON + 10, 54, 180 - mm * 180 / 540, &sx, &sy);
    disc(GColorBabyBlueEyes, sx, sy, 12);  disc(GColorPastelYellow, sx, sy, 10);
  }
  for (int y = HORIZON + 2; y < 205; y += 3) {                // glitter column under the sun/moon
    int w = 3 + (y - HORIZON) / 4;
    int off = (int)((hash2((uint32_t)y, s_tick / 2) % 9)) - 4;
    R(night ? GColorBabyBlueEyes : GColorPastelYellow, sx - w / 2 + off, y, w, 1);
  }
  // low horizontal clouds drifting
  for (int i = 0; i < 3; i++) {
    int x = ((i * 120 + m) % 340) - 60;
    int y = 70 + i * 17;
    RR(k.cloud_body, x, y, 70, 7, 3);  RR(k.cloud_lit, x + 8, y - 3, 40, 6, 3);  R(k.cloud_shade, x + 4, y + 5, 62, 2);
  }
  // wave dashes
  for (int i = 0; i < 30; i++) {
    int y = HORIZON + 8 + (int)(hash2((uint32_t)i, 2) % 90);
    int x = ((int)(hash2((uint32_t)i, 3) % 280) + (int)(s_tick * (s_aux ? 5 : 2))) % 280 - 10;
    R(k.cloud_lit, x, y, 6 + (y - HORIZON) / 12, 1);
  }
  // causeway: posts down into the water, a track and sleepers
  for (int x = 6; x < 260; x += 24) {
    R(GColorWindsorTan, x, TRACK_Y + 4, 3, 24);
    R(shallow, x - 1, TRACK_Y + 30, 5, 10);                   // splash at the waterline
  }
  R(GColorDarkGray, 0, TRACK_Y + 2, SW_, 4);
  R(GColorLightGray, 0, TRACK_Y + 2, SW_, 1);
  for (int x = 2; x < 260; x += 6) R(GColorWindsorTan, x, TRACK_Y + 6, 2, 3);
  // the train: position = minutes through the hour
  int secs = s_min * 60 + s_sec;
  int tx = -92 + secs * 350 / 3600;
  GColor body = night ? GColorCobaltBlue : (m >= 1020 ? GColorBulgarianRose : GColorTiffanyBlue);
  train(tx, TRACK_Y + 2, glow, body, GColorPastelYellow, k.cloud_body);
  // its reflection: a few darker, shortened rows
  for (int r = 0; r < 8; r++) R(night ? GColorOxfordBlue : GColorCobaltBlue, tx + (r & 1), TRACK_Y + 34 + r * 2, 82 - r * 3, 1);
  // causeway lamps = battery
  int lamps = (s_batt + 19) / 20;
  for (int i = 0; i < 5; i++) {
    int x = 40 + i * 44;
    R(GColorBlack, x, TRACK_Y - 18, 2, 20);
    if (i < lamps) { disc(GColorChromeYellow, x + 1, TRACK_Y - 20, 5);  disc(GColorYellow, x + 1, TRACK_Y - 20, 3); }
    else disc(GColorDarkGray, x + 1, TRACK_Y - 20, 3);
  }
  // buoys = steps
  int buoys = s_steps / 1000 > 8 ? 8 : s_steps / 1000;
  for (int i = 0; i < 8; i++) {
    int x = 22 + i * 30 + (i & 1) * 6;
    int y = 222 + (int)((s_tick + (uint32_t)i) % 3) - 1 + (i & 1) * 8;
    if (i < buoys) { disc(GColorWhite, x, y, 4); R(GColorRed, x - 4, y - 1, 9, 3); R(GColorWindsorTan, x, y - 8, 1, 4); }
    else { disc(shallow, x, y + 1, 2); }
  }
  // gulls by day
  if (!night) for (int i = 0; i < 2; i++) bird(70 + i * 80 + (int)((s_tick * (1 + (uint32_t)i)) % 18), 96 + i * 10, (int)((s_tick + (uint32_t)i) & 1) * 2 + 1, GColorWhite);
  // the hour on a little station board at the top
  int hour = s_hour;
  if (!s_24h) { hour %= 12; if (hour == 0) hour = 12; }
  plate(66, 20, 128, 54, 12, GColorTiffanyBlue, GColorPastelYellow, night ? GColorOxfordBlue : GColorCobaltBlue);
  RR(GColorDukeBlue, 72, 26, 116, 42, 8);
  int dg[4] = {hour / 10, hour % 10, s_min / 10, s_min % 10};
  const int xs[4] = {76, 98, 134, 156};
  for (int i = 0; i < 4; i++) {
    if (i == 0 && !s_24h && dg[0] == 0) continue;
    big_digit(dg[i], xs[i], 30, 4, GColorPastelYellow);
  }
  if (!(s_tick & 1) || !s_in_focus) { R(GColorPastelYellow, 128, 38, 4, 4); R(GColorPastelYellow, 128, 50, 4, 4); }
  char buf[28];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 64, 1, GColorCeleste);
}
