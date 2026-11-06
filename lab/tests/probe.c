// Test probe: compiles a face's real source plus the host stubs, then prints values from its physics code.
//   gcc -DFACE_SRC='"../orrery/src/c/main.c"' -DPROBE_ORRERY probe.c   (see run_checks.py)
#define main harness_main
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
  return 0;
}
