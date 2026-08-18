(* Expected diagnostic:
   err_var_parameter_requires_var
   A VAR formal parameter requires a variable as argument.
   The first call (with variables) is valid and produces no error;
   the second call passes a literal, which is rejected. *)
MODULE Gcd;
VAR x : INTEGER;

PROCEDURE Swap(VAR a, b : INTEGER);
VAR t : INTEGER;
BEGIN
  t := a;
  a := b;
  b := t
END Swap;

BEGIN
  Swap(x, x);
  Swap(1, x)
END Gcd.
