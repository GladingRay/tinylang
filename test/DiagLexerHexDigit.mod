(* Expected diagnostic:
   err_hex_digit_in_decimal
   '12A' is not a valid decimal literal.
   NOTE: the literal is placed where the parser never consumes it as an
   expression, to avoid the crashes described in the README. *)
MODULE 12A;
