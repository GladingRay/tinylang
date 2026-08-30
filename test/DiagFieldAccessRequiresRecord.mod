(* Expected diagnostic:
   err_field_access_requires_record
   Field access on an INTEGER variable. *)
MODULE Test;

VAR x: INTEGER;

PROCEDURE P();
BEGIN
  x.f := 1;
END P;

END Test.
