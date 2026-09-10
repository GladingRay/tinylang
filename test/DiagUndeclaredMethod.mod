(* Expected diagnostic:
   err_undeclared_method (method Nope)
   The receiver type has no method with that name. *)
MODULE Gcd;
TYPE Base = RECORD x: INTEGER END;
VAR b: Base;
BEGIN
  b.Nope()
END Gcd.
