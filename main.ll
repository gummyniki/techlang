; ModuleID = 'techlang'
source_filename = "techlang"

%person = type { i32, float }

@fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@fmt.1 = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1
@fmt.2 = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

declare i32 @printf(ptr, ...)

define void @main() {
entry:
  %i = alloca i32, align 4
  %p = alloca %person, align 8
  %fieldptr = getelementptr %person, ptr %p, i32 0, i32 0
  store i32 0, ptr %fieldptr, align 4
  %fieldptr1 = getelementptr %person, ptr %p, i32 0, i32 1
  store float 0.000000e+00, ptr %fieldptr1, align 4
  %fieldptr2 = getelementptr %person, ptr %p, i32 0, i32 0
  store i32 30, ptr %fieldptr2, align 4
  %fieldptr3 = getelementptr %person, ptr %p, i32 0, i32 1
  store float 1.750000e+00, ptr %fieldptr3, align 4
  %fieldptr4 = getelementptr %person, ptr %p, i32 0, i32 0
  %fieldval = load i32, ptr %fieldptr4, align 4
  %printtmp = call i32 (ptr, ...) @printf(ptr @fmt, i32 %fieldval)
  %fieldptr5 = getelementptr %person, ptr %p, i32 0, i32 1
  %fieldval6 = load float, ptr %fieldptr5, align 4
  %0 = fpext float %fieldval6 to double
  %printtmp7 = call i32 (ptr, ...) @printf(ptr @fmt.1, double %0)
  store i32 0, ptr %i, align 4
  br label %forcond

forcond:                                          ; preds = %forincrement, %entry
  %i8 = load i32, ptr %i, align 4
  %lttmp = icmp slt i32 %i8, 10
  br i1 %lttmp, label %forbody, label %forafter

forbody:                                          ; preds = %forcond
  %i9 = load i32, ptr %i, align 4
  %printtmp10 = call i32 (ptr, ...) @printf(ptr @fmt.2, i32 %i9)
  br label %forincrement

forincrement:                                     ; preds = %forbody
  %i11 = load i32, ptr %i, align 4
  %addtmp = add i32 %i11, 1
  store i32 %addtmp, ptr %i, align 4
  br label %forcond

forafter:                                         ; preds = %forcond
  ret void
}
