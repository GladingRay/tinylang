(* Expected diagnostic:
   err_invalid_real_literal
   '1.2E+' has a scale factor without any exponent digits. *)
MODULE Gcd;
VAR r: REAL;
BEGIN
  r := 1.2E+
END Gcd.
