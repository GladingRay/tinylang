; ModuleID = 'example/TypeAlias.mod'
source_filename = "example/TypeAlias.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t9TypeAlias1v = private global [4 x i64] zeroinitializer

define void @_t9TypeAlias4Fill() {
entry:
  br label %while.cond.0

while.cond.0:                                     ; preds = %while.body.0, %entry
  %0 = phi i64 [ %5, %while.body.0 ], [ 0, %entry ]
  %1 = icmp slt i64 %0, 4
  br i1 %1, label %while.body.0, label %after.while.0

while.body.0:                                     ; preds = %while.cond.0
  %2 = mul nsw i64 %0, 2
  %3 = sub i64 %0, 0
  %4 = getelementptr [4 x i64], ptr @_t9TypeAlias1v, i64 0, i64 %3
  store i64 %2, ptr %4, align 8
  %5 = add nsw i64 %0, 1
  br label %while.cond.0

after.while.0:                                    ; preds = %while.cond.0
  ret void
}

define i64 @_t9TypeAlias6SumVec(i64 %n) {
entry:
  br label %while.cond.1

while.cond.1:                                     ; preds = %while.body.1, %entry
  %0 = phi i64 [ %7, %while.body.1 ], [ 0, %entry ]
  %1 = phi i64 [ %6, %while.body.1 ], [ 0, %entry ]
  %2 = icmp slt i64 %0, %n
  br i1 %2, label %while.body.1, label %after.while.1

while.body.1:                                     ; preds = %while.cond.1
  %3 = sub i64 %0, 0
  %4 = getelementptr [4 x i64], ptr @_t9TypeAlias1v, i64 0, i64 %3
  %5 = load i64, ptr %4, align 8
  %6 = add nsw i64 %1, %5
  %7 = add nsw i64 %0, 1
  br label %while.cond.1

after.while.1:                                    ; preds = %while.cond.1
  ret i64 %1
}

define i64 @_t9TypeAlias8Identity(i64 %x) {
entry:
  ret i64 %x
}
