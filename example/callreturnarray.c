#include <stdio.h>

extern long _t11ReturnArray6SumVec(void);
extern long _t11ReturnArray9SumDirect(void);
extern long _t11ReturnArray7TestVar(void);

int main(void) {
  long sum = _t11ReturnArray6SumVec();    /* 7 + 8 + 9 = 24 */
  long direct = _t11ReturnArray9SumDirect(); /* 7 + 9 = 16 */
  long var = _t11ReturnArray7TestVar();   /* 7 + 8 + 9 = 24 */

  int failed = 0;
  if (sum != 24) {
    printf("SumVec() = %ld, expected 24\n", sum);
    failed = 1;
  }
  if (direct != 16) {
    printf("SumDirect() = %ld, expected 16\n", direct);
    failed = 1;
  }
  if (var != 24) {
    printf("TestVar() = %ld, expected 24\n", var);
    failed = 1;
  }
  if (!failed)
    printf("all array-return checks OK (SumVec=%ld, SumDirect=%ld, TestVar=%ld)\n",
           sum, direct, var);
  return failed;
}
