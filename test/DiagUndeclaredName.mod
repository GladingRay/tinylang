(* Expected diagnostic:
   err_undeclared_name ("undeclared name y") *)
MODULE Gcd;
VAR x : INTEGER;
BEGIN
  IF y THEN
    x := 1
  END
END Gcd.
