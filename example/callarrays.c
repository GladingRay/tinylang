#include <stdio.h>

extern void _t6Arrays4Fill(void);
extern long _t6Arrays8SumArray(long);
extern long _t6Arrays9SumMatrix(void);
extern long _t6Arrays8LocalSum(void);

int main(void) {
  _t6Arrays4Fill();

  long s10 = _t6Arrays8SumArray(10); /* 0+1+4+9+...+81 = 285 */
  long sm = _t6Arrays9SumMatrix();   /* 11+12+13+21+22+23 = 102 */
  long sl = _t6Arrays8LocalSum();    /* 1+2+3+4+5 = 15 */

  int failed = 0;
  if (s10 != 285) {
    printf("SumArray(10) = %ld, expected 285\n", s10);
    failed = 1;
  }
  if (sm != 102) {
    printf("SumMatrix() = %ld, expected 102\n", sm);
    failed = 1;
  }
  if (sl != 15) {
    printf("LocalSum() = %ld, expected 15\n", sl);
    failed = 1;
  }
  if (!failed)
    printf("all array checks OK (SumArray=%ld, SumMatrix=%ld, LocalSum=%ld)\n",
           s10, sm, sl);
  return failed;
}
