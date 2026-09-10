(* Expected diagnostic:
   err_duplicate_method (method Area)
   Declaring the same method twice inside a record is rejected. *)
MODULE Gcd;
TYPE R = RECORD
           x: INTEGER;
           PROCEDURE Area(): INTEGER;
           PROCEDURE Area(): INTEGER;
         END;
VAR r: R;

PROCEDURE (rr: R) Area(): INTEGER;
BEGIN
  RETURN 0
END Area;

BEGIN
  r.x := 1
END Gcd.
