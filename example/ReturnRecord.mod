MODULE ReturnRecord;

TYPE
  Point = RECORD x, y: INTEGER END;

PROCEDURE MakePoint(x, y: INTEGER): Point;
VAR p: Point;
BEGIN
  p.x := x;
  p.y := y;
  RETURN p;
END MakePoint;

PROCEDURE SumPoint(p: Point): INTEGER;
BEGIN
  RETURN p.x + p.y;
END SumPoint;

PROCEDURE TestReturn(): INTEGER;
VAR p: Point;
BEGIN
  p := MakePoint(10, 20);
  RETURN p.x + p.y + MakePoint(3, 4).x;
END TestReturn;

PROCEDURE TestChained(): INTEGER;
BEGIN
  RETURN MakePoint(1, 2).y + SumPoint(MakePoint(5, 6));
END TestChained;

END ReturnRecord.
