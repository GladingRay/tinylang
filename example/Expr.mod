MODULE Expr;

(* Precedence: * / DIV MOD bind tighter than + - *)
PROCEDURE Precedence(a, b, c: INTEGER): INTEGER;
BEGIN
  RETURN a + b * c
END Precedence;

PROCEDURE Parenthesized(a, b, c: INTEGER): INTEGER;
BEGIN
  RETURN (a + b) * c
END Parenthesized;

PROCEDURE Nested(a, b, c: INTEGER): INTEGER;
BEGIN
  RETURN ((a + b) * (c - 1)) - (a - (b + c))
END Nested;

(* A unary minus belongs to the first term: "-a + b" is "(-a) + b" ... *)
PROCEDURE NegAdd(a, b: INTEGER): INTEGER;
BEGIN
  RETURN -a + b
END NegAdd;

PROCEDURE ExplicitNegAdd(a, b: INTEGER): INTEGER;
BEGIN
  RETURN (-a) + b
END ExplicitNegAdd;

(* ... while "-a * b" negates the whole term, i.e. "-(a * b)" ... *)
PROCEDURE NegMul(a, b: INTEGER): INTEGER;
BEGIN
  RETURN -a * b
END NegMul;

PROCEDURE NegQuot(a, b: INTEGER): INTEGER;
BEGIN
  RETURN -a DIV b
END NegQuot;

(* ... and parentheses make a sum the operand of the sign. *)
PROCEDURE NegSum(a, b: INTEGER): INTEGER;
BEGIN
  RETURN -(a + b)
END NegSum;

PROCEDURE DivMod(a, b: INTEGER): INTEGER;
BEGIN
  RETURN (a DIV b) * 100 + (a MOD b)
END DivMod;

(* Relational operators report their result as a bit mask. *)
PROCEDURE Relations(a, b: INTEGER): INTEGER;
VAR r: INTEGER;
BEGIN
  r := 0;
  IF a < b THEN r := r + 1 END;
  IF a <= b THEN r := r + 10 END;
  IF a > b THEN r := r + 100 END;
  IF a >= b THEN r := r + 1000 END;
  IF a = b THEN r := r + 10000 END;
  IF a # b THEN r := r + 100000 END;
  RETURN r
END Relations;

PROCEDURE LogicalFlags(a, b: INTEGER): INTEGER;
VAR r: INTEGER;
BEGIN
  r := 0;
  IF NOT (a > b) THEN r := r + 1 END;
  IF (a > b) OR (b > 0) THEN r := r + 10 END;
  RETURN r
END LogicalFlags;

END Expr.
