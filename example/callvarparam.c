#include <stdio.h>

extern long _t8VarParam16RefChangesCaller(void);
extern long _t8VarParam16ValueKeepsCaller(void);
extern long _t8VarParam14LocalIncrement(void);
extern long _t8VarParam15GlobalIncrement(void);
extern long _t8VarParam10IncElement(void);
extern long _t8VarParam15MoveGlobalPoint(void);
extern long _t8VarParam15FillGlobalArray(void);
extern long _t8VarParam13LoopIncrement(void);

int main(void) {
  struct {
    const char *name;
    long got;
    long expected;
  } checks[] = {
      /* VAR parameters modify the caller's object. */
      {"Inc(local) -> 8", _t8VarParam16RefChangesCaller(), 8},
      {"Inc+AddTo(local) -> 44", _t8VarParam14LocalIncrement(), 44},
      {"Inc(global) -> 11", _t8VarParam15GlobalIncrement(), 11},
      {"Inc(v[1]) -> 6", _t8VarParam10IncElement(), 6},
      {"MovePoint(p, 10, 20) -> 1122", _t8VarParam15MoveGlobalPoint(), 1122},
      {"Fill(v, 5) -> 567", _t8VarParam15FillGlobalArray(), 567},
      {"Inc(s) in a WHILE loop -> 3", _t8VarParam13LoopIncrement(), 3},
      /* Value parameters only copy: the caller keeps k. */
      {"BumpVal(7) * 100 + 7 -> 807", _t8VarParam16ValueKeepsCaller(), 807},
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
    printf("all VAR/value parameter checks OK\n");
  return failed;
}
