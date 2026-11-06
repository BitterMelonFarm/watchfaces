// Test probe: compiles a face's real source plus the host stubs, then prints values from its physics code.
//   gcc -DFACE_SRC='"../orrery/src/c/main.c"' -DPROBE_ORRERY probe.c   (see run_checks.py)
#define main harness_main
#include <math.h>
#include "../tools/render.c"
#undef main

int main(int argc, char **argv) {
  if (argc < 7) return 1;
  s_year = atoi(argv[1]); s_mon = atoi(argv[2]) - 1; s_mday = atoi(argv[3]);
  s_hour = atoi(argv[4]); s_min = atoi(argv[5]); s_sec = atoi(argv[6]);
#ifdef PROBE_ORRERY
  printf("%d %d %d %d %d %d %d\n", helio_long(0), helio_long(1), helio_long(2), helio_long(3), helio_long(4), helio_long(5), moon_phase_deg());
#endif
#ifdef PROBE_ANALEMMA
  int n = day_of_year(s_mon, s_mday, s_year);
  printf("%d %d %d\n", n, eot100(n), dec100(n));
#endif
#ifdef PROBE_KEPLER
  // argv[7] = eccentricity x1000, argv[8] = mean anomaly in milli-radians
  float e = atoi(argv[7]) / 1000.0f, m = atoi(argv[8]) / 1000.0f;
  float E = solve_kepler(m, e);
  printf("%d\n", (int)((E - e * fsin(E) - m) * 1000000.0f));
#endif
#ifdef PROBE_CORE
  printf("%lld\n", (long long)secs_since_j2000());
#endif
#ifdef PROBE_BOHR
  printf("%d\n", lambda_nm(atoi(argv[7]), atoi(argv[8])));
#endif
#ifdef PROBE_CHAOS
  {
    // argv[7], argv[8]: initial angles in milli-radians; argv[9]: seconds to integrate
    St st = {atoi(argv[7]) / 1000.0f, atoi(argv[8]) / 1000.0f, 0.0f, 0.0f};
    #define ENERGY(S) ((double)((S).w1 * (S).w1 + 0.5 * (S).w2 * (S).w2 + (S).w1 * (S).w2 * cos((S).a1 - (S).a2)) - GRAV * (2 * cos((S).a1) + cos((S).a2)))
    double e0 = ENERGY(st);
    for (int i = 0; i < atoi(argv[9]) * STEPS_PER_SEC; i++) st = rk4(st);
    printf("%d %d\n", (int)(e0 * 1e6), (int)(ENERGY(st) * 1e6));
  }
#endif
  return 0;
}
