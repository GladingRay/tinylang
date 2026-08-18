(* Expected diagnostic:
   err_unterminated_char_or_string
   The string literal below is never closed. *)
MODULE Gcd;
VAR x : INTEGER;
BEGIN
  x := 1;
  "abc
END Gcd.
