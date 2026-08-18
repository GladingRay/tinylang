(* Expected diagnostic:
   err_var_parameter_requires_var
   NOTE: the intended semantics is that only non-variable arguments are
   rejected.  The current Sema check in Sema.cpp is inverted, so passing
   a variable currently triggers the error (and a literal does not). *)
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
  Swap(x, x)
END Gcd.
