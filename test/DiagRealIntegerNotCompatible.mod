(* Expected diagnostics:
   err_types_for_operator_not_compatible (operator +)
   err_types_for_operator_not_compatible (operator :=)
   Modula-2 REAL is neither expression- nor assignment-compatible with
   INTEGER; explicit conversion functions (FLOAT etc.) are required. *)
MODULE Gcd;
VAR x: INTEGER;
    r: REAL;
BEGIN
  r := x + 1.5
END Gcd.
