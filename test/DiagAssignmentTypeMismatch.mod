(* Expected diagnostic:
   err_types_for_operator_not_compatible (operator :=)
   Assigning a BOOLEAN value to an INTEGER variable. *)
MODULE Gcd;
VAR x : INTEGER;
BEGIN
  x := TRUE
END Gcd.
