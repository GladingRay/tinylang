(* Expected diagnostics:
   err_types_for_operator_not_compatible (operator +)
   err_types_for_operator_not_compatible (operator AND)
   Mixing INTEGER and BOOLEAN operands. *)
MODULE Gcd;
VAR x : INTEGER;
    ok : BOOLEAN;
BEGIN
  x := 1 + ok;
  x := 1 AND ok
END Gcd.
