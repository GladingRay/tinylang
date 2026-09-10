; ModuleID = 'example/Shapes.mod'
source_filename = "example/Shapes.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t6ShapesT1desc = private constant [2 x ptr] [ptr @_t6ShapesT1Area, ptr @_t6ShapesT1Kind]
@_t6Shapes1c = private global { ptr, i64, i64, i64 } { ptr @_t6ShapesT1desc, i64 0, i64 0, i64 0 }
@_t6ShapesT2desc = private constant [2 x ptr] [ptr @_t6ShapesT2Area, ptr @_t6ShapesT2Kind]
@_t6Shapes1s = private global { ptr, i64, i64, i64 } { ptr @_t6ShapesT2desc, i64 0, i64 0, i64 0 }
@_t6ShapesT3desc = private constant [2 x ptr] [ptr @_t6ShapesT3Area, ptr @_t6ShapesT0Kind]
@_t6Shapes1b = private global { ptr, i64, i64, i64 } { ptr @_t6ShapesT3desc, i64 0, i64 0, i64 0 }
@_t6ShapesT0desc = private constant [2 x ptr] [ptr @_t6ShapesT0Area, ptr @_t6ShapesT0Kind]

define i64 @_t6ShapesT0Area(ptr captures(none) dereferenceable(24) %sh) {
entry:
  ret i64 0
}

define i64 @_t6ShapesT1Area(ptr captures(none) dereferenceable(32) %ci) {
entry:
  %0 = getelementptr { ptr, i64, i64, i64 }, ptr %ci, i32 0, i32 3
  %1 = load i64, ptr %0, align 8
  %2 = mul nsw i64 3, %1
  %3 = getelementptr { ptr, i64, i64, i64 }, ptr %ci, i32 0, i32 3
  %4 = load i64, ptr %3, align 8
  %5 = mul nsw i64 %2, %4
  ret i64 %5
}

define i64 @_t6ShapesT2Area(ptr captures(none) dereferenceable(32) %sq) {
entry:
  %0 = getelementptr { ptr, i64, i64, i64 }, ptr %sq, i32 0, i32 3
  %1 = load i64, ptr %0, align 8
  %2 = getelementptr { ptr, i64, i64, i64 }, ptr %sq, i32 0, i32 3
  %3 = load i64, ptr %2, align 8
  %4 = mul nsw i64 %1, %3
  ret i64 %4
}

define i64 @_t6ShapesT3Area(ptr captures(none) dereferenceable(32) %bl) {
entry:
  %0 = getelementptr { ptr, i64, i64, i64 }, ptr %bl, i32 0, i32 3
  %1 = load i64, ptr %0, align 8
  %2 = mul nsw i64 2, %1
  ret i64 %2
}

define i64 @_t6ShapesT0Kind(ptr captures(none) dereferenceable(24) %sh) {
entry:
  ret i64 1
}

define i64 @_t6ShapesT1Kind(ptr captures(none) dereferenceable(32) %ci) {
entry:
  ret i64 2
}

define i64 @_t6ShapesT2Kind(ptr captures(none) dereferenceable(32) %sq) {
entry:
  ret i64 3
}

define void @_t6Shapes5Setup() {
entry:
  store i64 1, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1c, i32 0, i32 1), align 8
  store i64 2, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1c, i32 0, i32 2), align 8
  store i64 5, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1c, i32 0, i32 3), align 8
  store i64 3, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1s, i32 0, i32 1), align 8
  store i64 4, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1s, i32 0, i32 2), align 8
  store i64 6, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1s, i32 0, i32 3), align 8
  store i64 5, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1b, i32 0, i32 1), align 8
  store i64 6, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1b, i32 0, i32 2), align 8
  store i64 7, ptr getelementptr ({ ptr, i64, i64, i64 }, ptr @_t6Shapes1b, i32 0, i32 3), align 8
  ret void
}

define i64 @_t6Shapes6AreaOf(ptr captures(none) dereferenceable(24) %sh) {
entry:
  %0 = load ptr, ptr %sh, align 8
  %1 = getelementptr [2 x ptr], ptr %0, i32 0, i32 0
  %2 = load ptr, ptr %1, align 8
  %3 = call i64 %2(ptr %sh)
  ret i64 %3
}

define i64 @_t6Shapes6KindOf(ptr captures(none) dereferenceable(24) %sh) {
entry:
  %0 = load ptr, ptr %sh, align 8
  %1 = getelementptr [2 x ptr], ptr %0, i32 0, i32 1
  %2 = load ptr, ptr %1, align 8
  %3 = call i64 %2(ptr %sh)
  ret i64 %3
}

define i64 @_t6Shapes8CoordSum(ptr captures(none) dereferenceable(24) %sh) {
entry:
  %0 = getelementptr { ptr, i64, i64 }, ptr %sh, i32 0, i32 1
  %1 = load i64, ptr %0, align 8
  %2 = getelementptr { ptr, i64, i64 }, ptr %sh, i32 0, i32 2
  %3 = load i64, ptr %2, align 8
  %4 = add nsw i64 %1, %3
  ret i64 %4
}

