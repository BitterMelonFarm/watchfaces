// Periodic Time - hour, minute, seconds and day-of-month shown as real element tiles
// (atomic number, symbol, name, mass), coloured by element family. 12:34 is Mg (24.305) and Se (78.971).
//
// Wrist tap: swap the seconds tile for a STEPS element - your steps today map onto atomic number
// 0..59, reaching element 59 (praseodymium) at the 8,000-step goal.
#define FACE_SECONDS 1
#include "../../../common/face.h"

static const char *SYM[60] = {
  "NT", "H", "HE", "LI", "BE", "B", "C", "N", "O", "F",
  "NE", "NA", "MG", "AL", "SI", "P", "S", "CL", "AR", "K",
  "CA", "SC", "TI", "V", "CR", "MN", "FE", "CO", "NI", "CU",
  "ZN", "GA", "GE", "AS", "SE", "BR", "KR", "RB", "SR", "Y",
  "ZR", "NB", "MO", "TC", "RU", "RH", "PD", "AG", "CD", "IN",
  "SN", "SB", "TE", "I", "XE", "CS", "BA", "LA", "CE", "PR",
};
static const char *NAME[60] = {
  "NEUTRON", "HYDROGEN", "HELIUM", "LITHIUM", "BERYLLIUM", "BORON", "CARBON", "NITROGEN", "OXYGEN", "FLUORINE",
  "NEON", "SODIUM", "MAGNESIUM", "ALUMINIUM", "SILICON", "PHOSPHORUS", "SULFUR", "CHLORINE", "ARGON", "POTASSIUM",
  "CALCIUM", "SCANDIUM", "TITANIUM", "VANADIUM", "CHROMIUM", "MANGANESE", "IRON", "COBALT", "NICKEL", "COPPER",
  "ZINC", "GALLIUM", "GERMANIUM", "ARSENIC", "SELENIUM", "BROMINE", "KRYPTON", "RUBIDIUM", "STRONTIUM", "YTTRIUM",
  "ZIRCONIUM", "NIOBIUM", "MOLYBDENUM", "TECHNETIUM", "RUTHENIUM", "RHODIUM", "PALLADIUM", "SILVER", "CADMIUM", "INDIUM",
  "TIN", "ANTIMONY", "TELLURIUM", "IODINE", "XENON", "CAESIUM", "BARIUM", "LANTHANUM", "CERIUM", "PRASEODYMIUM",
};
static const char *MASS[60] = {
  "1.009", "1.008", "4.003", "6.94", "9.012", "10.81", "12.011", "14.007", "15.999", "18.998",
  "20.180", "22.990", "24.305", "26.982", "28.085", "30.974", "32.06", "35.45", "39.948", "39.098",
  "40.078", "44.956", "47.867", "50.942", "51.996", "54.938", "55.845", "58.933", "58.693", "63.546",
  "65.38", "69.723", "72.630", "74.922", "78.971", "79.904", "83.798", "85.468", "87.62", "88.906",
  "91.224", "92.906", "95.95", "98", "101.07", "102.91", "106.42", "107.87", "112.41", "114.82",
  "118.71", "121.76", "127.60", "126.90", "131.29", "132.91", "137.33", "138.91", "140.12", "140.91",
};

static void face_init(void) {}
static void face_tap(void) { s_aux = (s_aux + 1) % 2; }

// element family: 0 neutron, 1 alkali, 2 alkaline earth, 3 transition, 4 post-transition,
// 5 metalloid, 6 nonmetal, 7 halogen, 8 noble gas, 9 lanthanide
static int family(int z) {
  switch (z) {
    case 0: return 0;
    case 1: case 6: case 7: case 8: case 15: case 16: case 34: return 6;
    case 2: case 10: case 18: case 36: case 54: return 8;
    case 3: case 11: case 19: case 37: case 55: return 1;
    case 4: case 12: case 20: case 38: case 56: return 2;
    case 5: case 14: case 32: case 33: case 51: case 52: return 5;
    case 9: case 17: case 35: case 53: return 7;
    case 13: case 31: case 49: case 50: return 4;
    case 57: case 58: case 59: return 9;
    default: return 3;
  }
}

