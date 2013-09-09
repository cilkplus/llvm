; RUN: llc < %s -march=x86-64 -mcpu=corei7-avx | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Testing integers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; CHECK:       test32_1:
; CHECK:       movl %eax, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_1(i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}
declare void @func32(i32)

;
; CHECK:       test32_2:
; CHECK:       movl %ecx, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_2(i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_3:
; CHECK:       movl %edx, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_3(i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_4:
; CHECK-NOT:   movl
; CHECK:       callq func32
;
define x86_regcallcc void @test32_4(i32, i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_5:
; CHECK:       movl %esi, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_5(i32, i32, i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_6:
; CHECK:       movl %r8d, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_6(i32, i32, i32, i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_7:
; CHECK:       movl %r9d, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_7(i32, i32, i32, i32, i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_8:
; CHECK:       movl %r12d, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_8(i32, i32, i32, i32, i32, i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_9:
; CHECK:       movl %r13d, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_9(i32, i32, i32, i32, i32,
                                    i32, i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_10:
; CHECK:       movl %r14d, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_10(i32, i32, i32, i32, i32,
                                     i32, i32, i32, i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_11:
; CHECK:       movl %r15d, %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_11(i32, i32, i32, i32, i32,
                                     i32, i32, i32, i32, i32,
                                     i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test32_12:
; CHECK:       movl {{[0-9]+}}(%rsp), %edi
; CHECK-NEXT:  callq func32
;
define x86_regcallcc void @test32_12(i32, i32, i32, i32, i32,
                                     i32, i32, i32, i32, i32,
                                     i32, i32 %a) {
entry:
  call void @func32(i32 %a)
  ret void
}

;
; CHECK:       test8:
; CHECK:       movl %eax, %edi
; CHECK-NEXT:  callq func8
;
define x86_regcallcc void @test8(i8 %a) {
entry:
  call void @func8(i8 %a)
  ret void
}
declare x86_regcallcc void @func8(i8)

;
; CHECK:       test8_regcall:
; CHECK-FIXME: vmovaps
; CHECK-NOT:   movl %eax
; CHECK:       callq func8_regcall
;
define x86_regcallcc void @test8_regcall(i8 %a) {
entry:
  call x86_regcallcc void @func8_regcall(i8 %a)
  ret void
}
declare x86_regcallcc void @func8_regcall(i8)

;
; CHECK:       test64_1:
; CHECK:       movq %rax, %rdi
; CHECK-NEXT:  callq func64
;
define x86_regcallcc void @test64_1(i64 %a) {
entry:
  call void @func64(i64 %a)
  ret void
}
declare void @func64(i64)

;
; CHECK:       test64_6:
; CHECK:       movq %r8, %rdi
; CHECK-NEXT:  callq func64
;
define x86_regcallcc void @test64_6(i64, i64, i64, i64, i64, i64 %a) {
entry:
  call void @func64(i64 %a)
  ret void
}

;
; CHECK:       test64_8:
; CHECK:       movq %r12, %rdi
; CHECK-NEXT:  callq func64
;
define x86_regcallcc void @test64_8(i64, i64, i64, i64, i64, i64, i64, i64 %a) {
entry:
  call void @func64(i64 %a)
  ret void
}

;
; CHECK:       test64_12:
; CHECK:       movq {{[0-9]+}}(%rsp), %rdi
; CHECK-NEXT:  callq func64
;
define x86_regcallcc void @test64_12(i64, i64, i64, i64, i64,
                                     i64, i64, i64, i64, i64, i64, i64 %a) {
entry:
  call void @func64(i64 %a)
  ret void
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Testing pointers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; CHECK:       test_pointer_1:
; CHECK:       movq %rax, %rdi
; CHECK-NEXT:  callq func_ptr
;
define x86_regcallcc void @test_pointer_1(i8* %a) {
entry:
  call void @func_ptr(i8* %a)
  ret void
}
declare void @func_ptr(i8*)

;
; CHECK:       test_pointer_5:
; CHECK:       movq %rsi, %rdi
; CHECK-NEXT:  callq func_ptr
;
define x86_regcallcc void @test_pointer_5(i8*, i8*, i8*, i8*, i8* %a) {
entry:
  call void @func_ptr(i8* %a)
  ret void
}

;
; CHECK:       test_pointer_10:
; CHECK:       movq %r14, %rdi
; CHECK-NEXT:  callq func_ptr
;
define x86_regcallcc void @test_pointer_10(i8*, i8*, i8*, i8*, i8*,
                                           i8*, i8*, i8*, i8*, i8* %a) {
entry:
  call void @func_ptr(i8* %a)
  ret void
}

;
; CHECK:       test_pointer_12:
; CHECK:       movq {{[0-9]+}}(%rsp), %rdi
; CHECK-NEXT:  callq func_ptr
;
define x86_regcallcc void @test_pointer_12(i8*, i8*, i8*, i8*, i8*,
                                           i8*, i8*, i8*, i8*, i8*,
                                           i8*, i8* %a) {
entry:
  call void @func_ptr(i8* %a)
  ret void
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Testing floating points
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; CHECK:       test_float_1:
; CHECK:       vcvttss2si %xmm0, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_1(float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_float_3:
; CHECK:       vcvttss2si %xmm2, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_3(float, float, float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_float_5:
; CHECK:       vcvttss2si %xmm4, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_5(float, float, float, float, float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_float_7:
; CHECK:       vcvttss2si %xmm6, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_7(float, float, float, float, float,
                                        float, float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_float_9:
; CHECK: vcvttss2si %xmm8, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_9(float, float, float, float, float,
                                        float, float, float, float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_float_13:
; CHECK: vcvttss2si %xmm12, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_13(float, float, float, float, float,
                                         float, float, float, float, float,
                                         float, float, float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_float_16:
; CHECK: vcvttss2si %xmm15, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_16(float, float, float, float, float,
                                         float, float, float, float, float,
                                         float, float, float, float, float,
                                         float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_float_17:
; CHECK:       vcvttss2si {{[0-9]+}}(%rsp), %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_float_17(float, float, float, float, float,
                                         float, float, float, float, float,
                                         float, float, float, float, float,
                                         float, float %a) {
entry:
  %conv = fptoui float %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_double_1:
; CHECK:       vcvttsd2si %xmm0, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_double_1(double %a) {
entry:
  %conv = fptoui double %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_double_2:
; CHECK:       vcvttsd2si %xmm1, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_double_2(double, double %a) {
entry:
  %conv = fptoui double %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_double_4:
; CHECK:       vcvttsd2si %xmm3, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_double_4(double, double, double, double %a) {
entry:
  %conv = fptoui double %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_double_6:
; CHECK:       vcvttsd2si %xmm5, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_double_6(double, double, double, double, double,
                                         double %a) {
entry:
  %conv = fptoui double %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_double_10:
; CHECK-FIXME: vcvttsd2si %xmm9, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_double_10(double, double, double, double, double,
                                          double, double, double, double, double %a) {
entry:
  %conv = fptoui double %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_double_14:
; CHECK-FIXME: vcvttsd2si %xmm13, %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_double_14(double, double, double, double, double,
                                          double, double, double, double, double,
                                          double, double, double, double %a) {
entry:
  %conv = fptoui double %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_double_17:
; CHECK:       vcvttsd2si {{[0-9]+}}(%rsp), %rdi
; CHECK:       callq func32
;
define x86_regcallcc void @test_double_17(double, double, double, double, double,
                                          double, double, double, double, double,
                                          double, double, double, double, double,
                                          double, double %a) {
entry:
  %conv = fptoui double %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_fp128:
; CHECK-FIXME:
; CHECK:       callq func32
;
define x86_regcallcc void @test_fp128(fp128 %a) {
entry:
  %conv = fptoui fp128 %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_long_double_1:
; CHECK:       fldt
; CHECK:       callq func32
;
define x86_regcallcc void @test_long_double_1(x86_fp80 %a) {
entry:
  %conv = fptoui x86_fp80 %a to i32
  call void @func32(i32 %conv)
  ret void
}

;
; CHECK:       test_long_double_2:
; CHECK:       fldt
; CHECK:       fldt
; CHECK:       callq func32
;
define x86_regcallcc void @test_long_double_2(x86_fp80 %a1, x86_fp80 %a2) {
entry:
  %conv1 = fptoui x86_fp80 %a1 to i32
  %conv2 = fptoui x86_fp80 %a2 to i32
  %add = add i32 %conv1, %conv2
  call void @func32(i32 %add)
  ret void
}
