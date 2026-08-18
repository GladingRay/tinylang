(* Expected diagnostic:
   err_function_call_on_nonfunction
   P has no return type, so it cannot be called as a function.
   (A follow-up type error may also appear because the call
   expression's type is null.) *)
MODULE Gcd;
VAR x : INTEGER;

PROCEDURE P(a : INTEGER);
BEGIN
END P;

BEGIN
  x := P(1)
END Gcd.
