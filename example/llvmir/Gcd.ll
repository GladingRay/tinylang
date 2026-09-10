; ModuleID = 'example/Gcd.mod'
source_filename = "example/Gcd.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t3Gcd1x = private global i64 0

define i64 @_t3Gcd3GCD(i64 %a, i64 %b) {
entry:
  %0 = icmp eq i64 %b, 0
  br i1 %0, label %if.body.0, label %after.if.0

if.body.0:                                        ; preds = %entry
  ret i64 %a

after.if.0:                                       ; preds = %entry
  br label %while.cond.0

while.cond.0:                                     ; preds = %while.body.0, %after.if.0
  %1 = phi i64 [ %4, %while.body.0 ], [ %b, %after.if.0 ]
  %2 = phi i64 [ %1, %while.body.0 ], [ %a, %after.if.0 ]
  %3 = icmp ne i64 %1, 0
  br i1 %3, label %while.body.0, label %after.while.0

while.body.0:                                     ; preds = %while.cond.0
  %4 = srem i64 %2, %1
  br label %while.cond.0

after.while.0:                                    ; preds = %while.cond.0
  ret i64 %2
}
