; ModuleID = 'techlang'
source_filename = "techlang"

declare i32 @printf(ptr, ...)

declare void @tec_print_int(i32)

declare void @tec_print_float(float)

declare void @tec_print_string(i8)

declare void @tec_exit(i32)

declare i32 @tec_readInt()

declare float @tec_sqrt(float)

define void @main() {
entry:
  %s = alloca float, align 4
  %x = alloca i32, align 4
  store i32 5, ptr %x, align 4
  %x1 = load i32, ptr %x, align 4
  call void @tec_print_int(i32 %x1)
  call void @tec_print_int(i32 15)
  %calltmp = call float @tec_sqrt(float 1.600000e+01)
  store float %calltmp, ptr %s, align 4
  ret void
}
