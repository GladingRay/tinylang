; ModuleID = 'example/Record.mod'
source_filename = "example/Record.mod"
target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-n32:64-S128-Fn32"
target triple = "arm64-apple-darwin25.6.0"

@_t6Record6origin = private global { i64, i64 } zeroinitializer
@_t6Record1p = private global { i64, i64, { i64, i64 }, [3 x i64] } zeroinitializer

define void @_t6Record4Init() {
entry:
  store i64 1, ptr @_t6Record6origin, align 8
  store i64 2, ptr getelementptr ({ i64, i64 }, ptr @_t6Record6origin, i32 0, i32 1), align 8
  store i64 7, ptr @_t6Record1p, align 8
  store i64 3, ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 1), align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 2), ptr align 8 @_t6Record6origin, i64 16, i1 false)
  store i64 10, ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 2), align 8
  store i64 100, ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 3), align 8
  store i64 200, ptr getelementptr ([3 x i64], ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 3), i64 0, i64 1), align 8
  ret void
}

define i64 @_t6Record9SumPerson() {
entry:
  %0 = load i64, ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 1), align 8
  %1 = load i64, ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 2), align 8
  %2 = add nsw i64 %0, %1
  %3 = load i64, ptr getelementptr ({ i64, i64 }, ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 2), i32 0, i32 1), align 8
  %4 = add nsw i64 %2, %3
  %5 = load i64, ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 3), align 8
  %6 = add nsw i64 %4, %5
  %7 = load i64, ptr getelementptr ([3 x i64], ptr getelementptr ({ i64, i64, { i64, i64 }, [3 x i64] }, ptr @_t6Record1p, i32 0, i32 3), i64 0, i64 1), align 8
  %8 = add nsw i64 %6, %7
  ret i64 %8
}

define void @_t6Record4Bump(ptr captures(none) dereferenceable(16) %r) {
entry:
  %0 = getelementptr { i64, i64 }, ptr %r, i32 0, i32 0
  %1 = load i64, ptr %0, align 8
  %2 = add nsw i64 %1, 100
  %3 = getelementptr { i64, i64 }, ptr %r, i32 0, i32 0
  store i64 %2, ptr %3, align 8
  %4 = getelementptr { i64, i64 }, ptr %r, i32 0, i32 1
  %5 = load i64, ptr %4, align 8
  %6 = add nsw i64 %5, 200
  %7 = getelementptr { i64, i64 }, ptr %r, i32 0, i32 1
  store i64 %6, ptr %7, align 8
  ret void
}

define i64 @_t6Record8SumPoint({ i64, i64 } %r) {
entry:
  %0 = alloca { i64, i64 }, align 8
  store { i64, i64 } %r, ptr %0, align 8
  %1 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 0
  %2 = load i64, ptr %1, align 8
  %3 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 1
  %4 = load i64, ptr %3, align 8
  %5 = add nsw i64 %2, %4
  ret i64 %5
}

define i64 @_t6Record8TestBump() {
entry:
  %0 = alloca { i64, i64 }, align 8
  store { i64, i64 } zeroinitializer, ptr %0, align 8
  %1 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 0
  store i64 1, ptr %1, align 8
  %2 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 1
  store i64 2, ptr %2, align 8
  call void @_t6Record4Bump(ptr %0)
  %3 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 0
  %4 = load i64, ptr %3, align 8
  %5 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 1
  %6 = load i64, ptr %5, align 8
  %7 = add nsw i64 %4, %6
  ret i64 %7
}

define i64 @_t6Record8TestCopy() {
entry:
  %0 = alloca { i64, i64 }, align 8
  store { i64, i64 } zeroinitializer, ptr %0, align 8
  %1 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 0
  store i64 5, ptr %1, align 8
  %2 = getelementptr { i64, i64 }, ptr %0, i32 0, i32 1
  store i64 6, ptr %2, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 @_t6Record6origin, ptr align 8 %0, i64 16, i1 false)
  %3 = load i64, ptr @_t6Record6origin, align 8
  %4 = load i64, ptr getelementptr ({ i64, i64 }, ptr @_t6Record6origin, i32 0, i32 1), align 8
  %5 = add nsw i64 %3, %4
  ret i64 %5
}

define i64 @_t6Record9TestValue() {
entry:
  %0 = load { i64, i64 }, ptr @_t6Record6origin, align 8
  %1 = call i64 @_t6Record8SumPoint({ i64, i64 } %0)
  ret i64 %1
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
