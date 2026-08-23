#include <stdio.h>
#include <stdlib.h>

extern long _t3Gcd3GCD(long, long);

/* Reference implementation used to verify the code generated from
 * example/Gcd.mod.  Kept in lockstep with the algorithm in Gcd.mod. */
static long gcd(long a, long b) {
  while (b != 0) {
    long t = a % b;
    a = b;
    b = t;
  }
  return a;
}

static void usage(const char *prog) {
  fprintf(stderr, "usage: %s <a1> <b1> [<a2> <b2> ...]\n", prog);
}

static long parse_long(const char *prog, const char *s) {
  char *end;
  long v = strtol(s, &end, 10);
  if (end == s || *end != '\0') {
    fprintf(stderr, "%s: invalid integer: %s\n", prog, s);
    exit(2);
  }
  return v;
}

int main(int argc, char *argv[]) {
  if (argc < 3 || ((argc - 1) & 1)) {
    usage(argv[0]);
    return 2;
  }

  int failed = 0;
  for (int i = 1; i < argc; i += 2) {
    long a = parse_long(argv[0], argv[i]);
    long b = parse_long(argv[0], argv[i + 1]);
    long tinylang = _t3Gcd3GCD(a, b);
    long builtin = gcd(a, b);
    if (tinylang == builtin) {
      printf("gcd(%ld, %ld) = %ld  OK\n", a, b, tinylang);
    } else {
      printf("gcd(%ld, %ld) = %ld (tinylang) vs %ld (reference)  MISMATCH\n",
             a, b, tinylang, builtin);
      failed = 1;
    }
  }
  return failed;
}
