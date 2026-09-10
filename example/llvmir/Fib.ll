; ModuleID = 'example/Fib.mod'
source_filename = "example/Fib.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t3Fib6result = private global i64 0

define i64 @_t3Fib3FIB(i64 %n) {
entry:
  %0 = icmp slt i64 %n, 2
  br i1 %0, label %if.body.0, label %after.if.0

if.body.0:                                        ; preds = %entry
  ret i64 %n

after.if.0:                                       ; preds = %entry
  %1 = sub nsw i64 %n, 1
  %2 = call i64 @_t3Fib3FIB(i64 %1)
  %3 = sub nsw i64 %n, 2
  %4 = call i64 @_t3Fib3FIB(i64 %3)
  %5 = add nsw i64 %2, %4
  ret i64 %5
}

define void @_t3Fib7Compute() {
entry:
  %0 = call i64 @_t3Fib3FIB(i64 10)
  store i64 %0, ptr @_t3Fib6result, align 8
  ret void
}

define void @_t3Fib3Run() {
entry:
  call void @_t3Fib7Compute()
  ret void
}
