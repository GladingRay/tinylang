(* Expected diagnostic:
   err_procedure_call_on_nonprocedure
   GCD has a return type, so it cannot be called as a statement. *)
MODULE Gcd;

PROCEDURE GCD(a, b : INTEGER) : INTEGER;
BEGIN
  RETURN a
END GCD;

BEGIN
  GCD(1, 2)
END Gcd.
