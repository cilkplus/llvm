// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

void test1() {
// CHECK: void @__cilk_for_helper
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !0
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !0
// CHECK:   load i32* %__cv_i, align 4, !llvm.mem.parallel_loop_access !0
// CHECK:   store i32 %{{.*}}, i32* %__cv_i, align 4, !llvm.mem.parallel_loop_access !0
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !0
// CHECK:   store i32 %{{.*}}, i32* %{{.*}}, !llvm.mem.parallel_loop_access !0
// CHECK:   br label %{{.*}}, !llvm.loop !0
// CHECK:   ret void
  _Cilk_for (int i = 0; i < 10; ++i)
    ;
}

void test2() {
// CHECK: void @__cilk_for_helper1
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !1
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !1
// CHECK:   store i32 0, i32* %{{.*}}, align 4, !llvm.mem.parallel_loop_access !1
// CHECK:   load i32* %{{.*}}, align 4, !llvm.mem.parallel_loop_access !1
// CHECK:   load i32* %{{.*}}, align 4, !llvm.mem.parallel_loop_access !1
// CHECK:   store i32* %{{.*}}, i32** %{{.*}}, align 8, !llvm.mem.parallel_loop_access !1
// CHECK:   load i32* %__cv_i, align 4, !llvm.mem.parallel_loop_access !1
// CHECK:   store i32 %{{.*}}, i32* %__cv_i, align 4, !llvm.mem.parallel_loop_access !1
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !1
// CHECK:   store i32 %{{.*}}, i32* %{{.*}}, !llvm.mem.parallel_loop_access !1
// CHECK:   br label %{{.*}}, !llvm.loop !1
// CHECK:   ret void
  _Cilk_for (int i = 0; i < 10; ++i)
// CHECK: void @__cilk_for_helper2
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !2
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !2
// CHECK:   load i32* %__cv_j, align 4, !llvm.mem.parallel_loop_access !2
// CHECK:   store i32 %{{.*}}, i32* %__cv_j, align 4, !llvm.mem.parallel_loop_access !2
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !2
// CHECK:   store i32 %{{.*}}, i32* %{{.*}}, !llvm.mem.parallel_loop_access !2
// CHECK:   br label %{{.*}}, !llvm.loop !2
// CHECK:   ret void
    _Cilk_for (int j = 0; j < 10; ++j)
      ;
}

void test3() {
// CHECK: void @__cilk_for_helper3
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !3
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !3
// CHECK:   store i32 0, i32* %{{.*}}, align 4, !llvm.mem.parallel_loop_access !3
// CHECK-NOT: !llvm.mem.parallel_loop_access
// CHECK-NOT: !llvm.mem.parallel_loop_access
// CHECK-NOT: !llvm.loop
// CHECK-NOT: !llvm.loop
// CHECK:   load i32* %__cv_i, align 4, !llvm.mem.parallel_loop_access !3
// CHECK:   store i32 %{{.*}}, i32* %__cv_i, align 4, !llvm.mem.parallel_loop_access !3
// CHECK:   load i32* %{{.*}}, !llvm.mem.parallel_loop_access !3
// CHECK:   store i32 %{{.*}}, i32* %{{.*}}, !llvm.mem.parallel_loop_access !3
// CHECK:   br label %{{.*}}, !llvm.loop !3
// CHECK:   ret void
  _Cilk_for (int i = 0; i < 10; ++i)
    for (int j = 0; j < 10; ++j)
      ;
}
