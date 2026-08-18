(* Expected diagnostic:
   err_function_requires_return
   A function must RETURN a value. *)
MODULE Gcd;

PROCEDURE GCD(a : INTEGER) : INTEGER;
BEGIN
  RETURN
END GCD;

END Gcd.
