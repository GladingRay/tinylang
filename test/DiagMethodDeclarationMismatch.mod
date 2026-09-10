(* Expected diagnostic:
   err_method_signature_mismatch (method Area)
   The implementation must match the declaration in the record body. *)
MODULE Gcd;
TYPE R = RECORD
           x: INTEGER;
           PROCEDURE Area(): INTEGER;
         END;
VAR r: R;

PROCEDURE (rr: R) Area(): REAL;
BEGIN
  RETURN 0.0
END Area;

BEGIN
  r.x := 1
END Gcd.
