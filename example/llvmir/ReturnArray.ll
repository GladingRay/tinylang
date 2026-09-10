; ModuleID = 'example/ReturnArray.mod'
source_filename = "example/ReturnArray.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

define [3 x i64] @_t11ReturnArray7MakeVec() {
entry:
  %0 = alloca [3 x i64], align 8
  store [3 x i64] zeroinitializer, ptr %0, align 8
  %1 = getelementptr [3 x i64], ptr %0, i64 0, i64 0
  store i64 7, ptr %1, align 8
  %2 = getelementptr [3 x i64], ptr %0, i64 0, i64 1
  store i64 8, ptr %2, align 8
  %3 = getelementptr [3 x i64], ptr %0, i64 0, i64 2
  store i64 9, ptr %3, align 8
  %4 = load [3 x i64], ptr %0, align 8
  ret [3 x i64] %4
}

define i64 @_t11ReturnArray6SumVec() {
entry:
  %0 = alloca [3 x i64], align 8
  store [3 x i64] zeroinitializer, ptr %0, align 8
  %1 = call [3 x i64] @_t11ReturnArray7MakeVec()
  %2 = alloca [3 x i64], align 8
  store [3 x i64] %1, ptr %2, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %0, ptr align 8 %2, i64 24, i1 false)
  %3 = getelementptr [3 x i64], ptr %0, i64 0, i64 0
  %4 = load i64, ptr %3, align 8
  %5 = getelementptr [3 x i64], ptr %0, i64 0, i64 1
  %6 = load i64, ptr %5, align 8
  %7 = add nsw i64 %4, %6
  %8 = getelementptr [3 x i64], ptr %0, i64 0, i64 2
  %9 = load i64, ptr %8, align 8
  %10 = add nsw i64 %7, %9
  ret i64 %10
}

define i64 @_t11ReturnArray9SumDirect() {
entry:
  %0 = call [3 x i64] @_t11ReturnArray7MakeVec()
  %1 = alloca [3 x i64], align 8
  store [3 x i64] %0, ptr %1, align 8
  %2 = getelementptr [3 x i64], ptr %1, i64 0, i64 0
  %3 = load i64, ptr %2, align 8
  %4 = call [3 x i64] @_t11ReturnArray7MakeVec()
  %5 = alloca [3 x i64], align 8
  store [3 x i64] %4, ptr %5, align 8
  %6 = getelementptr [3 x i64], ptr %5, i64 0, i64 2
  %7 = load i64, ptr %6, align 8
  %8 = add nsw i64 %3, %7
  ret i64 %8
}

define i64 @_t11ReturnArray6SumArr(ptr captures(none) dereferenceable(24) %v) {
entry:
  %0 = getelementptr [3 x i64], ptr %v, i64 0, i64 0
  %1 = load i64, ptr %0, align 8
  %2 = getelementptr [3 x i64], ptr %v, i64 0, i64 1
  %3 = load i64, ptr %2, align 8
  %4 = add nsw i64 %1, %3
  %5 = getelementptr [3 x i64], ptr %v, i64 0, i64 2
  %6 = load i64, ptr %5, align 8
  %7 = add nsw i64 %4, %6
  ret i64 %7
}

define i64 @_t11ReturnArray7TestVar() {
entry:
  %0 = alloca [3 x i64], align 8
  store [3 x i64] zeroinitializer, ptr %0, align 8
  %1 = call [3 x i64] @_t11ReturnArray7MakeVec()
  %2 = alloca [3 x i64], align 8
  store [3 x i64] %1, ptr %2, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %0, ptr align 8 %2, i64 24, i1 false)
  %3 = call i64 @_t11ReturnArray6SumArr(ptr %0)
  ret i64 %3
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
