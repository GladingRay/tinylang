MODULE Real;

CONST
  Half = 0.5;

VAR r: REAL;

PROCEDURE SetR(x: REAL);
BEGIN
  r := x;
END SetR;

PROCEDURE GetR(): REAL;
BEGIN
  RETURN r;
END GetR;

PROCEDURE Add(a, b: REAL): REAL;
BEGIN
  RETURN a + b;
END Add;

PROCEDURE Mul(a, b: REAL): REAL;
BEGIN
  RETURN a * b;
END Mul;

PROCEDURE Div(a, b: REAL): REAL;
BEGIN
  RETURN a / b;
END Div;

PROCEDURE Neg(x: REAL): REAL;
BEGIN
  RETURN -x;
END Neg;

PROCEDURE Max(a, b: REAL): REAL;
BEGIN
  IF a >= b THEN
    RETURN a;
  END;
  RETURN b;
END Max;

PROCEDURE HalfOf(x: REAL): REAL;
BEGIN
  RETURN x * Half;
END HalfOf;

PROCEDURE Milli(x: REAL): REAL;
BEGIN
  RETURN x * 1.0E-3;
END Milli;

PROCEDURE Pow2(n: INTEGER): REAL;
VAR i: INTEGER;
    result: REAL;
BEGIN
  result := 1.0;
  i := 0;
  WHILE i < n DO
    result := result * 2.0;
    i := i + 1;
  END;
  RETURN result;
END Pow2;

PROCEDURE Quadratic(a, b, c, x: REAL): REAL;
BEGIN
  RETURN a * x * x + b * x + c;
END Quadratic;

END Real.
