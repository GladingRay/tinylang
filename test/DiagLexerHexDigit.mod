(* Expected diagnostic:
   err_hex_digit_in_decimal
   '12A' is not a valid decimal literal.  The lexer reports it and
   Sema treats the literal as 0 instead of aborting. *)
MODULE Gcd;
VAR x : INTEGER;
BEGIN
  x := 12A
END Gcd.
