(* Expected diagnostic:
   err_field_conflicts_with_base (field x)
   An extended record cannot redeclare an inherited field. *)
MODULE Gcd;
TYPE
  Base = RECORD x: INTEGER END;
  Derived = RECORD (Base) x, y: INTEGER END;
VAR d: Derived;
BEGIN
  d.y := 1
END Gcd.
