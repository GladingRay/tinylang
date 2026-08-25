#include <stdio.h>
#include <stdlib.h>

extern long _t3Fib3FIB(long);
extern void _t3Fib7Compute(void);
extern void _t3Fib3Run(void);

/* Reference implementation used to verify the code generated from
 * example/Fib.mod. */
static long fib(long n) {
  return n < 2 ? n : fib(n - 1) + fib(n - 2);
}

int main(int argc, char *argv[]) {
  long n = argc > 1 ? strtol(argv[1], NULL, 10) : 15;
  if (n < 0) {
    fprintf(stderr, "usage: %s [<n>]\n", argv[0]);
    return 2;
  }

  int failed = 0;
  for (long i = 0; i <= n; ++i) {
    long tinylang = _t3Fib3FIB(i);
    long reference = fib(i);
    printf("fib(%ld) = %ld  %s\n", i, tinylang,
           tinylang == reference ? "OK" : "MISMATCH");
    if (tinylang != reference)
      failed = 1;
  }

  /* Exercise statement-form procedure calls (void). */
  _t3Fib3Run();
  printf("Run() -> Compute() done\n");
  return failed;
}
