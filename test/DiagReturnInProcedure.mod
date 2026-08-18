(* Expected diagnostic:
   err_procedure_requires_empty_return
   A procedure must not RETURN a value. *)
MODULE Gcd;

PROCEDURE P(a : INTEGER);
BEGIN
  RETURN a
END P;

END Gcd.
