; ModuleID = 'example/VarParam.mod'
source_filename = "example/VarParam.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t8VarParam1g = private global i64 0
@_t8VarParam1p = private global { i64, i64 } zeroinitializer
@_t8VarParam1v = private global [3 x i64] zeroinitializer

define void @_t8VarParam3Inc(ptr captures(none) dereferenceable(8) %n) {
entry:
  %0 = load i64, ptr %n, align 8
  %1 = add nsw i64 %0, 1
  store i64 %1, ptr %n, align 8
  ret void
}

define void @_t8VarParam5AddTo(ptr captures(none) dereferenceable(8) %n, i64 %delta) {
entry:
  %0 = load i64, ptr %n, align 8
  %1 = add nsw i64 %0, %delta
  store i64 %1, ptr %n, align 8
  ret void
}

define void @_t8VarParam9MovePoint(ptr captures(none) dereferenceable(16) %q, i64 %dx, i64 %dy) {
entry:
  %0 = getelementptr { i64, i64 }, ptr %q, i32 0, i32 0
  %1 = load i64, ptr %0, align 8
  %2 = add nsw i64 %1, %dx
  %3 = getelementptr { i64, i64 }, ptr %q, i32 0, i32 0
  store i64 %2, ptr %3, align 8
  %4 = getelementptr { i64, i64 }, ptr %q, i32 0, i32 1
  %5 = load i64, ptr %4, align 8
  %6 = add nsw i64 %5, %dy
  %7 = getelementptr { i64, i64 }, ptr %q, i32 0, i32 1
  store i64 %6, ptr %7, align 8
  ret void
}

define void @_t8VarParam4Fill(ptr captures(none) dereferenceable(24) %a, i64 %base) {
entry:
  %0 = getelementptr [3 x i64], ptr %a, i64 0, i64 0
  store i64 %base, ptr %0, align 8
  %1 = add nsw i64 %base, 1
  %2 = getelementptr [3 x i64], ptr %a, i64 0, i64 1
  store i64 %1, ptr %2, align 8
  %3 = add nsw i64 %base, 2
  %4 = getelementptr [3 x i64], ptr %a, i64 0, i64 2
  store i64 %3, ptr %4, align 8
  ret void
}

define i64 @_t8VarParam7BumpVal(i64 %n) {
entry:
  %0 = add nsw i64 %n, 1
  ret i64 %0
}

define i64 @_t8VarParam16RefChangesCaller() {
entry:
  %k = alloca i64, align 8
  store i64 0, ptr %k, align 8
  store i64 7, ptr %k, align 8
  call void @_t8VarParam3Inc(ptr %k)
  %0 = load i64, ptr %k, align 8
  ret i64 %0
}

define i64 @_t8VarParam16ValueKeepsCaller() {
entry:
  %0 = call i64 @_t8VarParam7BumpVal(i64 7)
  %1 = mul nsw i64 %0, 100
  %2 = add nsw i64 %1, 7
  ret i64 %2
}

define i64 @_t8VarParam14LocalIncrement() {
entry:
  %k = alloca i64, align 8
  store i64 0, ptr %k, align 8
  store i64 41, ptr %k, align 8
  call void @_t8VarParam3Inc(ptr %k)
  call void @_t8VarParam5AddTo(ptr %k, i64 2)
  %0 = load i64, ptr %k, align 8
  ret i64 %0
}

define i64 @_t8VarParam15GlobalIncrement() {
entry:
  store i64 10, ptr @_t8VarParam1g, align 8
  call void @_t8VarParam3Inc(ptr @_t8VarParam1g)
  %0 = load i64, ptr @_t8VarParam1g, align 8
  ret i64 %0
}

define i64 @_t8VarParam10IncElement() {
entry:
  store i64 5, ptr @_t8VarParam1v, align 8
  call void @_t8VarParam3Inc(ptr @_t8VarParam1v)
  %0 = load i64, ptr @_t8VarParam1v, align 8
  ret i64 %0
}

define i64 @_t8VarParam15MoveGlobalPoint() {
entry:
  store i64 1, ptr @_t8VarParam1p, align 8
  store i64 2, ptr getelementptr ({ i64, i64 }, ptr @_t8VarParam1p, i32 0, i32 1), align 8
  call void @_t8VarParam9MovePoint(ptr @_t8VarParam1p, i64 10, i64 20)
  %0 = load i64, ptr @_t8VarParam1p, align 8
  %1 = mul nsw i64 %0, 100
  %2 = load i64, ptr getelementptr ({ i64, i64 }, ptr @_t8VarParam1p, i32 0, i32 1), align 8
  %3 = add nsw i64 %1, %2
  ret i64 %3
}

define i64 @_t8VarParam15FillGlobalArray() {
entry:
  call void @_t8VarParam4Fill(ptr @_t8VarParam1v, i64 5)
  %0 = load i64, ptr @_t8VarParam1v, align 8
  %1 = mul nsw i64 %0, 100
  %2 = load i64, ptr getelementptr ([3 x i64], ptr @_t8VarParam1v, i64 0, i64 1), align 8
  %3 = mul nsw i64 %2, 10
  %4 = add nsw i64 %1, %3
  %5 = load i64, ptr getelementptr ([3 x i64], ptr @_t8VarParam1v, i64 0, i64 2), align 8
  %6 = add nsw i64 %4, %5
  ret i64 %6
}

define i64 @_t8VarParam13LoopIncrement() {
entry:
  %s = alloca i64, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %s, align 8
  br label %while.cond.0

while.cond.0:                                     ; preds = %while.body.0, %entry
  %0 = phi i64 [ %2, %while.body.0 ], [ 0, %entry ]
  %1 = icmp slt i64 %0, 3
  br i1 %1, label %while.body.0, label %after.while.0

while.body.0:                                     ; preds = %while.cond.0
  call void @_t8VarParam3Inc(ptr %s)
  %2 = add nsw i64 %0, 1
  br label %while.cond.0

after.while.0:                                    ; preds = %while.cond.0
  %3 = load i64, ptr %s, align 8
  ret i64 %3
}

define i64 @_t8VarParam10MixedCalls() {
entry:
  %k = alloca i64, align 8
  store i64 0, ptr %k, align 8
  store i64 10, ptr %k, align 8
  %0 = load i64, ptr %k, align 8
  %1 = call i64 @_t8VarParam7BumpVal(i64 %0)
  call void @_t8VarParam3Inc(ptr %k)
  %2 = load i64, ptr %k, align 8
  %3 = call i64 @_t8VarParam7BumpVal(i64 %2)
  call void @_t8VarParam3Inc(ptr %k)
  %4 = load i64, ptr %k, align 8
  %5 = call i64 @_t8VarParam7BumpVal(i64 %4)
  %6 = mul nsw i64 %1, 10000
  %7 = mul nsw i64 %3, 100
  %8 = add nsw i64 %6, %7
  %9 = add nsw i64 %8, %5
  ret i64 %9
}
