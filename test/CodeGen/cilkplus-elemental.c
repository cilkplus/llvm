// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

__attribute__((vector)) float f0(float x) { return x; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4v_f0(<4 x float> %x, <4 x float> %mask)
// CHECK: define x86_regcallcc <4 x float> @_ZGVxN4v_f0(<4 x float> %x)

__attribute__((vector(nomask))) float f1(float x) { return x; }
// CHECK-NOT: define x86_regcallcc <4 x float> @_ZGVxM4v_f1(<4 x float> %x, <4 x float> %mask)
// CHECK: define x86_regcallcc <4 x float> @_ZGVxN4v_f1(<4 x float> %x)
__attribute__((vector(mask))) float f2(float x) { return x; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4v_f2(<4 x float> %x, <4 x float> %mask)
// CHECK-NOT: define x86_regcallcc <4 x float> @_ZGVxN4v_f2(<4 x float> %x)

__attribute__((vector(mask, vectorlength(8)))) float f3(float x) { return x; }
// CHECK: define x86_regcallcc <8 x float> @_ZGVxM8v_f3(<8 x float> %x, <8 x float> %mask)

__attribute__((vector(mask, uniform(y)))) float f4(float x, float y) { return x + y; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4vu_f4(<4 x float> %x, float %y, <4 x float> %mask)

__attribute__((vector(mask, linear(y:1)))) float f5(float x, int y) { return x + y; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4vl_f5(<4 x float> %x, i32 %y, <4 x float> %mask)
__attribute__((vector(mask, linear(y:2)))) float f6(float x, int y) { return x + y; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4vl2_f6(<4 x float> %x, i32 %y, <4 x float> %mask)
__attribute__((vector(mask, linear(y:s), uniform(s)))) float f7(float x, int y, int s) { return x + y; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4vs2u_f7(<4 x float> %x, i32 %y, i32 %s, <4 x float> %mask)

__attribute__((vector(processor(pentium_4)))) float f8(float x) { return x; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4v_f8(<4 x float> %x, <4 x float> %mask)
__attribute__((vector(processor(pentium_4_sse3)))) float f9(float x) { return x; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4v_f9(<4 x float> %x, <4 x float> %mask)
__attribute__((vector(processor(core_2_duo_ssse3)))) float f10(float x) { return x; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4v_f10(<4 x float> %x, <4 x float> %mask)
__attribute__((vector(processor(core_2_duo_sse4_1)))) float f11(float x) { return x; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4v_f11(<4 x float> %x, <4 x float> %mask)
__attribute__((vector(processor(core_i7_sse4_2)))) float f12(float x) { return x; }
// CHECK: define x86_regcallcc <4 x float> @_ZGVxM4v_f12(<4 x float> %x, <4 x float> %mask)
__attribute__((vector(processor(core_2nd_gen_avx)))) float f13(float x) { return x; }
// CHECK: define x86_regcallcc <8 x float> @_ZGVyM8v_f13(<8 x float> %x, <8 x float> %mask)
__attribute__((vector(processor(core_3rd_gen_avx)))) float f14(float x) { return x; }
// CHECK: define x86_regcallcc <8 x float> @_ZGVyM8v_f14(<8 x float> %x, <8 x float> %mask)
__attribute__((vector(processor(core_4th_gen_avx)))) float f15(float x) { return x; }
// CHECK: define x86_regcallcc <8 x float> @_ZGVYM8v_f15(<8 x float> %x, <8 x float> %mask)

__attribute__((vector)) double f16(float x) { return x; }
// CHECK: define x86_regcallcc <2 x double> @_ZGVxM2v_f16(<2 x float> %x, <2 x double> %mask)
