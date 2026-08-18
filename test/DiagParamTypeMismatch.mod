(* Expected diagnostic:
   err_type_of_formal_and_actual_parameter_not_compatible
   GCD expects INTEGER, but a BOOLEAN is passed as the 2nd argument. *)
MODULE Gcd;
VAR x : INTEGER;
    ok : BOOLEAN;

PROCEDURE GCD(a, b : INTEGER) : INTEGER;
BEGIN
  RETURN a
END GCD;

BEGIN
  x := GCD(1, ok)
END Gcd.
