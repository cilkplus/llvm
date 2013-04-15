// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

void test(void) {
  int j;
  _Cilk_for (int i = 3; i < 10; i++) {
    (void)j;
  }
  // CHECK: store i32 3, i32* [[i:%[a-zA-Z0-9_]*]]
  // CHECK: [[Cond:%[a-zA-Z0-9_]*]] = icmp slt i32 {{.*}}, 10
  // CHECK: br i1 [[Cond]]
  //
  // Captures: i, j
  // CHECK: getelementptr inbounds
  // CHECK: store i32* [[i]],
  // CHECK: getelementptr inbounds
  // CHECK: store i32*
  //
  // TODO: loop count
  //
  // CHECK: call void @__cilkrts_cilk_for_32

  _Cilk_for (long long i = 2; i < 12; i++) { }
  // CHECK: store i64 2, i64*
  // CHECK: icmp slt i64 {{.*}}, 12
  //
  // TODO: loop count
  //
  // CHECK: call void @__cilkrts_cilk_for_64

  int foo(void);
  int bar(void);
  _Cilk_for (int i = foo(); i < bar(); i++) { }
  // CHECK: [[Init:%[a-zA-Z0-9_]*]] = call i32 @foo()
  // CHECK: store i32 [[Init]], i32*
  // CHECK: [[Limit:%[a-zA-Z0-9_]*]] = call i32 @bar()
  // CHECK: icmp slt i32 {{.*}}, [[Limit]]
  //
  // CHECK: call void @__cilkrts_cilk_for_32
}
