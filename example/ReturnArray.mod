MODULE ReturnArray;

TYPE
  Vec = ARRAY [0..2] OF INTEGER;

PROCEDURE MakeVec(): Vec;
VAR v: Vec;
BEGIN
  v[0] := 7;
  v[1] := 8;
  v[2] := 9;
  RETURN v;
END MakeVec;

PROCEDURE SumVec(): INTEGER;
VAR v: Vec;
BEGIN
  v := MakeVec();
  RETURN v[0] + v[1] + v[2];
END SumVec;

PROCEDURE SumDirect(): INTEGER;
BEGIN
  RETURN MakeVec()[0] + MakeVec()[2];
END SumDirect;

PROCEDURE SumArr(VAR v: Vec): INTEGER;
BEGIN
  RETURN v[0] + v[1] + v[2];
END SumArr;

PROCEDURE TestVar(): INTEGER;
VAR v: Vec;
BEGIN
  v := MakeVec();
  RETURN SumArr(v);
END TestVar;

END ReturnArray.
