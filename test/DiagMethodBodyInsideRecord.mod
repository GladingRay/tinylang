(* Expected diagnostic:
   err_method_body_inside_record
   A method body needs a receiver, so it must be written outside the
   record; only the declaration belongs into the record body. *)
MODULE Gcd;
TYPE R = RECORD
           x: INTEGER;
           PROCEDURE Area(): INTEGER;
           BEGIN RETURN 0 END Area;
         END;
VAR r: R;
BEGIN
  r.x := 1
END Gcd.
