; ModuleID = 'example/Real.mod'
source_filename = "example/Real.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t4Real1r = private global float 0.000000e+00

define void @_t4Real4SetR(float %x) {
entry:
  store float %x, ptr @_t4Real1r, align 4
  ret void
}

define float @_t4Real4GetR() {
entry:
  %0 = load float, ptr @_t4Real1r, align 4
  ret float %0
}

define float @_t4Real3Add(float %a, float %b) {
entry:
  %0 = fadd float %a, %b
  ret float %0
}

define float @_t4Real3Mul(float %a, float %b) {
entry:
  %0 = fmul float %a, %b
  ret float %0
}

define float @_t4Real3Div(float %a, float %b) {
entry:
  %0 = fdiv float %a, %b
  ret float %0
}

define float @_t4Real3Neg(float %x) {
entry:
  %0 = fneg float %x
  ret float %0
}

define float @_t4Real3Max(float %a, float %b) {
entry:
  %0 = fcmp oge float %a, %b
  br i1 %0, label %if.body.0, label %after.if.0

if.body.0:                                        ; preds = %entry
  ret float %a

after.if.0:                                       ; preds = %entry
  ret float %b
}

define float @_t4Real6HalfOf(float %x) {
entry:
  %0 = fmul float %x, 5.000000e-01
  ret float %0
}

define float @_t4Real5Milli(float %x) {
entry:
  %0 = fmul float %x, 1.000000e-03
  ret float %0
}

define float @_t4Real4Pow2(i64 %n) {
entry:
  br label %while.cond.0

while.cond.0:                                     ; preds = %while.body.0, %entry
  %0 = phi i64 [ %4, %while.body.0 ], [ 0, %entry ]
  %1 = phi float [ %3, %while.body.0 ], [ 1.000000e+00, %entry ]
  %2 = icmp slt i64 %0, %n
  br i1 %2, label %while.body.0, label %after.while.0

while.body.0:                                     ; preds = %while.cond.0
  %3 = fmul float %1, 2.000000e+00
  %4 = add nsw i64 %0, 1
  br label %while.cond.0

after.while.0:                                    ; preds = %while.cond.0
  ret float %1
}

define float @_t4Real9Quadratic(float %a, float %b, float %c, float %x) {
entry:
  %0 = fmul float %a, %x
  %1 = fmul float %0, %x
  %2 = fmul float %b, %x
  %3 = fadd float %1, %2
  %4 = fadd float %3, %c
  ret float %4
}
