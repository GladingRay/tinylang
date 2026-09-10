(* Expected diagnostic:
   err_duplicate_method (method Get)
   A record type cannot declare the same method twice. *)
MODULE Gcd;
TYPE Base = RECORD x: INTEGER END;
VAR b: Base;

PROCEDURE (r: Base) Get(): INTEGER;
BEGIN
  RETURN r.x
END Get;

PROCEDURE (r: Base) Get(): INTEGER;
BEGIN
  RETURN r.x
END Get;

BEGIN
  b.x := 1
END Gcd.
