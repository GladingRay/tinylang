(* Expected diagnostic:
   err_vardecl_requires_type
   't' is a variable, not a type. *)
MODULE Gcd;
VAR t : INTEGER;
    x : t;
BEGIN
END Gcd.
