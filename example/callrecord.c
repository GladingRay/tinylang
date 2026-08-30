#include <stdio.h>

extern void _t6Record4Init(void);
extern long _t6Record9SumPerson(void);
extern long _t6Record8TestBump(void);
extern long _t6Record8TestCopy(void);
extern long _t6Record9TestValue(void);

int main(void) {
  _t6Record4Init();

  long person = _t6Record9SumPerson(); /* 3 + 10 + 2 + 100 + 200 = 315 */
  long bump = _t6Record8TestBump();    /* (1+100) + (2+200) = 303 */
  long copy = _t6Record8TestCopy();    /* 5 + 6 = 11 */
  long value = _t6Record9TestValue();  /* SumPoint(origin) = 11 */

  int failed = 0;
  if (person != 315) {
    printf("SumPerson() = %ld, expected 315\n", person);
    failed = 1;
  }
  if (bump != 303) {
    printf("TestBump() = %ld, expected 303\n", bump);
    failed = 1;
  }
  if (copy != 11) {
    printf("TestCopy() = %ld, expected 11\n", copy);
    failed = 1;
  }
  if (value != 11) {
    printf("TestValue() = %ld, expected 11\n", value);
    failed = 1;
  }
  if (!failed)
    printf("all record checks OK (SumPerson=%ld, Bump=%ld, Copy=%ld, Value=%ld)\n",
           person, bump, copy, value);
  return failed;
}
