(* Expected diagnostic:
   err_types_for_operator_not_compatible (operator /)
   In Modula-2 '/' is real division; integer division uses DIV. *)
MODULE Gcd;
VAR x: INTEGER;
BEGIN
  x := 7 / 2
END Gcd.
