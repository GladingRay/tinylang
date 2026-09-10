; ModuleID = 'example/ReturnRecord.mod'
source_filename = "example/ReturnRecord.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

define { i64, i64 } @_t12ReturnRecord9MakePoint(i64 %x, i64 %y) {
entry:
  %0 = alloca { i64, i64 }, align 8
  store { i64, i64 } zeroinitializer, ptr %0, align 8
  %1 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 0
  store i64 %x, ptr %1, align 8
  %2 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 1
  store i64 %y, ptr %2, align 8
  %3 = load { i64, i64 }, ptr %0, align 8
  ret { i64, i64 } %3
}

define i64 @_t12ReturnRecord8SumPoint({ i64, i64 } %p) {
entry:
  %0 = alloca { i64, i64 }, align 8
  store { i64, i64 } %p, ptr %0, align 8
  %1 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 0
  %2 = load i64, ptr %1, align 8
  %3 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 1
  %4 = load i64, ptr %3, align 8
  %5 = add nsw i64 %2, %4
  ret i64 %5
}

define i64 @_t12ReturnRecord10TestReturn() {
entry:
  %0 = alloca { i64, i64 }, align 8
  store { i64, i64 } zeroinitializer, ptr %0, align 8
  %1 = call { i64, i64 } @_t12ReturnRecord9MakePoint(i64 10, i64 20)
  %2 = alloca { i64, i64 }, align 8
  store { i64, i64 } %1, ptr %2, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %0, ptr align 8 %2, i64 16, i1 false)
  %3 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 0
  %4 = load i64, ptr %3, align 8
  %5 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 1
  %6 = load i64, ptr %5, align 8
  %7 = add nsw i64 %4, %6
  %8 = call { i64, i64 } @_t12ReturnRecord9MakePoint(i64 3, i64 4)
  %9 = alloca { i64, i64 }, align 8
  store { i64, i64 } %8, ptr %9, align 8
  %10 = getelementptr { i64, i64 }, ptr %9, i32 0, i32 0
  %11 = load i64, ptr %10, align 8
  %12 = add nsw i64 %7, %11
  ret i64 %12
}

define i64 @_t12ReturnRecord11TestChained() {
entry:
  %0 = call { i64, i64 } @_t12ReturnRecord9MakePoint(i64 1, i64 2)
  %1 = alloca { i64, i64 }, align 8
  store { i64, i64 } %0, ptr %1, align 8
  %2 = getelementptr { i64, i64 }, ptr %1, i32 0, i32 1
  %3 = load i64, ptr %2, align 8
  %4 = call { i64, i64 } @_t12ReturnRecord9MakePoint(i64 5, i64 6)
  %5 = alloca { i64, i64 }, align 8
  store { i64, i64 } %4, ptr %5, align 8
  %6 = load { i64, i64 }, ptr %5, align 8
  %7 = call i64 @_t12ReturnRecord8SumPoint({ i64, i64 } %6)
  %8 = add nsw i64 %3, %7
  ret i64 %8
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
