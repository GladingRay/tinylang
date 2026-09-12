MODULE VarParam;

TYPE
  Point = RECORD x, y: INTEGER END;
  Vec = ARRAY [1..3] OF INTEGER;

VAR
  g: INTEGER;
  p: Point;
  v: Vec;

(* Reference parameters: the callee works on the caller's object. *)
PROCEDURE Inc(VAR n: INTEGER);
BEGIN
  n := n + 1
END Inc;

PROCEDURE AddTo(VAR n: INTEGER; delta: INTEGER);
BEGIN
  n := n + delta
END AddTo;

PROCEDURE MovePoint(VAR q: Point; dx, dy: INTEGER);
BEGIN
  q.x := q.x + dx;
  q.y := q.y + dy
END MovePoint;

PROCEDURE Fill(VAR a: Vec; base: INTEGER);
BEGIN
  a[1] := base;
  a[2] := base + 1;
  a[3] := base + 2
END Fill;

(* Value parameters: the callee only sees a copy. *)
PROCEDURE BumpVal(n: INTEGER): INTEGER;
BEGIN
  n := n + 1;
  RETURN n
END BumpVal;

PROCEDURE RefChangesCaller(): INTEGER;
VAR k: INTEGER;
BEGIN
  k := 7;
  Inc(k);
  RETURN k
END RefChangesCaller;

PROCEDURE ValueKeepsCaller(): INTEGER;
VAR k: INTEGER;
BEGIN
  k := 7;
  RETURN BumpVal(k) * 100 + k
END ValueKeepsCaller;

PROCEDURE LocalIncrement(): INTEGER;
VAR k: INTEGER;
BEGIN
  k := 41;
  Inc(k);
  AddTo(k, 2);
  RETURN k
END LocalIncrement;

PROCEDURE GlobalIncrement(): INTEGER;
BEGIN
  g := 10;
  Inc(g);
  RETURN g
END GlobalIncrement;

PROCEDURE IncElement(): INTEGER;
BEGIN
  v[1] := 5;
  Inc(v[1]);
  RETURN v[1]
END IncElement;

PROCEDURE MoveGlobalPoint(): INTEGER;
BEGIN
  p.x := 1;
  p.y := 2;
  MovePoint(p, 10, 20);
  RETURN p.x * 100 + p.y
END MoveGlobalPoint;

PROCEDURE FillGlobalArray(): INTEGER;
BEGIN
  Fill(v, 5);
  RETURN v[1] * 100 + v[2] * 10 + v[3]
END FillGlobalArray;

(* The promoted local is used across a loop, so it interacts with the phi
   construction of the SSA based locals. *)
PROCEDURE LoopIncrement(): INTEGER;
VAR i, s: INTEGER;
BEGIN
  s := 0;
  i := 0;
  WHILE i < 3 DO
    Inc(s);
    i := i + 1
  END;
  RETURN s
END LoopIncrement;

(* The same local passed both to a value parameter and to a VAR parameter.
   The VAR call forces k into memory, so every value call has to load the
   current value from that stack slot. *)
PROCEDURE MixedCalls(): INTEGER;
VAR k, a, b, c: INTEGER;
BEGIN
  k := 10;
  a := BumpVal(k);
  Inc(k);
  b := BumpVal(k);
  Inc(k);
  c := BumpVal(k);
  RETURN a * 10000 + b * 100 + c
END MixedCalls;

END VarParam.
