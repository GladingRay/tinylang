(* Expected diagnostic:
   err_type_test_not_extension
   "b IS A" is only valid if A extends the static type of b. *)
MODULE Gcd;
TYPE
  A = RECORD x: INTEGER END;
  B = RECORD y: INTEGER END;
VAR b: B;
BEGIN
  IF b IS A THEN
    b.y := 1
  END
END Gcd.
