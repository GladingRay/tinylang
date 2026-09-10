(* Expected diagnostic:
   err_method_not_implemented (method Area)
   A method declared in a record needs a definition with a receiver. *)
MODULE Gcd;
TYPE R = RECORD
           x: INTEGER;
           PROCEDURE Area(): INTEGER;
         END;
VAR r: R;
BEGIN
  r.x := 1
END Gcd.
