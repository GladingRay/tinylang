MODULE Fib;

VAR result: INTEGER;

PROCEDURE FIB(n: INTEGER): INTEGER;
BEGIN
  IF n < 2 THEN
    RETURN n;
  END;
  RETURN FIB(n - 1) + FIB(n - 2);
END FIB;

PROCEDURE Compute();
BEGIN
  result := FIB(10);
END Compute;

PROCEDURE Run();
BEGIN
  Compute();
END Run;

END Fib.
