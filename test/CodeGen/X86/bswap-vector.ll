; RUN: llc < %s -mcpu=x86-64 | FileCheck %s --check-prefix=CHECK-ALL --check-prefix=CHECK-SSE --check-prefix=CHECK-NOSSSE3
; RUN: llc < %s -mcpu=core2 | FileCheck %s --check-prefix=CHECK-ALL --check-prefix=CHECK-SSE --check-prefix=CHECK-SSSE3
; RUN: llc < %s -mcpu=core-avx2 | FileCheck %s --check-prefix=CHECK-AVX --check-prefix=CHECK-AVX2
; RUN: llc < %s -mcpu=core-avx2 -x86-experimental-vector-widening-legalization | FileCheck %s --check-prefix=CHECK-WIDE-AVX2

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare <8 x i16> @llvm.bswap.v8i16(<8 x i16>)
declare <4 x i32> @llvm.bswap.v4i32(<4 x i32>)
declare <2 x i64> @llvm.bswap.v2i64(<2 x i64>)

define <8 x i16> @test1(<8 x i16> %v) {
; CHECK-NOSSSE3-LABEL: test1:
; CHECK-NOSSSE3:       # BB#0: # %entry
; CHECK-NOSSSE3-NEXT:    pxor %xmm1, %xmm1
; CHECK-NOSSSE3-NEXT:    movdqa %xmm0, %xmm2
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm2 = xmm2[8],xmm1[8],xmm2[9],xmm1[9],xmm2[10],xmm1[10],xmm2[11],xmm1[11],xmm2[12],xmm1[12],xmm2[13],xmm1[13],xmm2[14],xmm1[14],xmm2[15],xmm1[15]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm2 = xmm2[1,0,3,2,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm2 = xmm2[0,1,2,3,5,4,7,6]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[1,0,3,2,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,5,4,7,6]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm2, %xmm0
; CHECK-NOSSSE3-NEXT:    retq
;
; CHECK-SSSE3-LABEL: test1:
; CHECK-SSSE3:       # BB#0: # %entry
; CHECK-SSSE3-NEXT:    pshufb {{.*#+}} xmm0 = xmm0[1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14]
; CHECK-SSSE3-NEXT:    retq
;
; CHECK-AVX2-LABEL: test1:
; CHECK-AVX2:       # BB#0: # %entry
; CHECK-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14]
; CHECK-AVX2-NEXT:    retq
;
; CHECK-WIDE-AVX2-LABEL: test1:
; CHECK-WIDE-AVX2:       # BB#0: # %entry
; CHECK-WIDE-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14]
; CHECK-WIDE-AVX2-NEXT:    retq
entry:
  %r = call <8 x i16> @llvm.bswap.v8i16(<8 x i16> %v)
  ret <8 x i16> %r
}

define <4 x i32> @test2(<4 x i32> %v) {
; CHECK-NOSSSE3-LABEL: test2:
; CHECK-NOSSSE3:       # BB#0: # %entry
; CHECK-NOSSSE3-NEXT:    pxor %xmm1, %xmm1
; CHECK-NOSSSE3-NEXT:    movdqa %xmm0, %xmm2
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm2 = xmm2[8],xmm1[8],xmm2[9],xmm1[9],xmm2[10],xmm1[10],xmm2[11],xmm1[11],xmm2[12],xmm1[12],xmm2[13],xmm1[13],xmm2[14],xmm1[14],xmm2[15],xmm1[15]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm2 = xmm2[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm2 = xmm2[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm2, %xmm0
; CHECK-NOSSSE3-NEXT:    retq
;
; CHECK-SSSE3-LABEL: test2:
; CHECK-SSSE3:       # BB#0: # %entry
; CHECK-SSSE3-NEXT:    pshufb {{.*#+}} xmm0 = xmm0[3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
; CHECK-SSSE3-NEXT:    retq
;
; CHECK-AVX2-LABEL: test2:
; CHECK-AVX2:       # BB#0: # %entry
; CHECK-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
; CHECK-AVX2-NEXT:    retq
;
; CHECK-WIDE-AVX2-LABEL: test2:
; CHECK-WIDE-AVX2:       # BB#0: # %entry
; CHECK-WIDE-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
; CHECK-WIDE-AVX2-NEXT:    retq
entry:
  %r = call <4 x i32> @llvm.bswap.v4i32(<4 x i32> %v)
  ret <4 x i32> %r
}

define <2 x i64> @test3(<2 x i64> %v) {
; CHECK-NOSSSE3-LABEL: test3:
; CHECK-NOSSSE3:       # BB#0: # %entry
; CHECK-NOSSSE3-NEXT:    pxor %xmm1, %xmm1
; CHECK-NOSSSE3-NEXT:    movdqa %xmm0, %xmm2
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm2 = xmm2[8],xmm1[8],xmm2[9],xmm1[9],xmm2[10],xmm1[10],xmm2[11],xmm1[11],xmm2[12],xmm1[12],xmm2[13],xmm1[13],xmm2[14],xmm1[14],xmm2[15],xmm1[15]
; CHECK-NOSSSE3-NEXT:    pshufd {{.*#+}} xmm2 = xmm2[2,3,0,1]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm2 = xmm2[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm2 = xmm2[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; CHECK-NOSSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm2, %xmm0
; CHECK-NOSSSE3-NEXT:    retq
;
; CHECK-SSSE3-LABEL: test3:
; CHECK-SSSE3:       # BB#0: # %entry
; CHECK-SSSE3-NEXT:    pshufb {{.*#+}} xmm0 = xmm0[7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8]
; CHECK-SSSE3-NEXT:    retq
;
; CHECK-AVX2-LABEL: test3:
; CHECK-AVX2:       # BB#0: # %entry
; CHECK-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8]
; CHECK-AVX2-NEXT:    retq
;
; CHECK-WIDE-AVX2-LABEL: test3:
; CHECK-WIDE-AVX2:       # BB#0: # %entry
; CHECK-WIDE-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8]
; CHECK-WIDE-AVX2-NEXT:    retq
entry:
  %r = call <2 x i64> @llvm.bswap.v2i64(<2 x i64> %v)
  ret <2 x i64> %r
}

declare <16 x i16> @llvm.bswap.v16i16(<16 x i16>)
declare <8 x i32> @llvm.bswap.v8i32(<8 x i32>)
declare <4 x i64> @llvm.bswap.v4i64(<4 x i64>)

define <16 x i16> @test4(<16 x i16> %v) {
; CHECK-NOSSSE3-LABEL: test4:
; CHECK-NOSSSE3:       # BB#0: # %entry
; CHECK-NOSSSE3-NEXT:    pxor %xmm2, %xmm2
; CHECK-NOSSSE3-NEXT:    movdqa %xmm0, %xmm3
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm3 = xmm3[1,0,3,2,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm3 = xmm3[0,1,2,3,5,4,7,6]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[1,0,3,2,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,5,4,7,6]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm3, %xmm0
; CHECK-NOSSSE3-NEXT:    movdqa %xmm1, %xmm3
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm3 = xmm3[1,0,3,2,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm3 = xmm3[0,1,2,3,5,4,7,6]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3],xmm1[4],xmm2[4],xmm1[5],xmm2[5],xmm1[6],xmm2[6],xmm1[7],xmm2[7]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm1 = xmm1[1,0,3,2,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm1 = xmm1[0,1,2,3,5,4,7,6]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm3, %xmm1
; CHECK-NOSSSE3-NEXT:    retq
;
; CHECK-SSSE3-LABEL: test4:
; CHECK-SSSE3:       # BB#0: # %entry
; CHECK-SSSE3-NEXT:    movdqa {{.*#+}} xmm2 = [1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14]
; CHECK-SSSE3-NEXT:    pshufb %xmm2, %xmm0
; CHECK-SSSE3-NEXT:    pshufb %xmm2, %xmm1
; CHECK-SSSE3-NEXT:    retq
;
; CHECK-AVX2-LABEL: test4:
; CHECK-AVX2:       # BB#0: # %entry
; CHECK-AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14,17,16,19,18,21,20,23,22,25,24,27,26,29,28,31,30]
; CHECK-AVX2-NEXT:    retq
;
; CHECK-WIDE-AVX2-LABEL: test4:
; CHECK-WIDE-AVX2:       # BB#0: # %entry
; CHECK-WIDE-AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14,17,16,19,18,21,20,23,22,25,24,27,26,29,28,31,30]
; CHECK-WIDE-AVX2-NEXT:    retq
entry:
  %r = call <16 x i16> @llvm.bswap.v16i16(<16 x i16> %v)
  ret <16 x i16> %r
}

define <8 x i32> @test5(<8 x i32> %v) {
; CHECK-NOSSSE3-LABEL: test5:
; CHECK-NOSSSE3:       # BB#0: # %entry
; CHECK-NOSSSE3-NEXT:    pxor %xmm2, %xmm2
; CHECK-NOSSSE3-NEXT:    movdqa %xmm0, %xmm3
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm3 = xmm3[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm3 = xmm3[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm3, %xmm0
; CHECK-NOSSSE3-NEXT:    movdqa %xmm1, %xmm3
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm3 = xmm3[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm3 = xmm3[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3],xmm1[4],xmm2[4],xmm1[5],xmm2[5],xmm1[6],xmm2[6],xmm1[7],xmm2[7]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm1 = xmm1[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm1 = xmm1[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm3, %xmm1
; CHECK-NOSSSE3-NEXT:    retq
;
; CHECK-SSSE3-LABEL: test5:
; CHECK-SSSE3:       # BB#0: # %entry
; CHECK-SSSE3-NEXT:    movdqa {{.*#+}} xmm2 = [3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
; CHECK-SSSE3-NEXT:    pshufb %xmm2, %xmm0
; CHECK-SSSE3-NEXT:    pshufb %xmm2, %xmm1
; CHECK-SSSE3-NEXT:    retq
;
; CHECK-AVX2-LABEL: test5:
; CHECK-AVX2:       # BB#0: # %entry
; CHECK-AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28]
; CHECK-AVX2-NEXT:    retq
;
; CHECK-WIDE-AVX2-LABEL: test5:
; CHECK-WIDE-AVX2:       # BB#0: # %entry
; CHECK-WIDE-AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12,19,18,17,16,23,22,21,20,27,26,25,24,31,30,29,28]
; CHECK-WIDE-AVX2-NEXT:    retq
entry:
  %r = call <8 x i32> @llvm.bswap.v8i32(<8 x i32> %v)
  ret <8 x i32> %r
}

define <4 x i64> @test6(<4 x i64> %v) {
; CHECK-NOSSSE3-LABEL: test6:
; CHECK-NOSSSE3:       # BB#0: # %entry
; CHECK-NOSSSE3-NEXT:    pxor %xmm2, %xmm2
; CHECK-NOSSSE3-NEXT:    movdqa %xmm0, %xmm3
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; CHECK-NOSSSE3-NEXT:    pshufd {{.*#+}} xmm3 = xmm3[2,3,0,1]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm3 = xmm3[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm3 = xmm3[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
; CHECK-NOSSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm3, %xmm0
; CHECK-NOSSSE3-NEXT:    movdqa %xmm1, %xmm3
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; CHECK-NOSSSE3-NEXT:    pshufd {{.*#+}} xmm3 = xmm3[2,3,0,1]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm3 = xmm3[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm3 = xmm3[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3],xmm1[4],xmm2[4],xmm1[5],xmm2[5],xmm1[6],xmm2[6],xmm1[7],xmm2[7]
; CHECK-NOSSSE3-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,3,0,1]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm1 = xmm1[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm1 = xmm1[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm3, %xmm1
; CHECK-NOSSSE3-NEXT:    retq
;
; CHECK-SSSE3-LABEL: test6:
; CHECK-SSSE3:       # BB#0: # %entry
; CHECK-SSSE3-NEXT:    movdqa {{.*#+}} xmm2 = [7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8]
; CHECK-SSSE3-NEXT:    pshufb %xmm2, %xmm0
; CHECK-SSSE3-NEXT:    pshufb %xmm2, %xmm1
; CHECK-SSSE3-NEXT:    retq
;
; CHECK-AVX2-LABEL: test6:
; CHECK-AVX2:       # BB#0: # %entry
; CHECK-AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24]
; CHECK-AVX2-NEXT:    retq
;
; CHECK-WIDE-AVX2-LABEL: test6:
; CHECK-WIDE-AVX2:       # BB#0: # %entry
; CHECK-WIDE-AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24]
; CHECK-WIDE-AVX2-NEXT:    retq
entry:
  %r = call <4 x i64> @llvm.bswap.v4i64(<4 x i64> %v)
  ret <4 x i64> %r
}

declare <4 x i16> @llvm.bswap.v4i16(<4 x i16>)

define <4 x i16> @test7(<4 x i16> %v) {
; CHECK-NOSSSE3-LABEL: test7:
; CHECK-NOSSSE3:       # BB#0: # %entry
; CHECK-NOSSSE3-NEXT:    pxor %xmm1, %xmm1
; CHECK-NOSSSE3-NEXT:    movdqa %xmm0, %xmm2
; CHECK-NOSSSE3-NEXT:    punpckhbw {{.*#+}} xmm2 = xmm2[8],xmm1[8],xmm2[9],xmm1[9],xmm2[10],xmm1[10],xmm2[11],xmm1[11],xmm2[12],xmm1[12],xmm2[13],xmm1[13],xmm2[14],xmm1[14],xmm2[15],xmm1[15]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm2 = xmm2[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm2 = xmm2[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; CHECK-NOSSSE3-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[3,2,1,0,4,5,6,7]
; CHECK-NOSSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,7,6,5,4]
; CHECK-NOSSSE3-NEXT:    packuswb %xmm2, %xmm0
; CHECK-NOSSSE3-NEXT:    psrld $16, %xmm0
; CHECK-NOSSSE3-NEXT:    retq
;
; CHECK-SSSE3-LABEL: test7:
; CHECK-SSSE3:       # BB#0: # %entry
; CHECK-SSSE3-NEXT:    pshufb {{.*#+}} xmm0 = xmm0[3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
; CHECK-SSSE3-NEXT:    psrld $16, %xmm0
; CHECK-SSSE3-NEXT:    retq
;
; CHECK-AVX2-LABEL: test7:
; CHECK-AVX2:       # BB#0: # %entry
; CHECK-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12]
; CHECK-AVX2-NEXT:    vpsrld $16, %xmm0, %xmm0
; CHECK-AVX2-NEXT:    retq
;
; CHECK-WIDE-AVX2-LABEL: test7:
; CHECK-WIDE-AVX2:       # BB#0: # %entry
; CHECK-WIDE-AVX2-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14]
; CHECK-WIDE-AVX2-NEXT:    retq
entry:
  %r = call <4 x i16> @llvm.bswap.v4i16(<4 x i16> %v)
  ret <4 x i16> %r
}

;
; Double BSWAP -> Identity
;

define <8 x i16> @identity_v8i16(<8 x i16> %v) {
; CHECK-ALL-LABEL: identity_v8i16:
; CHECK-ALL:       # BB#0: # %entry
; CHECK-ALL:         retq
entry:
  %bs1 = call <8 x i16> @llvm.bswap.v8i16(<8 x i16> %v)
  %bs2 = call <8 x i16> @llvm.bswap.v8i16(<8 x i16> %bs1)
  ret <8 x i16> %bs2
}

define <4 x i32> @identity_v4i32(<4 x i32> %v) {
; CHECK-ALL-LABEL: identity_v4i32:
; CHECK-ALL:       # BB#0: # %entry
; CHECK-ALL-NEXT:    retq
entry:
  %bs1 = call <4 x i32> @llvm.bswap.v4i32(<4 x i32> %v)
  %bs2 = call <4 x i32> @llvm.bswap.v4i32(<4 x i32> %bs1)
  ret <4 x i32> %bs2
}

define <2 x i64> @identity_v2i64(<2 x i64> %v) {
; CHECK-ALL-LABEL: identity_v2i64:
; CHECK-ALL:       # BB#0: # %entry
; CHECK-ALL-NEXT:    retq
entry:
  %bs1 = call <2 x i64> @llvm.bswap.v2i64(<2 x i64> %v)
  %bs2 = call <2 x i64> @llvm.bswap.v2i64(<2 x i64> %bs1)
  ret <2 x i64> %bs2
}

define <16 x i16> @identity_v16i16(<16 x i16> %v) {
; CHECK-ALL-LABEL: identity_v16i16:
; CHECK-ALL:       # BB#0: # %entry
; CHECK-ALL-NEXT:    retq
entry:
  %bs1 = call <16 x i16> @llvm.bswap.v16i16(<16 x i16> %v)
  %bs2 = call <16 x i16> @llvm.bswap.v16i16(<16 x i16> %bs1)
  ret <16 x i16> %bs2
}

define <8 x i32> @identity_v8i32(<8 x i32> %v) {
; CHECK-ALL-LABEL: identity_v8i32:
; CHECK-ALL:       # BB#0: # %entry
; CHECK-ALL-NEXT:    retq
entry:
  %bs1 = call <8 x i32> @llvm.bswap.v8i32(<8 x i32> %v)
  %bs2 = call <8 x i32> @llvm.bswap.v8i32(<8 x i32> %bs1)
  ret <8 x i32> %bs2
}

define <4 x i64> @identity_v4i64(<4 x i64> %v) {
; CHECK-ALL-LABEL: identity_v4i64:
; CHECK-ALL:       # BB#0: # %entry
; CHECK-ALL-NEXT:    retq
entry:
  %bs1 = call <4 x i64> @llvm.bswap.v4i64(<4 x i64> %v)
  %bs2 = call <4 x i64> @llvm.bswap.v4i64(<4 x i64> %bs1)
  ret <4 x i64> %bs2
}

define <4 x i16> @identity_v4i16(<4 x i16> %v) {
; CHECK-ALL-LABEL: identity_v4i16:
; CHECK-ALL:       # BB#0: # %entry
; CHECK-ALL-NEXT:    retq
entry:
  %bs1 = call <4 x i16> @llvm.bswap.v4i16(<4 x i16> %v)
  %bs2 = call <4 x i16> @llvm.bswap.v4i16(<4 x i16> %bs1)
  ret <4 x i16> %bs2
}

;
; Constant Folding
;

define <8 x i16> @fold_v8i16() {
; CHECK-SSE-LABEL: fold_v8i16:
; CHECK-SSE:       # BB#0: # %entry
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm0 = [0,256,65535,512,65023,1024,64511,1536]
; CHECK-SSE-NEXT:    retq
;
; CHECK-AVX-LABEL: fold_v8i16:
; CHECK-AVX:       # BB#0: # %entry
; CHECK-AVX-NEXT:    vmovaps {{.*#+}} xmm0 = [0,256,65535,512,65023,1024,64511,1536]
; CHECK-AVX-NEXT:    retq
entry:
  %r = call <8 x i16> @llvm.bswap.v8i16(<8 x i16> <i16 0, i16 1, i16 -1, i16 2, i16 -3, i16 4, i16 -5, i16 6>)
  ret <8 x i16> %r
}

define <4 x i32> @fold_v4i32() {
; CHECK-SSE-LABEL: fold_v4i32:
; CHECK-SSE:       # BB#0: # %entry
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm0 = [0,4294967295,33554432,4261412863]
; CHECK-SSE-NEXT:    retq
;
; CHECK-AVX-LABEL: fold_v4i32:
; CHECK-AVX:       # BB#0: # %entry
; CHECK-AVX-NEXT:    vmovaps {{.*#+}} xmm0 = [0,4294967295,33554432,4261412863]
; CHECK-AVX-NEXT:    retq
entry:
  %r = call <4 x i32> @llvm.bswap.v4i32(<4 x i32> <i32 0, i32 -1, i32 2, i32 -3>)
  ret <4 x i32> %r
}

define <2 x i64> @fold_v2i64() {
; CHECK-SSE-LABEL: fold_v2i64:
; CHECK-SSE:       # BB#0: # %entry
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm0 = [18374686479671623680,18446744073709551615]
; CHECK-SSE-NEXT:    retq
;
; CHECK-AVX-LABEL: fold_v2i64:
; CHECK-AVX:       # BB#0: # %entry
; CHECK-AVX-NEXT:    vmovaps {{.*#+}} xmm0 = [18374686479671623680,18446744073709551615]
; CHECK-AVX-NEXT:    retq
entry:
  %r = call <2 x i64> @llvm.bswap.v2i64(<2 x i64> <i64 255, i64 -1>)
  ret <2 x i64> %r
}

define <16 x i16> @fold_v16i16() {
; CHECK-SSE-LABEL: fold_v16i16:
; CHECK-SSE:       # BB#0: # %entry
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm0 = [0,256,65535,512,65023,1024,64511,1536]
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm1 = [63999,2048,63487,2560,62975,3072,62463,3584]
; CHECK-SSE-NEXT:    retq
;
; CHECK-AVX-LABEL: fold_v16i16:
; CHECK-AVX:       # BB#0: # %entry
; CHECK-AVX-NEXT:    vmovaps {{.*#+}} ymm0 = [0,256,65535,512,65023,1024,64511,1536,63999,2048,63487,2560,62975,3072,62463,3584]
; CHECK-AVX-NEXT:    retq
entry:
  %r = call <16 x i16> @llvm.bswap.v16i16(<16 x i16> <i16 0, i16 1, i16 -1, i16 2, i16 -3, i16 4, i16 -5, i16 6, i16 -7, i16 8, i16 -9, i16 10, i16 -11, i16 12, i16 -13, i16 14>)
  ret <16 x i16> %r
}

define <8 x i32> @fold_v8i32() {
; CHECK-SSE-LABEL: fold_v8i32:
; CHECK-SSE:       # BB#0: # %entry
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm0 = [0,16777216,4294967295,33554432]
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm1 = [4261412863,67108864,4227858431,100663296]
; CHECK-SSE-NEXT:    retq
;
; CHECK-AVX-LABEL: fold_v8i32:
; CHECK-AVX:       # BB#0: # %entry
; CHECK-AVX-NEXT:    vmovaps {{.*#+}} ymm0 = [0,16777216,4294967295,33554432,4261412863,67108864,4227858431,100663296]
; CHECK-AVX-NEXT:    retq
entry:
  %r = call <8 x i32> @llvm.bswap.v8i32(<8 x i32> <i32 0, i32 1, i32 -1, i32 2, i32 -3, i32 4, i32 -5, i32 6>)
  ret <8 x i32> %r
}

define <4 x i64> @fold_v4i64() {
; CHECK-SSE-LABEL: fold_v4i64:
; CHECK-SSE:       # BB#0: # %entry
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm0 = [18374686479671623680,18446744073709551615]
; CHECK-SSE-NEXT:    movaps {{.*#+}} xmm1 = [18446462598732840960,72056494526300160]
; CHECK-SSE-NEXT:    retq
;
; CHECK-AVX-LABEL: fold_v4i64:
; CHECK-AVX:       # BB#0: # %entry
; CHECK-AVX-NEXT:    vmovaps {{.*#+}} ymm0 = [18374686479671623680,18446744073709551615,18446462598732840960,72056494526300160]
; CHECK-AVX-NEXT:    retq
entry:
  %r = call <4 x i64> @llvm.bswap.v4i64(<4 x i64> <i64 255, i64 -1, i64 65535, i64 16776960>)
  ret <4 x i64> %r
}
