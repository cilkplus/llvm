; RUN: llc < %s -march=x86-64 -mcpu=corei7     | FileCheck -check-prefix=SSE %s
; RUN: llc < %s -march=x86-64 -mcpu=corei7-avx | FileCheck -check-prefix=AVX %s
; RUN: llc < %s -march=x86-64 -mcpu=core-avx2  | FileCheck -check-prefix=AVX2 %s
; RUN: llc < %s -march=x86    -mcpu=corei7     | FileCheck -check-prefix=X86_SSE %s

;;;;;;;;;;;;;;;;;;;;;;;
;
; Testing integers
;
;;;;;;;;;;;;;;;;;;;;;;;

;
; SSE:       test_ret_i8:
; SSE:       movl $1, %eax
; SSE-NEXT:  ret
;
; AVX:       test_ret_i8:
; AVX:       movl $1, %eax
; AVX-NEXT:  ret
;
; AVX2:      test_ret_i8:
; AVX2:      movl $1, %eax
; AVX2-NEXT: ret
;
define x86_regcallcc i8 @test_ret_i8() {
  ret i8 1
}

;
; SSE:       test_ret_i16:
; SSE:       movl $2, %eax
; SSE-NEXT:  ret
;
; AVX:       test_ret_i16:
; AVX:       movl $2, %eax
; AVX-NEXT:  ret
;
; AVX2:      test_ret_i16:
; AVX2:      movl $2, %eax
; AVX2-NEXT: ret
;
define x86_regcallcc i16 @test_ret_i16() {
  ret i16 2
}

;
; SSE:       test_ret_i32:
; SSE:       movl $3, %eax
; SSE-NEXT:  ret
;
; AVX:       test_ret_i32:
; AVX:       movl $3, %eax
; AVX-NEXT:  ret
;
; AVX2:      test_ret_i32:
; AVX2:      movl $3, %eax
; AVX2-NEXT: ret
;
define x86_regcallcc i32 @test_ret_i32() {
  ret i32 3
}

;
; SSE:       test_ret_i64:
; SSE:       addq %rax, %rax
; SSE-NEXT:  ret
;
; AVX:       test_ret_i64:
; AVX:       addq %rax, %rax
; AVX-NEXT:  ret
;
; AVX2:      test_ret_i64:
; AVX2:      addq %rax, %rax
; AVX2-NEXT: ret
;
define x86_regcallcc i64 @test_ret_i64(i64 %a) {
  %b = add i64 %a, %a
  ret i64 %b
}

;
; SSE:       test_ret_struct5:
; SSE:       movl $1, %eax
; SSE-NEXT:  movl $2, %ecx
; SSE-NEXT:  movl $3, %edx
; SSE-NEXT:  movl $4, %edi
; SSE-NEXT:  movl $5, %esi
; SSE-NEXT:  ret
;
; X86_SSE:       test_ret_struct5:
; X86_SSE:       movl $1, %eax
; X86_SSE-NEXT:  movl $2, %ecx
; X86_SSE-NEXT:  movl $3, %edx
; X86_SSE-NEXT:  movl $4, %edi
; X86_SSE-NEXT:  movl $5, %esi
; X86_SSE-NEXT:  ret
;
define x86_regcallcc {i32, i32, i32, i32, i32} @test_ret_struct5() {
  ret {i32, i32, i32, i32, i32} {i32 1, i32 2, i32 3, i32 4, i32 5}
}

;
; SSE:       test_ret_v2i8:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v2i8:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v2i8:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <2 x i8> @test_ret_v2i8() {
  ret <2 x i8> <i8 1, i8 2>
}

;
; SSE:       test_ret_v4i8:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v4i8:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v4i8:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <4 x i8> @test_ret_v4i8() {
  ret <4 x i8> <i8 1, i8 2, i8 3, i8 4>
}

;
; SSE:       test_ret_v8i8:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v8i8:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v8i8:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <8 x i8> @test_ret_v8i8() {
  ret <8 x i8> <i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8>
}

