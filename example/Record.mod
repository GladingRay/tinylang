MODULE Record;

TYPE
  Point = RECORD x, y: INTEGER END;
  MyPoint = Point;
  Person = RECORD
    name, age: INTEGER;
    home: Point;
    tags: ARRAY [0..2] OF INTEGER
  END;

VAR
  origin: Point;
  p: Person;

PROCEDURE Init();
BEGIN
  origin.x := 1;
  origin.y := 2;
  p.name := 7;
  p.age := 3;
  p.home := origin;
  p.home.x := 10;
  p.tags[0] := 100;
  p.tags[1] := 200;
END Init;

PROCEDURE SumPerson(): INTEGER;
BEGIN
  RETURN p.age + p.home.x + p.home.y + p.tags[0] + p.tags[1];
END SumPerson;

PROCEDURE Bump(VAR r: Point);
BEGIN
  r.x := r.x + 100;
  r.y := r.y + 200;
END Bump;

PROCEDURE SumPoint(r: Point): INTEGER;
BEGIN
  RETURN r.x + r.y;
END SumPoint;

PROCEDURE TestBump(): INTEGER;
VAR q: MyPoint;
BEGIN
  q.x := 1;
  q.y := 2;
  Bump(q);
  RETURN q.x + q.y;
END TestBump;

PROCEDURE TestCopy(): INTEGER;
VAR q: MyPoint;
BEGIN
  q.x := 5;
  q.y := 6;
  origin := q;
  RETURN origin.x + origin.y;
END TestCopy;

PROCEDURE TestValue(): INTEGER;
BEGIN
  RETURN SumPoint(origin);
END TestValue;

END Record.
