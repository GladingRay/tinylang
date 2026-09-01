#include <stdio.h>

extern long _t12ReturnRecord10TestReturn(void);
extern long _t12ReturnRecord11TestChained(void);

int main(void) {
  long ret = _t12ReturnRecord10TestReturn();   /* 10 + 20 + 3 = 33 */
  long chained = _t12ReturnRecord11TestChained(); /* 2 + 11 = 13 */

  int failed = 0;
  if (ret != 33) {
    printf("TestReturn() = %ld, expected 33\n", ret);
    failed = 1;
  }
  if (chained != 13) {
    printf("TestChained() = %ld, expected 13\n", chained);
    failed = 1;
  }
  if (!failed)
    printf("all record-return checks OK (TestReturn=%ld, TestChained=%ld)\n",
           ret, chained);
  return failed;
}
