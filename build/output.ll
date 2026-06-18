; ModuleID = 'techlang'
source_filename = "techlang"

@fmt = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@str = private unnamed_addr constant [21 x i8] c"hello from techlang!\00", align 1
@fmt.1 = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1

declare i32 @printf(ptr, ...)

define void @main() {
entry:
  %x = alloca i32, align 4
  store i32 42, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  %printtmp = call i32 (ptr, ...) @printf(ptr @fmt, i32 %x1)
  %printtmp2 = call i32 (ptr, ...) @printf(ptr @fmt.1, ptr @str)
  ret void
}