;
; SSE:       test_ret_v16i8:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v16i8:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v16i8:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <16 x i8> @test_ret_v16i8() {
  ret <16 x i8> <i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8,
                 i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8>
}

;
; SSE:       test_ret_v32i8:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps %xmm0, %xmm1
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v32i8:
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm0
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm1
; AVX-FIXME: ret
;
; AVX2:      test_ret_v32i8:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: ret
;
define x86_regcallcc <32 x i8> @test_ret_v32i8() {
  ret <32 x i8> <i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8,
                 i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8,
                 i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8,
                 i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 8>
}

;
; SSE:       test_ret_v2i16:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v2i16:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v2i16:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <2 x i16> @test_ret_v2i16() {
  ret <2 x i16> <i16 1, i16 2>
}

;
; SSE:       test_ret_v4i16:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v4i16:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v4i16:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <4 x i16> @test_ret_v4i16() {
  ret <4 x i16> <i16 1, i16 2, i16 3, i16 4>
}

;
; SSE:       test_ret_v8i16:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v8i16:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v8i16:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <8 x i16> @test_ret_v8i16() {
  ret <8 x i16> <i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8>
}

;
; SSE:       test_ret_v16i16:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps %xmm0, %xmm1
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v16i16:
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm0
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm1
; AVX-FIXME: ret
;
; AVX2:      test_ret_v16i16:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT:  ret
;
define x86_regcallcc <16 x i16> @test_ret_v16i16() {
  ret <16 x i16> <i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8,
                  i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7, i16 8>
}

;
; SSE:       test_ret_v2i32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v2i32:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v2i32:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <2 x i32> @test_ret_v2i32() {
  ret <2 x i32> <i32 1, i32 2>
}

;
; SSE:       test_ret_v4i32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v4i32:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v4i32:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <4 x i32> @test_ret_v4i32() {
  ret <4 x i32> <i32 1, i32 2, i32 3, i32 4>
}

;
; SSE:       test_ret_v8i32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v8i32:
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm0
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm1
; AVX-FIXME: ret
;
; AVX2:      test_ret_v8i32:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: ret
;
define x86_regcallcc <8 x i32> @test_ret_v8i32() {
  ret <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
}

;
; SSE:       test_ret_v16i32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  movaps %xmm0, %xmm2
; SSE-NEXT:  movaps %xmm1, %xmm3
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v16i32:
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm0
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm1
; AVX-FIXME: vmovaps %xmm0, %xmm2
; AVX-FIXME: vmovaps %xmm1, %xmm3
; AVX-FIXME: ret
;
; AVX2:      test_ret_v16i32:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: vmovaps %ymm0, %ymm1
; AVX2-NEXT: ret
;
define x86_regcallcc <16 x i32> @test_ret_v16i32() {
  ret <16 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8,
                  i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
}

;
; SSE:       test_ret_v2i64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v2i64:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v2i64:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <2 x i64> @test_ret_v2i64() {
  ret <2 x i64> <i64 1, i64 2>
}

;
; SSE:       test_ret_v4i64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v4i64:
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm0
; AVX-FIXME: vmovaps {{.*}}(%rip), %xmm1
; AVX-FIXME: ret
;
; AVX2:      test_ret_v4i64:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: ret
;
define x86_regcallcc <4 x i64> @test_ret_v4i64() {
  ret <4 x i64> <i64 1, i64 2, i64 3, i64 4>
}

;
; SSE:       test_ret_v8i64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm2
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm3
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v8i64:
; AVX-FIXME: movaps {{.*}}(%rip), %xmm0
; AVX-FIXME: movaps {{.*}}(%rip), %xmm1
; AVX-FIXME: movaps {{.*}}(%rip), %xmm2
; AVX-FIXME: movaps {{.*}}(%rip), %xmm3
; AVX-FIXME: ret
;
; AVX2:      test_ret_v8i64:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: vmovaps {{.*}}(%rip), %ymm1
; AVX2-NEXT: ret
;
define x86_regcallcc <8 x i64> @test_ret_v8i64() {
  ret <8 x i64> <i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8>
}

