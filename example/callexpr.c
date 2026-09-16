#include <stdio.h>

extern long _t4Expr10Precedence(long, long, long);
extern long _t4Expr13Parenthesized(long, long, long);
extern long _t4Expr6Nested(long, long, long);
extern long _t4Expr6NegAdd(long, long);
extern long _t4Expr14ExplicitNegAdd(long, long);
extern long _t4Expr6NegMul(long, long);
extern long _t4Expr7NegQuot(long, long);
extern long _t4Expr6NegSum(long, long);
extern long _t4Expr6DivMod(long, long);
extern long _t4Expr9Relations(long, long);
extern long _t4Expr12LogicalFlags(long, long);

int main(void) {
  struct {
    const char *name;
    long got;
    long expected;
  } checks[] = {
      /* Precedence and parentheses. */
      {"2 + 3 * 4", _t4Expr10Precedence(2, 3, 4), 14},
      {"(2 + 3) * 4", _t4Expr13Parenthesized(2, 3, 4), 20},
      {"((2+3)*(4-1)) - (2-(3+4))", _t4Expr6Nested(2, 3, 4), 20},
      /* Unary minus: the sign belongs to the first term. */
      {"-7 + 3 == (-7) + 3", _t4Expr6NegAdd(7, 3), -4},
      {"(-7) + 3", _t4Expr14ExplicitNegAdd(7, 3), -4},
      {"-(7 + 3)", _t4Expr6NegSum(7, 3), -10},
      {"-3 * 4 == -(3 * 4)", _t4Expr6NegMul(3, 4), -12},
      {"-8 DIV 2", _t4Expr7NegQuot(8, 2), -4},
      /* Other operators. */
      {"(17 DIV 5) * 100 + 17 MOD 5", _t4Expr6DivMod(17, 5), 302},
      {"relational mask <,<=,>,>=,=,#", _t4Expr9Relations(3, 5), 100011},
      {"relational mask =,<=", _t4Expr9Relations(5, 5), 11010},
      {"NOT (3 > 5) and (3 > 5) OR (5 > 0)", _t4Expr12LogicalFlags(3, 5), 11},
      {"NOT (7 > 2) and (7 > 2) OR (2 > 0)", _t4Expr12LogicalFlags(7, 2), 10},
  };

  int failed = 0;
  for (unsigned i = 0; i < sizeof(checks) / sizeof(checks[0]); ++i) {
    if (checks[i].got == checks[i].expected) {
      printf("%-38s = %ld  OK\n", checks[i].name, checks[i].got);
    } else {
      printf("%-38s = %ld (tinylang) vs %ld (expected)  MISMATCH\n",
             checks[i].name, checks[i].got, checks[i].expected);
      failed = 1;
    }
  }
  if (!failed)
    printf("all expression checks OK\n");
  return failed;
}
