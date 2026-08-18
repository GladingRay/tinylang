(* Expected diagnostic:
   err_proc_identifier_not_equal
   note_proc_identifier_declaration
   The identifier after END differs from the procedure name. *)
MODULE Gcd;

PROCEDURE GCD(a, b: INTEGER) : INTEGER;
BEGIN
  RETURN a
END GDC;

END Gcd.
