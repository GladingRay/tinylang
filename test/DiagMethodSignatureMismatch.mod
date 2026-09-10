(* Expected diagnostic:
   err_method_signature_mismatch (method Get)
   Overriding a method requires the same parameter and result types. *)
MODULE Gcd;
TYPE
  Base = RECORD x: INTEGER END;
  Derived = RECORD (Base) y: INTEGER END;

PROCEDURE (b: Base) Get(): INTEGER;
BEGIN
  RETURN b.x
END Get;

PROCEDURE (d: Derived) Get(v: INTEGER): INTEGER;
BEGIN
  RETURN v
END Get;

VAR d: Derived;
BEGIN
  d.y := 1
END Gcd.
