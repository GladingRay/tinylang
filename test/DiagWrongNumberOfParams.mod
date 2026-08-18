(* Expected diagnostic:
   err_wrong_number_of_parameters
   GCD expects 2 arguments, but only 1 is passed. *)
MODULE Gcd;
VAR x : INTEGER;

PROCEDURE GCD(a, b : INTEGER) : INTEGER;
BEGIN
  RETURN a
END GCD;

BEGIN
  x := GCD(1)
END Gcd.
