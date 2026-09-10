(* Expected diagnostic:
   err_method_call_requires_record
   A method call needs a receiver of record type. *)
MODULE Gcd;
VAR i: INTEGER;
BEGIN
  i.Get()
END Gcd.
