(* Expected diagnostic:
   err_function_and_return_type
   The RETURN value type must match the function return type. *)
MODULE Gcd;
VAR ok : BOOLEAN;

PROCEDURE F() : INTEGER;
BEGIN
  RETURN ok
END F;

END Gcd.
