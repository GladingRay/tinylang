#include <math.h>
#include <stdio.h>

/* REAL maps to C float: tinylang emits 32-bit IEEE single precision. */
extern void _t4Real4SetR(float);
extern float _t4Real4GetR(void);
extern float _t4Real3Add(float, float);
extern float _t4Real3Mul(float, float);
extern float _t4Real3Div(float, float);
extern float _t4Real3Neg(float);
extern float _t4Real3Max(float, float);
extern float _t4Real6HalfOf(float);
extern float _t4Real5Milli(float);
extern float _t4Real4Pow2(long);
extern float _t4Real9Quadratic(float, float, float, float);

static double close_to(double got, double expected) {
  return fabs(got - expected) <= 1e-6;
}

int main(void) {
  /* Reference values are computed independently in C. */
  struct {
    const char *name;
    double got;
    double expected;
  } checks[] = {
      {"global r via SetR/GetR", (_t4Real4SetR(42.5), _t4Real4GetR()), 42.5},
      {"Add(1.5, 2.25)", _t4Real3Add(1.5, 2.25), 3.75},
      {"Mul(1.5, -2.0)", _t4Real3Mul(1.5, -2.0), -3.0},
      {"Div(7.5, 2.0)", _t4Real3Div(7.5, 2.0), 3.75},
      {"Neg(-1.25)", _t4Real3Neg(-1.25), 1.25},
      {"Max(-1.0, 2.5)", _t4Real3Max(-1.0, 2.5), 2.5},
      {"Max(3.0, 3.0)", _t4Real3Max(3.0, 3.0), 3.0},
      {"HalfOf(9.0)", _t4Real6HalfOf(9.0), 4.5},
      {"Milli(250.0)", _t4Real5Milli(250.0), 0.25},
      {"Pow2(0)", _t4Real4Pow2(0), 1.0},
      {"Pow2(5)", _t4Real4Pow2(5), 32.0},
      {"Pow2(10)", _t4Real4Pow2(10), 1024.0},
      {"Quadratic(1.0, -3.0, 2.0, 4.0)", _t4Real9Quadratic(1.0, -3.0, 2.0, 4.0),
       6.0},
  };

  int failed = 0;
  for (unsigned i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i) {
    if (close_to(checks[i].got, checks[i].expected)) {
      printf("%s = %.6f  OK\n", checks[i].name, checks[i].got);
    } else {
      printf("%s = %.9f (tinylang) vs %.9f (reference)  MISMATCH\n",
             checks[i].name, checks[i].got, checks[i].expected);
      failed = 1;
    }
  }
  if (!failed)
    printf("all REAL checks OK\n");
  return failed;
}
