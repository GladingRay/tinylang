(* Expected diagnostic:
   err_receiver_requires_record
   A type-bound procedure needs a record receiver. *)
MODULE Gcd;
PROCEDURE (i: INTEGER) Get(): INTEGER;
BEGIN
  RETURN 0
END Get;

VAR x: INTEGER;
BEGIN
  x := 1
END Gcd.
