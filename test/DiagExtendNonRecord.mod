(* Expected diagnostic:
   err_extended_record_base_must_be_record
   Only a record type can be extended. *)
MODULE Gcd;
TYPE Bad = RECORD (INTEGER) x: INTEGER END;
VAR b: Bad;
BEGIN
  b.x := 1
END Gcd.
