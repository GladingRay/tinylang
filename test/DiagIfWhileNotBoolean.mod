(* Expected diagnostics:
   err_if_expr_must_be_bool
   err_while_expr_must_be_bool
   The conditions must have type BOOLEAN. *)
MODULE Gcd;
VAR x : INTEGER;
BEGIN
  IF 1 THEN
    x := 1
  END;
  WHILE 1 DO
    x := 1
  END
END Gcd.
