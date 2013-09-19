; RUN: llc < %s -march=x86-64 -mcpu=corei7 | FileCheck -check-prefix=SSE %s

define x86_regcallcc <2 x i8> @v2i8(<2 x i8> %a, <2 x i8> %b) {
  %result = add <2 x i8> %a, %b
  ret <2 x i8> %result
  ; SSE: v2i8:
  ; SSE: paddq %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <4 x i8> @v4i8(<4 x i8> %a, <4 x i8> %b) {
  %result = add <4 x i8> %a, %b
  ret <4 x i8> %result
  ; SSE: v4i8:
  ; SSE: paddd %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <8 x i8> @v8i8(<8 x i8> %a, <8 x i8> %b) {
  %result = add <8 x i8> %a, %b
  ret <8 x i8> %result
  ; SSE: v8i8:
  ; SSE: paddw %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <16 x i8> @v16i8(<16 x i8> %a, <16 x i8> %b) {
  %result = add <16 x i8> %a, %b
  ret <16 x i8> %result
  ; SSE: v16i8:
  ; SSE: paddb %xmm1, %xmm0
  ; SSE-NEXT: ret
}

; i16
define x86_regcallcc <2 x i16> @v2i16(<2 x i16> %a, <2 x i16> %b) {
  %result = add <2 x i16> %a, %b
  ret <2 x i16> %result
  ; SSE: v2i16:
  ; SSE: paddq %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <4 x i16> @v4i16(<4 x i16> %a, <4 x i16> %b) {
  %result = add <4 x i16> %a, %b
  ret <4 x i16> %result
  ; SSE: v4i16:
  ; SSE: paddd %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <8 x i16> @v8i16(<8 x i16> %a, <8 x i16> %b) {
  %result = add <8 x i16> %a, %b
  ret <8 x i16> %result
  ; SSE: v8i16:
  ; SSE: paddw %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <16 x i16> @v16i16(<16 x i16> %a, <16 x i16> %b) {
  %result = add <16 x i16> %a, %b
  ret <16 x i16> %result
  ; SSE: v16i16:
  ; SSE: paddw %xmm2, %xmm0
  ; SSE: paddw %xmm3, %xmm1
  ; SSE-NEXT: ret
}

; i32
define x86_regcallcc <2 x i32> @v2i32(<2 x i32> %a, <2 x i32> %b) {
  %result = add <2 x i32> %a, %b
  ret <2 x i32> %result
  ; SSE: v2i32:
  ; SSE: paddq %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <4 x i32> @v4i32(<4 x i32> %a, <4 x i32> %b) {
  %result = add <4 x i32> %a, %b
  ret <4 x i32> %result
  ; SSE: v4i32:
  ; SSE: paddd %xmm1, %xmm0
  ; SSE-NEXT: ret
}

define x86_regcallcc <8 x i32> @v8i32(<8 x i32> %a, <8 x i32> %b) {
  %result = add <8 x i32> %a, %b
  ret <8 x i32> %result
  ; SSE: v8i32:
  ; SSE: paddd %xmm2, %xmm0
  ; SSE: paddd %xmm3, %xmm1
  ; SSE-NEXT: ret
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
}

define x86_regcallcc <16 x i64> @test_csr(<16 x i64> %a, <16 x i64> %b) {
  %result = add <16 x i64> %a, %b
  ret <16 x i64> %result
  ; SSE: test_csr:
  ; SSE-NOT: pushq
  ; SSE-NOT: popq
  ; SSE: ret
}
