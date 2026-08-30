(* Expected diagnostic:
   err_aggregate_return_not_supported
   Functions cannot return record values. *)
MODULE Test;

TYPE R = RECORD x: INTEGER END;

PROCEDURE GetR(): R;
VAR r: R;
BEGIN
  RETURN r;
END GetR;

END Test.
