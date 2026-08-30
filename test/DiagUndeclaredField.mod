(* Expected diagnostic:
   err_undeclared_field
   'missing' is not a field of record R. *)
MODULE Test;

TYPE R = RECORD x: INTEGER END;

VAR r: R;

PROCEDURE P();
BEGIN
  r.missing := 1;
END P;

END Test.
