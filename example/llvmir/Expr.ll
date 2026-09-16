; ModuleID = 'example/Expr.mod'
source_filename = "example/Expr.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

define i64 @_t4Expr10Precedence(i64 %a, i64 %b, i64 %c) {
entry:
  %0 = mul nsw i64 %b, %c
  %1 = add nsw i64 %a, %0
  ret i64 %1
}

define i64 @_t4Expr13Parenthesized(i64 %a, i64 %b, i64 %c) {
entry:
  %0 = add nsw i64 %a, %b
  %1 = mul nsw i64 %0, %c
  ret i64 %1
}

define i64 @_t4Expr6Nested(i64 %a, i64 %b, i64 %c) {
entry:
  %0 = add nsw i64 %a, %b
  %1 = sub nsw i64 %c, 1
  %2 = mul nsw i64 %0, %1
  %3 = add nsw i64 %b, %c
  %4 = sub nsw i64 %a, %3
  %5 = sub nsw i64 %2, %4
  ret i64 %5
}

define i64 @_t4Expr6NegAdd(i64 %a, i64 %b) {
entry:
  %0 = sub i64 0, %a
  %1 = add nsw i64 %0, %b
  ret i64 %1
}

define i64 @_t4Expr14ExplicitNegAdd(i64 %a, i64 %b) {
entry:
  %0 = sub i64 0, %a
  %1 = add nsw i64 %0, %b
  ret i64 %1
}

define i64 @_t4Expr6NegMul(i64 %a, i64 %b) {
entry:
  %0 = mul nsw i64 %a, %b
  %1 = sub i64 0, %0
  ret i64 %1
}

define i64 @_t4Expr7NegQuot(i64 %a, i64 %b) {
entry:
  %0 = sdiv i64 %a, %b
  %1 = sub i64 0, %0
  ret i64 %1
}

define i64 @_t4Expr6NegSum(i64 %a, i64 %b) {
entry:
  %0 = add nsw i64 %a, %b
  %1 = sub i64 0, %0
  ret i64 %1
}

define i64 @_t4Expr6DivMod(i64 %a, i64 %b) {
entry:
  %0 = sdiv i64 %a, %b
  %1 = mul nsw i64 %0, 100
  %2 = srem i64 %a, %b
  %3 = add nsw i64 %1, %2
  ret i64 %3
}

define i64 @_t4Expr9Relations(i64 %a, i64 %b) {
entry:
  %0 = icmp slt i64 %a, %b
  br i1 %0, label %if.body.0, label %after.if.0

if.body.0:                                        ; preds = %entry
  %1 = add nsw i64 0, 1
  br label %after.if.0

after.if.0:                                       ; preds = %if.body.0, %entry
  %2 = phi i64 [ %1, %if.body.0 ], [ 0, %entry ]
  %3 = icmp sle i64 %a, %b
  br i1 %3, label %if.body.1, label %after.if.1

if.body.1:                                        ; preds = %after.if.0
  %4 = add nsw i64 %2, 10
  br label %after.if.1

after.if.1:                                       ; preds = %if.body.1, %after.if.0
  %5 = phi i64 [ %4, %if.body.1 ], [ %2, %after.if.0 ]
  %6 = icmp sgt i64 %a, %b
  br i1 %6, label %if.body.2, label %after.if.2

if.body.2:                                        ; preds = %after.if.1
  %7 = add nsw i64 %5, 100
  br label %after.if.2

after.if.2:                                       ; preds = %if.body.2, %after.if.1
  %8 = phi i64 [ %7, %if.body.2 ], [ %5, %after.if.1 ]
  %9 = icmp sge i64 %a, %b
  br i1 %9, label %if.body.3, label %after.if.3

if.body.3:                                        ; preds = %after.if.2
  %10 = add nsw i64 %8, 1000
  br label %after.if.3

after.if.3:                                       ; preds = %if.body.3, %after.if.2
  %11 = phi i64 [ %10, %if.body.3 ], [ %8, %after.if.2 ]
  %12 = icmp eq i64 %a, %b
  br i1 %12, label %if.body.4, label %after.if.4

if.body.4:                                        ; preds = %after.if.3
  %13 = add nsw i64 %11, 10000
  br label %after.if.4

after.if.4:                                       ; preds = %if.body.4, %after.if.3
  %14 = phi i64 [ %13, %if.body.4 ], [ %11, %after.if.3 ]
  %15 = icmp ne i64 %a, %b
  br i1 %15, label %if.body.5, label %after.if.5

if.body.5:                                        ; preds = %after.if.4
  %16 = add nsw i64 %14, 100000
  br label %after.if.5

after.if.5:                                       ; preds = %if.body.5, %after.if.4
  %17 = phi i64 [ %16, %if.body.5 ], [ %14, %after.if.4 ]
  ret i64 %17
}

define i64 @_t4Expr12LogicalFlags(i64 %a, i64 %b) {
entry:
  %0 = icmp sgt i64 %a, %b
  %1 = xor i1 %0, true
  br i1 %1, label %if.body.6, label %after.if.6

if.body.6:                                        ; preds = %entry
  %2 = add nsw i64 0, 1
  br label %after.if.6

after.if.6:                                       ; preds = %if.body.6, %entry
  %3 = phi i64 [ %2, %if.body.6 ], [ 0, %entry ]
  %4 = icmp sgt i64 %a, %b
  %5 = icmp sgt i64 %b, 0
  %6 = or i1 %4, %5
  br i1 %6, label %if.body.7, label %after.if.7

if.body.7:                                        ; preds = %after.if.6
  %7 = add nsw i64 %3, 10
  br label %after.if.7

after.if.7:                                       ; preds = %if.body.7, %after.if.6
  %8 = phi i64 [ %7, %if.body.7 ], [ %3, %after.if.6 ]
  ret i64 %8
}
