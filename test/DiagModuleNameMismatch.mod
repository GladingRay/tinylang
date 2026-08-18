(* Expected diagnostic:
   err_module_identifier_not_equal
   note_module_identifier_declaration
   The identifier after END differs from the one after MODULE. *)
MODULE Gcd;

VAR x: INTEGER;

END WrongName.
