// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

__attribute__((vector(processor(core_2_duo_sse4_1),
                      vectorlength(8),
                      uniform(w),
                      linear(x:w, y:2),
                      mask)))
void f(int w, int x, int y, int z)
{
}

// CHECK: !cilk.functions

// CHECK-DAG: [[E:![0-9]+]] = metadata !{metadata !"elemental"}
// CHECK-DAG: [[M0:![0-9]+]] = metadata !{metadata !"mask", i1 false}
// CHECK-DAG: [[M1:![0-9]+]] = metadata !{metadata !"mask", i1 true}

// Metadata for f:
// CHECK-DAG: [[FN:![0-9]+]] = metadata !{metadata !"arg_name", metadata !"w", metadata !"x", metadata !"y", metadata !"z"}
// CHECK-DAG: [[FS:![0-9]+]] = metadata !{metadata !"arg_step", i32 0, metadata !"w", i32 2, i32 undef}
// CHECK-DAG: [[FT:![0-9]+]] = metadata !{metadata !"vec_length", i32 undef, i32 {{[0-9]+}}}
// CHECK-DAG: [[FP:![0-9]+]] = metadata !{metadata !"processor", metadata !"core_2_duo_sse4_1"}
// CHECK-DAG: [[FV:![0-9]+]] = metadata !{metadata !"variant", void (i32, i32, i32, <8 x i32>, <8 x i32>)* @_ZGVxM8us0l2v_f}
// CHECK-DAG: {{![0-9]+}} = metadata !{void (i32, i32, i32, i32)* @f, metadata [[E]], metadata [[FN]], metadata [[FS]], metadata [[FT]], metadata [[M1]], metadata [[FP]], metadata [[FV]]}

__attribute__((vector))
void g(float x)
{
}

// Metadata for g:
// CHECK-DAG: [[GN:![0-9]+]] = metadata !{metadata !"arg_name", metadata !"x"}
// CHECK-DAG: [[GS:![0-9]+]] = metadata !{metadata !"arg_step", i32 undef}
// CHECK-DAG: [[GT:![0-9]+]] = metadata !{metadata !"vec_length", float undef, i32 {{[0-9]+}}}
// CHECK-DAG: [[GV1:![0-9]+]] = metadata !{metadata !"variant", void (<4 x float>, <4 x float>)* @_ZGVxM4v_g}
// CHECK-DAG: [[GV2:![0-9]+]] = metadata !{metadata !"variant", void (<4 x float>)* @_ZGVxN4v_g}
// CHECK-DAG: {{![0-9]+}} = metadata !{void (float)* @g, metadata [[E]], metadata [[GN]], metadata [[GS]], metadata [[GT]], metadata [[M1]], metadata [[GV1]]}
// CHECK-DAG: {{![0-9]+}} = metadata !{void (float)* @g, metadata [[E]], metadata [[GN]], metadata [[GS]], metadata [[GT]], metadata [[M0]], metadata [[GV2]]}

__attribute__((vector))
void h(char *hp)
{
}

// Metadata for h:
// CHECK-DAG: metadata !{void (i8*)* @h,
// CHECK-DAG: metadata !{metadata !"arg_name", metadata !"hp"}
// CHECK-DAG: metadata !{metadata !"vec_length", i8* undef, i32 {{[0-9]+}}}

__attribute__((vector))
void k(char (*kf)(int), int a)
{
  kf(a);
}

// Metadata for k:
// CHECK-DAG: metadata !{void (i8 (i32)*, i32)* @k,
// CHECK-DAG: metadata !{metadata !"arg_name", metadata !"kf", metadata !"a"}
// CHECK-DAG: metadata !{metadata !"arg_step", i32 undef, i32 undef}
// CHECK-DAG: metadata !{metadata !"vec_length", i8 (i32)* undef, i32 {{[0-9]+}}}