define i64 @_t6Shapes10TypeCodeOf(ptr captures(none) dereferenceable(24) %sh) {
entry:
  %0 = load ptr, ptr %sh, align 8
  %1 = icmp eq ptr %0, @_t6ShapesT1desc
  br i1 %1, label %if.body.0, label %after.if.0

if.body.0:                                        ; preds = %entry
  ret i64 2

after.if.0:                                       ; preds = %entry
  %2 = load ptr, ptr %sh, align 8
  %3 = icmp eq ptr %2, @_t6ShapesT2desc
  br i1 %3, label %if.body.1, label %after.if.1

if.body.1:                                        ; preds = %after.if.0
  ret i64 3

after.if.1:                                       ; preds = %after.if.0
  %4 = load ptr, ptr %sh, align 8
  %5 = icmp eq ptr %4, @_t6ShapesT3desc
  br i1 %5, label %if.body.2, label %after.if.2

if.body.2:                                        ; preds = %after.if.1
  ret i64 4

after.if.2:                                       ; preds = %after.if.1
  ret i64 1
}

define i64 @_t6Shapes10CircleArea() {
entry:
  %0 = call i64 @_t6Shapes6AreaOf(ptr @_t6Shapes1c)
  ret i64 %0
}

define i64 @_t6Shapes10SquareArea() {
entry:
  %0 = call i64 @_t6Shapes6AreaOf(ptr @_t6Shapes1s)
  ret i64 %0
}

define i64 @_t6Shapes8BlobArea() {
entry:
  %0 = call i64 @_t6Shapes6AreaOf(ptr @_t6Shapes1b)
  ret i64 %0
}

define i64 @_t6Shapes10CircleKind() {
entry:
  %0 = call i64 @_t6Shapes6KindOf(ptr @_t6Shapes1c)
  ret i64 %0
}

define i64 @_t6Shapes10SquareKind() {
entry:
  %0 = call i64 @_t6Shapes6KindOf(ptr @_t6Shapes1s)
  ret i64 %0
}

define i64 @_t6Shapes8BlobKind() {
entry:
  %0 = call i64 @_t6Shapes6KindOf(ptr @_t6Shapes1b)
  ret i64 %0
}

define i64 @_t6Shapes14CircleTypeCode() {
entry:
  %0 = call i64 @_t6Shapes10TypeCodeOf(ptr @_t6Shapes1c)
  ret i64 %0
}

define i64 @_t6Shapes12BlobTypeCode() {
entry:
  %0 = call i64 @_t6Shapes10TypeCodeOf(ptr @_t6Shapes1b)
  ret i64 %0
}

define i64 @_t6Shapes10CoordTotal() {
entry:
  %0 = call i64 @_t6Shapes8CoordSum(ptr @_t6Shapes1c)
  %1 = call i64 @_t6Shapes8CoordSum(ptr @_t6Shapes1s)
  %2 = add nsw i64 %0, %1
  %3 = call i64 @_t6Shapes8CoordSum(ptr @_t6Shapes1b)
  %4 = add nsw i64 %2, %3
  ret i64 %4
}

define i64 @_t6Shapes16DirectCircleKind() {
entry:
  %0 = load ptr, ptr @_t6Shapes1c, align 8
  %1 = getelementptr [2 x ptr], ptr %0, i32 0, i32 1
  %2 = load ptr, ptr %1, align 8
  %3 = call i64 %2(ptr @_t6Shapes1c)
  ret i64 %3
}

define i64 @_t6Shapes15LocalCircleArea() {
entry:
  %0 = alloca { ptr, i64, i64, i64 }, align 8
  store { ptr, i64, i64, i64 } { ptr @_t6ShapesT1desc, i64 0, i64 0, i64 0 }, ptr %0, align 8
  %1 = getelementptr { ptr, i64, i64, i64 }, ptr %0, i32 0, i32 1
  store i64 0, ptr %1, align 8
  %2 = getelementptr { ptr, i64, i64, i64 }, ptr %0, i32 0, i32 2
  store i64 0, ptr %2, align 8
  %3 = getelementptr { ptr, i64, i64, i64 }, ptr %0, i32 0, i32 3
  store i64 4, ptr %3, align 8
  %4 = call i64 @_t6Shapes6AreaOf(ptr %0)
  ret i64 %4
}

define i64 @_t6Shapes8CopyArea({ ptr, i64, i64, i64 } %cp) {
entry:
  %0 = alloca { ptr, i64, i64, i64 }, align 8
  store { ptr, i64, i64, i64 } %cp, ptr %0, align 8
  %1 = load ptr, ptr %0, align 8
  %2 = getelementptr [2 x ptr], ptr %1, i32 0, i32 0
  %3 = load ptr, ptr %2, align 8
  %4 = call i64 %3(ptr %0)
  ret i64 %4
}

define i64 @_t6Shapes16CopiedCircleArea() {
entry:
  %0 = load { ptr, i64, i64, i64 }, ptr @_t6Shapes1c, align 8
  %1 = call i64 @_t6Shapes8CopyArea({ ptr, i64, i64, i64 } %0)
  ret i64 %1
}
