MODULE TypeAlias;

TYPE
  MyInt = INTEGER;
  Index = MyInt;
  Vec = ARRAY [0..3] OF MyInt;
  VecAlias = Vec;

VAR
  v: VecAlias;

PROCEDURE Fill();
VAR i: Index;
BEGIN
  i := 0;
  WHILE i < 4 DO
    v[i] := i * 2;
    i := i + 1;
  END;
END Fill;

PROCEDURE SumVec(n: Index): MyInt;
VAR i, s: Index;
BEGIN
  s := 0;
  i := 0;
  WHILE i < n DO
    s := s + v[i];
    i := i + 1;
  END;
  RETURN s;
END SumVec;

PROCEDURE Identity(x: MyInt): MyInt;
BEGIN
  RETURN x;
END Identity;

END TypeAlias.
