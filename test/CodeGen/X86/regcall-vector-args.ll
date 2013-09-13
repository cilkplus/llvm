; RUN: llc < %s -march=x86-64 -mcpu=corei7 | FileCheck -check-prefix=SSE %s
; RUN: llc < %s -march=x86-64 -mcpu=corei7-avx | FileCheck -check-prefix=AVX %s
; RUN: llc < %s -march=x86-64 -mcpu=corei7-avx | FileCheck -check-prefix=AVX-fp %s
; RUN: llc < %s -march=x86-64 -mcpu=core-avx2 | FileCheck -check-prefix=AVX2 %s
; RUN: llc < %s -march=x86-64 -mcpu=core-avx2 | FileCheck -check-prefix=AVX-fp %s

define x86_regcallcc <2 x i8> @v2i8(<2 x i8> %a, <2 x i8> %b) {
  %result = add <2 x i8> %a, %b
  ret <2 x i8> %result
  ; SSE: v2i8:
  ; SSE: paddq %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v2i8:
  ; AVX: vpaddq %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <4 x i8> @v4i8(<4 x i8> %a, <4 x i8> %b) {
  %result = add <4 x i8> %a, %b
  ret <4 x i8> %result
  ; SSE: v4i8:
  ; SSE: paddd %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v4i8:
  ; AVX: vpaddd %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <8 x i8> @v8i8(<8 x i8> %a, <8 x i8> %b) {
  %result = add <8 x i8> %a, %b
  ret <8 x i8> %result
  ; SSE: v8i8:
  ; SSE: paddw %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v8i8:
  ; AVX: vpaddw %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <16 x i8> @v16i8(<16 x i8> %a, <16 x i8> %b) {
  %result = add <16 x i8> %a, %b
  ret <16 x i8> %result
  ; SSE: v16i8:
  ; SSE: paddb %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v16i8:
  ; AVX: vpaddb %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

; i16
define x86_regcallcc <2 x i16> @v2i16(<2 x i16> %a, <2 x i16> %b) {
  %result = add <2 x i16> %a, %b
  ret <2 x i16> %result
  ; SSE: v2i16:
  ; SSE: paddq %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v2i16:
  ; AVX: vpaddq %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <4 x i16> @v4i16(<4 x i16> %a, <4 x i16> %b) {
  %result = add <4 x i16> %a, %b
  ret <4 x i16> %result
  ; SSE: v4i16:
  ; SSE: paddd %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v4i16:
  ; AVX: vpaddd %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <8 x i16> @v8i16(<8 x i16> %a, <8 x i16> %b) {
  %result = add <8 x i16> %a, %b
  ret <8 x i16> %result
  ; SSE: v8i16:
  ; SSE: paddw %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v8i16:
  ; AVX: vpaddw %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <16 x i16> @v16i16(<16 x i16> %a, <16 x i16> %b) {
  %result = add <16 x i16> %a, %b
  ret <16 x i16> %result
  ; SSE: v16i16:
  ; SSE: paddw %xmm2, %xmm0
  ; SSE: paddw %xmm3, %xmm1
  ; SSE-NEXT: ret

  ; AVX: v16i16:
  ; AVX-FIXME: vpaddw %xmm2, %xmm0, %xmm0
  ; AVX-FIXME: vpaddw %xmm3, %xmm1, %xmm0
  ; AVX-FIXME-NEXT: ret
}

; i32
define x86_regcallcc <2 x i32> @v2i32(<2 x i32> %a, <2 x i32> %b) {
  %result = add <2 x i32> %a, %b
  ret <2 x i32> %result
  ; SSE: v2i32:
  ; SSE: paddq %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v2i32:
  ; AVX: vpaddq %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <4 x i32> @v4i32(<4 x i32> %a, <4 x i32> %b) {
  %result = add <4 x i32> %a, %b
  ret <4 x i32> %result
  ; SSE: v4i32:
  ; SSE: paddd %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v4i32:
  ; AVX: vpaddd %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret
}

define x86_regcallcc <8 x i32> @v8i32(<8 x i32> %a, <8 x i32> %b) {
  %result = add <8 x i32> %a, %b
  ret <8 x i32> %result
  ; SSE: v8i32:
  ; SSE: paddd %xmm2, %xmm0
  ; SSE: paddd %xmm3, %xmm1
  ; SSE-NEXT: ret

  ; AVX: v8i32:
  ; AVX-FIXME: vpaddd %xmm2, %xmm0, %xmm0
  ; AVX-FIXME: vpaddd %xmm3, %xmm1, %xmm1
  ; AVX-FIXME-NEXT: ret
}

define x86_regcallcc <16 x i32> @v16i32(<16 x i32> %a, <16 x i32> %b) {
  %result = add <16 x i32> %a, %b
  ret <16 x i32> %result
  ; SSE: v16i32:
  ; SSE: paddd %xmm4, %xmm0
  ; SSE: paddd %xmm5, %xmm1
  ; SSE: paddd %xmm6, %xmm2
  ; SSE: paddd %xmm7, %xmm3
  ; SSE-NEXT: ret

  ; AVX: v16i32:
  ; AVX-FIXME: vpaddd %xmm4, %xmm0, %xmm0
  ; AVX-FIXME: vpaddd %xmm5, %xmm1, %xmm1
  ; AVX-FIXME: vpaddd %xmm6, %xmm2, %xmm2
  ; AVX-FIXME: vpaddd %xmm7, %xmm3, %xmm3
  ; AVX-FIXME-NEXT: ret

  ; AVX2: v16i32:
  ; AVX2: vpaddd %ymm2, %ymm0, %ymm0
  ; AVX2: vpaddd %ymm3, %ymm1, %ymm1
  ; AVX2-NEXT: ret
}

; i64
define x86_regcallcc <2 x i64> @v2i64(<2 x i64> %a, <2 x i64> %b) {
  %result = add <2 x i64> %a, %b
  ret <2 x i64> %result
  ; SSE: v2i64:
  ; SSE: paddq %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX: v2i64:
  ; AVX: vpaddq %xmm1, %xmm0, %xmm0
  ; AVX-NEXT: ret

  ; AVX2: v2i64:
  ; AVX2: vpaddq %xmm1, %xmm0, %xmm0
  ; AVX2-NEXT: ret
}

define x86_regcallcc <4 x i64> @v4i64(<4 x i64> %a, <4 x i64> %b) {
  %result = add <4 x i64> %a, %b
  ret <4 x i64> %result
  ; SSE: v4i64:
  ; SSE: paddq %xmm2, %xmm0
  ; SSE: paddq %xmm3, %xmm1
  ; SSE-NEXT: ret

  ; AVX: v4i64:
  ; AVX-FIXME: vpaddq %xmm2, %xmm0, %xmm0
  ; AVX-FIXME: vpaddq %xmm3, %xmm1, %xmm1
  ; AVX-FIXME-NEXT: ret

  ; AVX2: v4i64:
  ; AVX2: vpaddq %ymm1, %ymm0, %ymm0
  ; AVX2-NEXT: ret
}

define x86_regcallcc <8 x i64> @v8i64(<8 x i64> %a, <8 x i64> %b) {
  %result = add <8 x i64> %a, %b
  ret <8 x i64> %result
  ; SSE: v8i64:
  ; SSE: paddq %xmm4, %xmm0
  ; SSE: paddq %xmm5, %xmm1
  ; SSE: paddq %xmm6, %xmm2
  ; SSE: paddq %xmm7, %xmm3
  ; SSE-NEXT: ret

  ; AVX: v8i64:
  ; AVX-FIXME: vpaddq %xmm4, %xmm0, %xmm0
  ; AVX-FIXME: vpaddq %xmm5, %xmm1, %xmm1
  ; AVX-FIXME: vpaddq %xmm6, %xmm2, %xmm2
  ; AVX-FIXME: vpaddq %xmm7, %xmm3, %xmm3
  ; AVX-FIXME-NEXT: ret

  ; AVX2: v8i64:
  ; AVX2: vpaddq %ymm2, %ymm0, %ymm0
  ; AVX2: vpaddq %ymm3, %ymm1, %ymm1
  ; AVX2-NEXT: ret
}

define x86_regcallcc <16 x i64> @v16i64(<16 x i64> %a, <16 x i64> %b) {
  %result = add <16 x i64> %a, %b
  ret <16 x i64> %result
  ; SSE: v16i64:
  ; SSE: paddq %xmm8,  %xmm0
  ; SSE: paddq %xmm9,  %xmm1
  ; SSE: paddq %xmm10, %xmm2
  ; SSE: paddq %xmm11, %xmm3
  ; SSE: paddq %xmm12, %xmm4
  ; SSE: paddq %xmm13, %xmm5
  ; SSE: paddq %xmm14, %xmm6
  ; SSE: paddq %xmm15, %xmm7

  ; AVX: v16i64:
  ; AVX-FIXME: paddq %xmm8,  %xmm0
  ; AVX-FIXME: paddq %xmm9,  %xmm1
  ; AVX-FIXME: paddq %xmm10, %xmm2
  ; AVX-FIXME: paddq %xmm11, %xmm3
  ; AVX-FIXME: paddq %xmm12, %xmm4
  ; AVX-FIXME: paddq %xmm13, %xmm5
  ; AVX-FIXME: paddq %xmm14, %xmm6
  ; AVX-FIXME: paddq %xmm15, %xmm7

  ; AVX2: v16i64:
  ; AVX2: vpaddq %ymm4, %ymm0, %ymm0
  ; AVX2: vpaddq %ymm5, %ymm1, %ymm1
  ; AVX2: vpaddq %ymm6, %ymm2, %ymm2
  ; AVX2: vpaddq %ymm7, %ymm3, %ymm3
}

; float
define x86_regcallcc <2 x float> @v2f32(<2 x float> %a, <2 x float> %b) {
  %result = fadd <2 x float> %a, %b
  ret <2 x float> %result
  ; SSE: v2f32:
  ; SSE: addps %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX-fp: v2f32:
  ; AVX-fp: vaddps %xmm1, %xmm0, %xmm0
  ; AVX-fp-NEXT: ret
}

define x86_regcallcc <4 x float> @v4f32(<4 x float> %a, <4 x float> %b) {
  %result = fadd <4 x float> %a, %b
  ret <4 x float> %result
  ; SSE: v4f32:
  ; SSE: addps %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX-fp: v4f32:
  ; AVX-fp: vaddps %xmm1, %xmm0, %xmm0
  ; AVX-fp-NEXT: ret
}

define x86_regcallcc <8 x float> @v8f32(<8 x float> %a, <8 x float> %b) {
  %result = fadd <8 x float> %a, %b
  ret <8 x float> %result
  ; SSE: v8f32:
  ; SSE: addps %xmm2, %xmm0
  ; SSE: addps %xmm3, %xmm1
  ; SSE-NEXT: ret

  ; AVX-fp: v8f32:
  ; AVX-fp: vaddps %ymm1, %ymm0, %ymm0
  ; AVX-fp-NEXT: ret
}

define x86_regcallcc <16 x float> @v16f32(<16 x float> %a, <16 x float> %b) {
  %result = fadd <16 x float> %a, %b
  ret <16 x float> %result
  ; SSE: v16f32:
  ; SSE: addps %xmm4, %xmm0
  ; SSE: addps %xmm5, %xmm1
  ; SSE: addps %xmm6, %xmm2
  ; SSE: addps %xmm7, %xmm3
  ; SSE-NEXT: ret

  ; AVX-fp: v16f32:
  ; AVX-fp: vaddps %ymm2, %ymm0, %ymm0
  ; AVX-fp: vaddps %ymm3, %ymm1, %ymm1
  ; AVX-fp-NEXT: ret
}

; double
define x86_regcallcc <2 x double> @v2f64(<2 x double> %a, <2 x double> %b) {
  %result = fadd <2 x double> %a, %b
  ret <2 x double> %result
  ; SSE: v2f64:
  ; SSE: addpd %xmm1, %xmm0
  ; SSE-NEXT: ret

  ; AVX-fp: v2f64:
  ; AVX-fp: vaddpd %xmm1, %xmm0, %xmm0
  ; AVX-fp-NEXT: ret
}

define x86_regcallcc <4 x double> @v4f64(<4 x double> %a, <4 x double> %b) {
  %result = fadd <4 x double> %a, %b
  ret <4 x double> %result
  ; SSE: v4f64:
  ; SSE: addpd %xmm2, %xmm0
  ; SSE: addpd %xmm3, %xmm1
  ; SSE-NEXT: ret

  ; AVX-fp: v4f64:
  ; AVX-fp: vaddpd %ymm1, %ymm0, %ymm0
  ; AVX-fp-NEXT: ret
}

define x86_regcallcc <8 x double> @v8f64(<8 x double> %a, <8 x double> %b) {
  %result = fadd <8 x double> %a, %b
  ret <8 x double> %result
  ; SSE: v8f64:
  ; SSE: addpd %xmm4, %xmm0
  ; SSE: addpd %xmm5, %xmm1
  ; SSE: addpd %xmm6, %xmm2
  ; SSE: addpd %xmm7, %xmm3
  ; SSE-NEXT: ret

  ; AVX-fp: v8f64:
  ; AVX-fp: vaddpd %ymm2, %ymm0, %ymm0
  ; AVX-fp: vaddpd %ymm3, %ymm1, %ymm1
  ; AVX-fp-NEXT: ret
}

define x86_regcallcc <16 x double> @v16f64(<16 x double> %a, <16 x double> %b) {
  %result = fadd <16 x double> %a, %b
  ret <16 x double> %result
  ; SSE: v16f64:
  ; SSE: addpd %xmm8,  %xmm0
  ; SSE: addpd %xmm9,  %xmm1
  ; SSE: addpd %xmm10, %xmm2
  ; SSE: addpd %xmm11, %xmm3
  ; SSE: addpd %xmm12, %xmm4
  ; SSE: addpd %xmm13, %xmm5
  ; SSE: addpd %xmm14, %xmm6
  ; SSE: addpd %xmm15, %xmm7

  ; AVX-fp: v16f64:
  ; AVX-fp: vaddpd %ymm4, %ymm0, %ymm0
  ; AVX-fp: vaddpd %ymm5, %ymm1, %ymm1
  ; AVX-fp: vaddpd %ymm6, %ymm2, %ymm2
  ; AVX-fp: vaddpd %ymm7, %ymm3, %ymm3
  ; AVX-fp-NEXT: ret
}

define x86_regcallcc <16 x i64> @test_csr(<16 x i64> %a, <16 x i64> %b) {
  %result = add <16 x i64> %a, %b
  ret <16 x i64> %result
  ; SSE: test_csr:
  ; SSE-NOT: pushq
  ; SSE-NOT: popq
  ; SSE: ret
}
