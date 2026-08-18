(* Expected diagnostic:
   err_returntype_must_be_type
   't' is a variable, not a type. *)
MODULE Gcd;
VAR t : INTEGER;

PROCEDURE F() : t;
BEGIN
END F;

END Gcd.
