// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

__attribute__((vector(processor(core_2_duo_sse_4_1),
                      vectorlengthfor(float),
                      vectorlength(8),
                      uniform(w),
                      linear(x:w, y:2),
                      mask)))
void f(int w, int x, int y, int z)
{
}

// CHECK: !cilk.functions = !{{{![0-9]+}}, {{![0-9]+}}, {{![0-9]+}}}

// CHECK-DAG: [[E:![0-9]+]] = metadata !{metadata !"elemental"}
// CHECK-DAG: [[M0:![0-9]+]] = metadata !{metadata !"mask", i1 false}
// CHECK-DAG: [[M1:![0-9]+]] = metadata !{metadata !"mask", i1 true}

// Metadata for f:
// CHECK-DAG: [[FN:![0-9]+]] = metadata !{metadata !"arg_name", metadata !"w", metadata !"x", metadata !"y", metadata !"z"}
// CHECK-DAG: [[FS:![0-9]+]] = metadata !{metadata !"arg_step", i32 0, metadata !"w", i32 2, i32 undef}
// CHECK-DAG: [[FT:![0-9]+]] = metadata !{metadata !"vec_type_hint", <8 x float> undef, i32 0}
// CHECK-DAG: [[FP:![0-9]+]] = metadata !{metadata !"processor", metadata !"core_2_duo_sse_4_1"}
// CHECK-DAG: {{![0-9]+}} = metadata !{void (i32, i32, i32, i32)* @f, metadata [[E]], metadata [[FN]], metadata [[FS]], metadata [[FT]], metadata [[M1]], metadata [[FP]]}

__attribute__((vector))
void g(int x)
{
}

// Metadata for g:
// CHECK-DAG: [[GN:![0-9]+]] = metadata !{metadata !"arg_name", metadata !"x"}
// CHECK-DAG: [[GS:![0-9]+]] = metadata !{metadata !"arg_step", i32 undef}
// CHECK-DAG: [[GT:![0-9]+]] = metadata !{metadata !"vec_type_hint", <4 x i32> undef, i32 1}
// CHECK-DAG: {{![0-9]+}} = metadata !{void (i32)* @g, metadata [[E]], metadata [[GN]], metadata [[GS]], metadata [[GT]], metadata [[M1]]}
// CHECK-DAG: {{![0-9]+}} = metadata !{void (i32)* @g, metadata [[E]], metadata [[GN]], metadata [[GS]], metadata [[GT]], metadata [[M0]]}
