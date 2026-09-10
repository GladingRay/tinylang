MODULE Shapes;

TYPE
  Shape = RECORD
    x, y: INTEGER;
    PROCEDURE Area(): INTEGER;
    PROCEDURE Kind(): INTEGER;
  END;
  Circle = RECORD (Shape)
    radius: INTEGER;
    PROCEDURE Area(): INTEGER;
    PROCEDURE Kind(): INTEGER;
  END;
  Square = RECORD (Shape)
    side: INTEGER;
    PROCEDURE Area(): INTEGER;
    PROCEDURE Kind(): INTEGER;
  END;
  Blob = RECORD (Shape)
    weight: INTEGER;
    (* Only Area is overridden; Kind is inherited from Shape. *)
    PROCEDURE Area(): INTEGER;
  END;

VAR
  c: Circle;
  s: Square;
  b: Blob;

PROCEDURE (sh: Shape) Area(): INTEGER;
BEGIN
  RETURN 0;
END Area;

PROCEDURE (ci: Circle) Area(): INTEGER;
BEGIN
  RETURN 3 * ci.radius * ci.radius;
END Area;

PROCEDURE (sq: Square) Area(): INTEGER;
BEGIN
  RETURN sq.side * sq.side;
END Area;

PROCEDURE (bl: Blob) Area(): INTEGER;
BEGIN
  RETURN 2 * bl.weight;
END Area;

PROCEDURE (sh: Shape) Kind(): INTEGER;
BEGIN
  RETURN 1;
END Kind;

PROCEDURE (ci: Circle) Kind(): INTEGER;
BEGIN
  RETURN 2;
END Kind;

PROCEDURE (sq: Square) Kind(): INTEGER;
BEGIN
  RETURN 3;
END Kind;

PROCEDURE Setup();
BEGIN
  c.x := 1;
  c.y := 2;
  c.radius := 5;
  s.x := 3;
  s.y := 4;
  s.side := 6;
  b.x := 5;
  b.y := 6;
  b.weight := 7;
END Setup;

(* Polymorphic: the static type is Shape, the dynamic type decides which
   method implementation runs. *)
PROCEDURE AreaOf(VAR sh: Shape): INTEGER;
BEGIN
  RETURN sh.Area();
END AreaOf;

PROCEDURE KindOf(VAR sh: Shape): INTEGER;
BEGIN
  RETURN sh.Kind();
END KindOf;

PROCEDURE CoordSum(VAR sh: Shape): INTEGER;
BEGIN
  RETURN sh.x + sh.y;
END CoordSum;

PROCEDURE TypeCodeOf(VAR sh: Shape): INTEGER;
BEGIN
  IF sh IS Circle THEN
    RETURN 2;
  END;
  IF sh IS Square THEN
    RETURN 3;
  END;
  IF sh IS Blob THEN
    RETURN 4;
  END;
  RETURN 1;
END TypeCodeOf;

PROCEDURE CircleArea(): INTEGER;
BEGIN
  RETURN AreaOf(c);
END CircleArea;

PROCEDURE SquareArea(): INTEGER;
BEGIN
  RETURN AreaOf(s);
END SquareArea;

PROCEDURE BlobArea(): INTEGER;
BEGIN
  RETURN AreaOf(b);
END BlobArea;

PROCEDURE CircleKind(): INTEGER;
BEGIN
  RETURN KindOf(c);
END CircleKind;

PROCEDURE SquareKind(): INTEGER;
BEGIN
  RETURN KindOf(s);
END SquareKind;

PROCEDURE BlobKind(): INTEGER;
BEGIN
  RETURN KindOf(b);
END BlobKind;

PROCEDURE CircleTypeCode(): INTEGER;
BEGIN
  RETURN TypeCodeOf(c);
END CircleTypeCode;

PROCEDURE BlobTypeCode(): INTEGER;
BEGIN
  RETURN TypeCodeOf(b);
END BlobTypeCode;

PROCEDURE CoordTotal(): INTEGER;
BEGIN
  RETURN CoordSum(c) + CoordSum(s) + CoordSum(b);
END CoordTotal;

PROCEDURE DirectCircleKind(): INTEGER;
BEGIN
  RETURN c.Kind();
END DirectCircleKind;

(* A local variable of an extended record type: the type descriptor must be
   initialised before the variable is passed on polymorphically. *)
PROCEDURE LocalCircleArea(): INTEGER;
VAR lc: Circle;
BEGIN
  lc.x := 0;
  lc.y := 0;
  lc.radius := 4;
  RETURN AreaOf(lc);
END LocalCircleArea;

(* A record value parameter is copied, including its type descriptor, so
   method calls on the copy still see the dynamic type. *)
PROCEDURE CopyArea(cp: Circle): INTEGER;
BEGIN
  RETURN cp.Area();
END CopyArea;

PROCEDURE CopiedCircleArea(): INTEGER;
BEGIN
  RETURN CopyArea(c);
END CopiedCircleArea;

END Shapes.
