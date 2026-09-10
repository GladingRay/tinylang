; ModuleID = 'example/Arrays.mod'
source_filename = "example/Arrays.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t6Arrays1a = private global [10 x i64] zeroinitializer
@_t6Arrays1m = private global [2 x [3 x i64]] zeroinitializer

define void @_t6Arrays4Fill() {
entry:
  br label %while.cond.0

while.cond.0:                                     ; preds = %while.body.0, %entry
  %0 = phi i64 [ %5, %while.body.0 ], [ 0, %entry ]
  %1 = icmp slt i64 %0, 10
  br i1 %1, label %while.body.0, label %after.while.0

while.body.0:                                     ; preds = %while.cond.0
  %2 = mul nsw i64 %0, %0
  %3 = sub i64 %0, 0
  %4 = getelementptr [10 x i64], ptr @_t6Arrays1a, i64 0, i64 %3
  store i64 %2, ptr %4, align 8
  %5 = add nsw i64 %0, 1
  br label %while.cond.0

after.while.0:                                    ; preds = %while.cond.0
  store i64 11, ptr @_t6Arrays1m, align 8
  store i64 12, ptr getelementptr ([3 x i64], ptr @_t6Arrays1m, i64 0, i64 1), align 8
  store i64 13, ptr getelementptr ([3 x i64], ptr @_t6Arrays1m, i64 0, i64 2), align 8
  store i64 21, ptr getelementptr ([2 x [3 x i64]], ptr @_t6Arrays1m, i64 0, i64 1), align 8
  store i64 22, ptr getelementptr ([3 x i64], ptr getelementptr ([2 x [3 x i64]], ptr @_t6Arrays1m, i64 0, i64 1), i64 0, i64 1), align 8
  store i64 23, ptr getelementptr ([3 x i64], ptr getelementptr ([2 x [3 x i64]], ptr @_t6Arrays1m, i64 0, i64 1), i64 0, i64 2), align 8
  ret void
}

define i64 @_t6Arrays8SumArray(i64 %n) {
entry:
  br label %while.cond.1

while.cond.1:                                     ; preds = %while.body.1, %entry
  %0 = phi i64 [ %7, %while.body.1 ], [ 0, %entry ]
  %1 = phi i64 [ %6, %while.body.1 ], [ 0, %entry ]
  %2 = icmp slt i64 %0, %n
  br i1 %2, label %while.body.1, label %after.while.1

while.body.1:                                     ; preds = %while.cond.1
  %3 = sub i64 %0, 0
  %4 = getelementptr [10 x i64], ptr @_t6Arrays1a, i64 0, i64 %3
  %5 = load i64, ptr %4, align 8
  %6 = add nsw i64 %1, %5
  %7 = add nsw i64 %0, 1
  br label %while.cond.1

after.while.1:                                    ; preds = %while.cond.1
  ret i64 %1
}

define i64 @_t6Arrays9SumMatrix() {
entry:
  br label %while.cond.2

while.cond.2:                                     ; preds = %after.while.3, %entry
  %0 = phi i64 [ %13, %after.while.3 ], [ 1, %entry ]
  %1 = phi i64 [ %4, %after.while.3 ], [ 0, %entry ]
  %2 = icmp sle i64 %0, 2
  br i1 %2, label %while.body.2, label %after.while.2

while.body.2:                                     ; preds = %while.cond.2
  br label %while.cond.3

after.while.2:                                    ; preds = %while.cond.2
  ret i64 %1

while.cond.3:                                     ; preds = %while.body.3, %while.body.2
  %3 = phi i64 [ %12, %while.body.3 ], [ 1, %while.body.2 ]
  %4 = phi i64 [ %11, %while.body.3 ], [ %1, %while.body.2 ]
  %5 = icmp sle i64 %3, 3
  br i1 %5, label %while.body.3, label %after.while.3

while.body.3:                                     ; preds = %while.cond.3
  %6 = sub i64 %0, 1
  %7 = getelementptr [2 x [3 x i64]], ptr @_t6Arrays1m, i64 0, i64 %6
  %8 = sub i64 %3, 1
  %9 = getelementptr [3 x i64], ptr %7, i64 0, i64 %8
  %10 = load i64, ptr %9, align 8
  %11 = add nsw i64 %4, %10
  %12 = add nsw i64 %3, 1
  br label %while.cond.3

after.while.3:                                    ; preds = %while.cond.3
  %13 = add nsw i64 %0, 1
  br label %while.cond.2
}

define i64 @_t6Arrays8LocalSum() {
entry:
  %0 = alloca [5 x i64], align 8
  store [5 x i64] zeroinitializer, ptr %0, align 8
  br label %while.cond.4

while.cond.4:                                     ; preds = %while.body.4, %entry
  %1 = phi i64 [ %6, %while.body.4 ], [ 0, %entry ]
  %2 = icmp slt i64 %1, 5
  br i1 %2, label %while.body.4, label %after.while.4

while.body.4:                                     ; preds = %while.cond.4
  %3 = add nsw i64 %1, 1
  %4 = sub i64 %1, 0
  %5 = getelementptr [5 x i64], ptr %0, i64 0, i64 %4
  store i64 %3, ptr %5, align 8
  %6 = add nsw i64 %1, 1
  br label %while.cond.4

after.while.4:                                    ; preds = %while.cond.4
  br label %while.cond.5

while.cond.5:                                     ; preds = %while.body.5, %after.while.4
  %7 = phi i64 [ %14, %while.body.5 ], [ 0, %after.while.4 ]
  %8 = phi i64 [ %13, %while.body.5 ], [ 0, %after.while.4 ]
  %9 = icmp slt i64 %7, 5
  br i1 %9, label %while.body.5, label %after.while.5

while.body.5:                                     ; preds = %while.cond.5
  %10 = sub i64 %7, 0
  %11 = getelementptr [5 x i64], ptr %0, i64 0, i64 %10
  %12 = load i64, ptr %11, align 8
  %13 = add nsw i64 %8, %12
  %14 = add nsw i64 %7, 1
  br label %while.cond.5

after.while.5:                                    ; preds = %while.cond.5
  ret i64 %8
}