static GColor fam_dim(int f) {
  static GColor t[10];
  t[0] = GColorDarkGray;   t[1] = GColorDarkCandyAppleRed; t[2] = GColorWindsorTan; t[3] = GColorCobaltBlue;
  t[4] = GColorJaegerGreen; t[5] = GColorBrass;           t[6] = GColorIslamicGreen; t[7] = GColorPurple;
  t[8] = GColorIndigo;     t[9] = GColorImperialPurple;
  return t[f];
}
static GColor fam_hi(int f) {
  static GColor t[10];
  t[0] = GColorWhite;      t[1] = GColorSunsetOrange; t[2] = GColorOrange;  t[3] = GColorPictonBlue;
  t[4] = GColorMediumAquamarine; t[5] = GColorYellow; t[6] = GColorGreen;  t[7] = GColorMagenta;
  t[8] = GColorElectricUltramarine; t[9] = GColorRichBrilliantLavender;
  return t[f];
}

// big tile: Z top-left, mass top-right, 2-letter symbol, name at the bottom
static void tile(int x, int y, int w, int h, int z, const char *label) {
  int f = family(z);
  RR(fam_hi(f), x, y, w, h, 6);
  RR(fam_dim(f), x + 2, y + 2, w - 4, h - 4, 5);
  char buf[12];
  snprintf(buf, sizeof(buf), "%d", z);
  micro(buf, x + 6, y + 6, 2, GColorWhite);
  micro_r(MASS[z], x + w - 6, y + 7, 1, GColorWhite);
  int sc = 7;
  micro_c(SYM[z], x + w / 2, y + 28, sc, GColorWhite);
  micro_c(NAME[z], x + w / 2, y + h - 15, 1, GColorWhite);
  micro_c(label, x + w / 2, y - 8, 1, GColorLightGray);
}

// small tile: Z, symbol, name
static void tile_small(int x, int y, int w, int h, int z, const char *label) {
  int f = family(z);
  RR(fam_hi(f), x, y, w, h, 5);
  RR(fam_dim(f), x + 2, y + 2, w - 4, h - 4, 4);
  char buf[12];
  snprintf(buf, sizeof(buf), "%d", z);
  micro(buf, x + 5, y + 5, 1, GColorWhite);
  micro_c(SYM[z], x + w / 2, y + 17, 5, GColorWhite);
  micro_c(NAME[z], x + w / 2, y + h - 11, 1, GColorWhite);
  micro_c(label, x + w / 2, y - 8, 1, GColorLightGray);
}

static const char *DAYS[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *MONS[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

static void face_draw(void) {
  // faint periodic-table backdrop: 18 columns of tiny cells
  for (int r = 0; r < 7; r++) {
    for (int c = 0; c < 18; c++) {
      if (((r * 18 + c) * 7 + 3) % 5 == 0) PX(GColorDarkGray, 24 + c * 12, 8 + r * 4);
    }
  }
  micro_c("PERIODIC TIME", 130, 22, 2, GColorCyan);
  int hz = s_hour;
  if (!s_24h) { hz %= 12; if (hz == 0) hz = 12; }
  tile(44, 48, 80, 94, hz, "HOUR");
  tile(136, 48, 80, 94, s_min, "MIN");
  R(GColorWhite, 128, 88, 4, 4);
  R(GColorWhite, 128, 102, 4, 4);
  int z2 = s_sec;
  const char *lab2 = "SEC";
  if (s_aux == 1) {
    z2 = s_steps * 59 / STEP_GOAL;
    if (z2 > 59) z2 = 59;
    lab2 = "STEPS";
  }
  tile_small(58, 158, 66, 56, z2, lab2);
  tile_small(136, 158, 66, 56, s_mday, "DAY");
  char buf[24];
  snprintf(buf, sizeof(buf), "%s %02d %s", DAYS[s_wday % 7], s_mday, MONS[s_mon % 12]);
  micro_c(buf, 130, 224, 2, GColorWhite);
  snprintf(buf, sizeof(buf), "BAT %d%%", s_batt);
  micro_c(buf, 130, 240, 1, s_batt <= 20 ? GColorRed : GColorLightGray);
}
