MODULE Arrays;

VAR
  a: ARRAY [0..9] OF INTEGER;
  m: ARRAY [1..2], [1..3] OF INTEGER;

PROCEDURE Fill();
VAR i: INTEGER;
BEGIN
  i := 0;
  WHILE i < 10 DO
    a[i] := i * i;
    i := i + 1;
  END;
  m[1][1] := 11; m[1][2] := 12; m[1][3] := 13;
  m[2][1] := 21; m[2][2] := 22; m[2][3] := 23;
END Fill;

PROCEDURE SumArray(n: INTEGER): INTEGER;
VAR i, s: INTEGER;
BEGIN
  s := 0;
  i := 0;
  WHILE i < n DO
    s := s + a[i];
    i := i + 1;
  END;
  RETURN s;
END SumArray;

PROCEDURE SumMatrix(): INTEGER;
VAR i, j, s: INTEGER;
BEGIN
  s := 0;
  i := 1;
  WHILE i <= 2 DO
    j := 1;
    WHILE j <= 3 DO
      s := s + m[i][j];
      j := j + 1;
    END;
    i := i + 1;
  END;
  RETURN s;
END SumMatrix;

PROCEDURE LocalSum(): INTEGER;
VAR b: ARRAY [0..4] OF INTEGER; i, s: INTEGER;
BEGIN
  i := 0;
  WHILE i < 5 DO
    b[i] := i + 1;
    i := i + 1;
  END;
  s := 0;
  i := 0;
  WHILE i < 5 DO
    s := s + b[i];
    i := i + 1;
  END;
  RETURN s;
END LocalSum;

END Arrays.
