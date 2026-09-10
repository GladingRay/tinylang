#include <stdio.h>

extern void _t6Shapes5Setup(void);
extern long _t6Shapes10CircleArea(void);
extern long _t6Shapes10SquareArea(void);
extern long _t6Shapes8BlobArea(void);
extern long _t6Shapes10CircleKind(void);
extern long _t6Shapes10SquareKind(void);
extern long _t6Shapes8BlobKind(void);
extern long _t6Shapes14CircleTypeCode(void);
extern long _t6Shapes12BlobTypeCode(void);
extern long _t6Shapes10CoordTotal(void);
extern long _t6Shapes16DirectCircleKind(void);
extern long _t6Shapes15LocalCircleArea(void);
extern long _t6Shapes16CopiedCircleArea(void);

int main(void) {
  _t6Shapes5Setup();

  struct {
    const char *name;
    long got;
    long expected;
  } checks[] = {
      /* Polymorphic calls: Circle/Square override Area and Kind, Blob only
         overrides Area and inherits Shape's Kind. */
      {"AreaOf(Circle)", _t6Shapes10CircleArea(), 75},
      {"AreaOf(Square)", _t6Shapes10SquareArea(), 36},
      {"AreaOf(Blob)", _t6Shapes8BlobArea(), 14},
      {"KindOf(Circle)", _t6Shapes10CircleKind(), 2},
      {"KindOf(Square)", _t6Shapes10SquareKind(), 3},
      {"KindOf(Blob) inherits Shape", _t6Shapes8BlobKind(), 1},
      {"TypeCodeOf(Circle)", _t6Shapes14CircleTypeCode(), 2},
      {"TypeCodeOf(Blob)", _t6Shapes12BlobTypeCode(), 4},
      /* Inherited fields are visible through the base type. */
      {"CoordTotal", _t6Shapes10CoordTotal(), 21},
      {"c.Kind()", _t6Shapes16DirectCircleKind(), 2},
      {"AreaOf(local Circle)", _t6Shapes15LocalCircleArea(), 48},
      {"c.Area() on a value copy", _t6Shapes16CopiedCircleArea(), 75},
  };

  int failed = 0;
  for (unsigned i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i) {
    if (checks[i].got == checks[i].expected) {
      printf("%-32s = %ld  OK\n", checks[i].name, checks[i].got);
    } else {
      printf("%-32s = %ld (tinylang) vs %ld (expected)  MISMATCH\n",
             checks[i].name, checks[i].got, checks[i].expected);
      failed = 1;
    }
  }
  if (!failed)
    printf("all inheritance/polymorphism checks OK\n");
  return failed;
}