;
; X86_SSE:      test_ret_v16i64:
; X86_SSE:      movaps {{.*}}, %xmm0
; X86_SSE-NEXT: movaps {{.*}}, %xmm1
; X86_SSE-NEXT: movaps {{.*}}, %xmm2
; X86_SSE-NEXT: movaps {{.*}}, %xmm3
; X86_SSE-NEXT: movaps %xmm0, %xmm4
; X86_SSE-NEXT: movaps %xmm1, %xmm5
; X86_SSE-NEXT: movaps %xmm2, %xmm6
; X86_SSE-NEXT: movaps %xmm3, %xmm7
; X86_SSE-NEXT: ret
;
; SSE:       test_ret_v16i64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm2
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm3
; SSE-NEXT:  movaps %xmm0, %xmm4
; SSE-NEXT:  movaps %xmm1, %xmm5
; SSE-NEXT:  movaps %xmm2, %xmm6
; SSE-NEXT:  movaps %xmm3, %xmm7
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v16i64:
; AVX-FIXME: movaps {{.*}}(%rip), %xmm0
; AVX-FIXME: movaps {{.*}}(%rip), %xmm1
; AVX-FIXME: movaps {{.*}}(%rip), %xmm2
; AVX-FIXME: movaps {{.*}}(%rip), %xmm3
; AVX-FIXME: movaps %xmm0, %xmm4
; AVX-FIXME: movaps %xmm1, %xmm5
; AVX-FIXME: movaps %xmm2, %xmm6
; AVX-FIXME: movaps %xmm3, %xmm7
; AVX-FIXME: ret
;
; AVX2:      test_ret_v16i64:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: vmovaps {{.*}}(%rip), %ymm1
; AVX2-NEXT: vmovaps %ymm0, %ymm2
; AVX2-NEXT: vmovaps %ymm1, %ymm3
; AVX2-NEXT: ret
;
define x86_regcallcc <16 x i64> @test_ret_v16i64() {
  ret <16 x i64> <i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8,
                  i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8>
}

;;;;;;;;;;;;;;;;;;;;
;
; Testing floats
;
;;;;;;;;;;;;;;;;;;;;

