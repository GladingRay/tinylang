#include <stdio.h>

extern void _t9TypeAlias4Fill(void);
extern long _t9TypeAlias6SumVec(long);
extern long _t9TypeAlias8Identity(long);

int main(void) {
  _t9TypeAlias4Fill();

  long s = _t9TypeAlias6SumVec(4);      /* 0 + 2 + 4 + 6 = 12 */
  long id = _t9TypeAlias8Identity(123); /* type alias round-trip */

  int failed = 0;
  if (s != 12) {
    printf("SumVec(4) = %ld, expected 12\n", s);
    failed = 1;
  }
  if (id != 123) {
    printf("Identity(123) = %ld, expected 123\n", id);
    failed = 1;
  }
  if (!failed)
    printf("all type alias checks OK (SumVec=%ld, Identity=%ld)\n", s, id);
  return failed;
}
