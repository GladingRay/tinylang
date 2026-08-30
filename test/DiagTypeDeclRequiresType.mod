(* Expected diagnostic:
   err_typedecl_requires_type
   'T = x' refers to a variable, not a type. *)
MODULE Test;

VAR x: INTEGER;

TYPE
  T = x;

BEGIN
END Test.