;
; SSE:       test_ret_f32:
; SSE:       movss {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_f32:
; AVX:       vmovss {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_f32:
; AVX2:      vmovss {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc float @test_ret_f32() {
  ret float 1.0
}

;
; SSE:       test_ret_v2f32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v2f32:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v2f32:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <2 x float> @test_ret_v2f32() {
  ret <2 x float> <float 1.0, float 2.0>
}

;
; SSE:       test_ret_v4f32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v4f32:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v4f32:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <4 x float> @test_ret_v4f32() {
  ret <4 x float> <float 1.0, float 2.0, float 3.0, float 4.0>
}

;
; SSE:       test_ret_v8f32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  ret
;
; AVX:       test_ret_v8f32:
; AVX:       vmovaps {{.*}}(%rip), %ymm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v8f32:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: ret
;
define x86_regcallcc <8 x float> @test_ret_v8f32() {
  ret <8 x float> <float 1.0, float 2.0, float 3.0, float 4.0,
                   float 5.0, float 6.0, float 7.0, float 8.0>
}

;
; SSE:       test_ret_v16f32:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  movaps %xmm0, %xmm2
; SSE-NEXT:  movaps %xmm1, %xmm3
; SSE-NEXT:  ret
;
; AVX:       test_ret_v16f32:
; AVX:       vmovaps {{.*}}(%rip), %ymm0
; AVX-NEXT:  vmovaps %ymm0, %ymm1
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v16f32:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: vmovaps %ymm0, %ymm1
; AVX2-NEXT: ret
;
define x86_regcallcc <16 x float> @test_ret_v16f32() {
  ret <16 x float> <float 1.0, float 2.0, float 3.0, float 4.0,
                    float 5.0, float 6.0, float 7.0, float 8.0,
                    float 1.0, float 2.0, float 3.0, float 4.0,
                    float 5.0, float 6.0, float 7.0, float 8.0>
}

;
; SSE:       test_ret_f64:
; SSE:       movsd {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_f64:
; AVX:       vmovsd {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_f64:
; AVX2:      vmovsd {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc double @test_ret_f64() {
  ret double 1.0
}

;
; SSE:       test_ret_v2f64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v2f64:
; AVX:       vmovaps {{.*}}(%rip), %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v2f64:
; AVX2:      vmovaps {{.*}}(%rip), %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <2 x double> @test_ret_v2f64() {
  ret <2 x double> <double 1.0, double 2.0>
}

;
; SSE:       test_ret_v4f64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  ret
;
; AVX:       test_ret_v4f64:
; AVX:       vmovaps {{.*}}(%rip), %ymm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v4f64:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: ret
;
define x86_regcallcc <4 x double> @test_ret_v4f64() {
  ret <4 x double> <double 1.0, double 2.0, double 3.0, double 4.0>
}

;
; SSE:       test_ret_v8f64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm2
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm3
; SSE-NEXT:  ret
;
; AVX:       test_ret_v8f64:
; AVX:       vmovaps {{.*}}(%rip), %ymm0
; AVX-NEXT:  vmovaps {{.*}}(%rip), %ymm1
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v8f64:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: vmovaps {{.*}}(%rip), %ymm1
; AVX2-NEXT: ret
;
define x86_regcallcc <8 x double> @test_ret_v8f64() {
  ret <8 x double> <double 1.0, double 2.0, double 3.0, double 4.0,
                    double 5.0, double 6.0, double 7.0, double 8.0>
}

;
; X86_SSE:       test_ret_v16f64:
; X86_SSE:       movaps {{.*}}, %xmm0
; X86_SSE-NEXT:  movaps {{.*}}, %xmm1
; X86_SSE-NEXT:  movaps {{.*}}, %xmm2
; X86_SSE-NEXT:  movaps {{.*}}, %xmm3
; X86_SSE-NEXT:  movaps %xmm0, %xmm4
; X86_SSE-NEXT:  movaps %xmm1, %xmm5
; X86_SSE-NEXT:  movaps %xmm2, %xmm6
; X86_SSE-NEXT:  movaps %xmm3, %xmm7
; X86_SSE-NEXT:  ret
;
; SSE:       test_ret_v16f64:
; SSE:       movaps {{.*}}(%rip), %xmm0
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm1
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm2
; SSE-NEXT:  movaps {{.*}}(%rip), %xmm3
; SSE-NEXT:  movaps %xmm0, %xmm4
; SSE-NEXT:  movaps %xmm1, %xmm5
; SSE-NEXT:  movaps %xmm2, %xmm6
; SSE-NEXT:  movaps %xmm3, %xmm7
; SSE-NEXT:  ret
;
; AVX:       test_ret_v16f64:
; AVX:       vmovaps {{.*}}(%rip), %ymm0
; AVX-NEXT:  vmovaps {{.*}}(%rip), %ymm1
; AVX-NEXT:  vmovaps %ymm0, %ymm2
; AVX-NEXT:  vmovaps %ymm1, %ymm3
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v16f64:
; AVX2:      vmovaps {{.*}}(%rip), %ymm0
; AVX2-NEXT: vmovaps {{.*}}(%rip), %ymm1
; AVX2-NEXT: vmovaps %ymm0, %ymm2
; AVX2-NEXT: vmovaps %ymm1, %ymm3
; AVX2-NEXT: ret
;
define x86_regcallcc <16 x double> @test_ret_v16f64() {
  ret <16 x double> <double 1.0, double 2.0, double 3.0, double 4.0,
                     double 5.0, double 6.0, double 7.0, double 8.0,
                     double 1.0, double 2.0, double 3.0, double 4.0,
                     double 5.0, double 6.0, double 7.0, double 8.0>
}

;;;;;;;;;;;;;;;;
;
; Test pointers
;
;;;;;;;;;;;;;;;;

;
; SSE:       test_ret_ptr:
; SSE:       addq $4, %rax
; SSE-NEXT:  ret
;
; AVX:       test_ret_ptr:
; AVX:       addq $4, %rax
; AVX-NEXT:  ret
;
; AVX2:      test_ret_ptr:
; AVX2:      addq $4, %rax
; AVX2-NEXT: ret
;
define x86_regcallcc i8* @test_ret_ptr(i8* %a) {
  %b = getelementptr inbounds i8* %a, i32 4
  ret i8* %b
}

;
; SSE:       test_ret_v2ptr:
; SSE:       shufpd $1, %xmm0, %xmm0
; SSE-NEXT:  ret
;
; AVX:       test_ret_v2ptr:
; AVX:       vshufpd $1, %xmm0, %xmm0, %xmm0
; AVX-NEXT:  ret
;
; AVX2:      test_ret_v2ptr:
; AVX2:      vshufpd $1, %xmm0, %xmm0, %xmm0
; AVX2-NEXT: ret
;
define x86_regcallcc <2 x i8*> @test_ret_v2ptr(<2 x i8*> %a) {
  %b = shufflevector <2 x i8*> %a, <2 x i8*> undef, <2 x i32> <i32 1, i32 0>
  ret <2 x i8*> %b
}

;
; SSE:       test_ret_v4ptr:
; SSE:       shufpd $1, %xmm0, %xmm0
; SSE-NEXT:  shufpd $1, %xmm1, %xmm1
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v4ptr:
; AVX-FIXME: shufpd $1, %xmm0, %xmm0
; AVX-FIXME: shufpd $1, %xmm1, %xmm1
; AVX-FIXME: ret
;
; AVX2:      test_ret_v4ptr:
; AVX2:      vpermilpd $5, %ymm0, %ymm0
; AVX2-NEXT: ret
;
define x86_regcallcc <4 x i8*> @test_ret_v4ptr(<4 x i8*> %a) {
  %b = shufflevector <4 x i8*> %a, <4 x i8*> undef, <4 x i32> <i32 1, i32 0, i32 3, i32 2>
  ret <4 x i8*> %b
}

;
; SSE:       test_ret_v8ptr:
; SSE:       shufpd $1, %xmm0, %xmm0
; SSE-NEXT:  shufpd $1, %xmm1, %xmm1
; SSE-NEXT:  shufpd $1, %xmm2, %xmm2
; SSE-NEXT:  shufpd $1, %xmm3, %xmm3
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v8ptr:
; AVX-FIXME: shufpd $1, %xmm0, %xmm0
; AVX-FIXME: shufpd $1, %xmm1, %xmm1
; AVX-FIXME: shufpd $1, %xmm2, %xmm2
; AVX-FIXME: shufpd $1, %xmm3, %xmm3
; AVX-FIXME: ret
;
; AVX2:      test_ret_v8ptr:
; AVX2:      vpermilpd $5, %ymm0, %ymm0
; AVX2-NEXT: vpermilpd $5, %ymm1, %ymm1
; AVX2-NEXT: ret
;
define x86_regcallcc <8 x i8*> @test_ret_v8ptr(<8 x i8*> %a) {
  %b = shufflevector <8 x i8*> %a, <8 x i8*> undef,
                     <8 x i32> <i32 1, i32 0, i32 3, i32 2, i32 5, i32 4, i32 7, i32 6>
  ret <8 x i8*> %b
}

;
; X86_SSE:       test_ret_v16ptr:
; X86_SSE:       pshufd $-79, %xmm0, %xmm0
; X86_SSE-NEXT:  pshufd $-79, %xmm1, %xmm1
; X86_SSE-NEXT:  pshufd $-79, %xmm2, %xmm2
; X86_SSE-NEXT:  pshufd $-79, %xmm3, %xmm3
; X86_SSE-NEXT:  ret
;
; SSE:       test_ret_v16ptr:
; SSE:       shufpd $1, %xmm0, %xmm0
; SSE-NEXT:  shufpd $1, %xmm1, %xmm1
; SSE-NEXT:  shufpd $1, %xmm2, %xmm2
; SSE-NEXT:  shufpd $1, %xmm3, %xmm3
; SSE-NEXT:  shufpd $1, %xmm4, %xmm4
; SSE-NEXT:  shufpd $1, %xmm5, %xmm5
; SSE-NEXT:  shufpd $1, %xmm6, %xmm6
; SSE-NEXT:  shufpd $1, %xmm7, %xmm7
; SSE-NEXT:  ret
;
; AVX-FIXME: test_ret_v16ptr:
; AVX-FIXME: shufpd $1, %xmm0, %xmm0
; AVX-FIXME: shufpd $1, %xmm1, %xmm1
; AVX-FIXME: shufpd $1, %xmm2, %xmm2
; AVX-FIXME: shufpd $1, %xmm3, %xmm3
; AVX-FIXME: shufpd $1, %xmm4, %xmm4
; AVX-FIXME: shufpd $1, %xmm5, %xmm5
; AVX-FIXME: shufpd $1, %xmm6, %xmm6
; AVX-FIXME: shufpd $1, %xmm7, %xmm7
; AVX-FIXME: ret
;
; AVX2:      test_ret_v16ptr:
; AVX2:      vpermilpd $5, %ymm0, %ymm0
; AVX2-NEXT: vpermilpd $5, %ymm1, %ymm1
; AVX2-NEXT: vpermilpd $5, %ymm2, %ymm2
; AVX2-NEXT: vpermilpd $5, %ymm3, %ymm3
; AVX2-NEXT: ret
;
define x86_regcallcc <16 x i8*> @test_ret_v16ptr(<16 x i8*> %a) {
  %b = shufflevector <16 x i8*> %a, <16 x i8*> undef,
                     <16 x i32> <i32 1, i32 0, i32 3, i32 2, i32 5, i32 4, i32 7, i32 6,
                                 i32 9, i32 8, i32 11, i32 10, i32 13, i32 12, i32 15, i32 14>
  ret <16 x i8*> %b
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Check callee saved registers
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; X86_SSE:       test_csr_i32:
; X86_SSE-NOT:   pushq
; X86_SSE-NOT:   popq
; X86_SSE:       ret
;
; SSE:       test_csr_i32:
; SSE-NOT:   pushq
; SSE-NOT:   popq
; SSE:       ret
;
define x86_regcallcc {i32, i32, i32, i32, i32, i32, i32, i32, i32} @test_csr_i32() {
  ret {i32, i32, i32, i32, i32, i32, i32, i32, i32}
      {i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9}
}

;
; X86_SSE:       test_csr_i64:
; X86_SSE-NOT:   pushq
; X86_SSE-NOT:   popq
; X86_SSE:       ret
;
; SSE:       test_csr_i64:
; SSE-NOT:   pushq
; SSE-NOT:   popq
; SSE:       ret
;
define x86_regcallcc {i64, i64, i64, i64, i64, i64, i64, i64, i64} @test_csr_i64() {
  ret {i64, i64, i64, i64, i64, i64, i64, i64, i64}
      {i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 10000000000, i64 10000000001}
}

;
; SSE:       test_csr_v32i64:
; SSE-NOT:   pushq
; SSE-NOT:   popq
; SSE:       ret
;
define x86_regcallcc <32 x i64> @test_csr_v32i64() {
  ret <32 x i64>
    <i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8,
     i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8,
     i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8,
     i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8>
}
