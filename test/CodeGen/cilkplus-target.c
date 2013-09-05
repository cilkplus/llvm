// RUN: %clang_cc1 -fcilkplus -emit-llvm -triple i386-unknown-unknown -target-feature +avx2 %s -o - | FileCheck %s --check-prefix=CHECK1
// RUN: %clang_cc1 -fcilkplus -emit-llvm -triple i386-unknown-unknown -target-feature +avx  %s -o - | FileCheck %s --check-prefix=CHECK2
// RUN: %clang_cc1 -fcilkplus -emit-llvm -triple i386-unknown-unknown -target-feature +sse2 %s -o - | FileCheck %s --check-prefix=CHECK3
// RUN: %clang_cc1 -fcilkplus -emit-llvm -triple i386-unknown-unknown -target-feature +sse  %s -o - | FileCheck %s --check-prefix=CHECK4
// RUN: %clang_cc1 -fcilkplus -emit-llvm -triple i386-unknown-unknown -target-feature +mmx  %s -o - | FileCheck %s --check-prefix=CHECK5
// RUN: %clang_cc1 -fcilkplus -emit-llvm -triple i386-unknown-unknown                       %s -o - | FileCheck %s --check-prefix=CHECK6

__attribute__((vector(nomask))) char f1(void);
__attribute__((vector(nomask))) float f2(void);

void test(void) {
  f1();
  f2();
}

// CHECK1: declare <64 x i8> @_ZGVxN64_f1
// CHECK1: declare <16 x float> @_ZGVxN16_f2
//
// CHECK2: declare <32 x i8> @_ZGVxN32_f1
// CHECK2: declare <8 x float> @_ZGVxN8_f2
//
// CHECK3: declare <16 x i8> @_ZGVxN16_f1
// CHECK3: declare <4 x float> @_ZGVxN4_f2
//
// CHECK4: declare <8 x i8> @_ZGVxN8_f1
// CHECK4: declare <4 x float> @_ZGVxN4_f2
//
// CHECK5: declare <8 x i8> @_ZGVxN8_f1
// CHECK5-NOT: declare {{.*}} @_ZGVxN{{[0-9]+}}_f2
//
// CHECK6-NOT: declare {{.*}} @_ZGVxN{{[0-9]+}}_f1
// CHECK6-NOT: declare {{.*}} @_ZGVxN{{[0-9]+}}_f2
