(* Expected diagnostic:
   err_longreal_literal_not_implemented
   The scale factor 'D' denotes LONGREAL, which tinylang does not
   implement yet. *)
MODULE Gcd;
VAR r: REAL;
BEGIN
  r := 1.0D0
END Gcd.
