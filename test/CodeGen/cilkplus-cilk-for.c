// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o %t
// RUN: FileCheck -input-file=%t -check-prefix=CHECK1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK2 %s

void test1(void) {
  // CHECK1: test1
  int j;
  _Cilk_for (int i = 3; i < 10; i++) {
    (void)j;
  }
  // CHECK1: store i32 3, i32* [[i:%[a-zA-Z0-9_]*]]
  // CHECK1: [[Cond:%[a-zA-Z0-9_]*]] = icmp slt i32 {{.*}}, 10
  // CHECK1: br i1 [[Cond]]
  //
  // Captures: i, j
  // CHECK1: getelementptr inbounds
  // CHECK1: store i32* [[i]],
  // CHECK1: getelementptr inbounds
  // CHECK1: store i32*
  //
  // TODO: loop count
  //
  // CHECK1: call void @__cilkrts_cilk_for_32(void (i8*, i32, i32)* bitcast (void (%[[CapStruct:[a-z0-9\.]*]]*, i32, i32)* [[helper1:@__cilk_for_helper[0-9]*]]
  //
  // CHECK1: define internal void [[helper1]]
  // CHECK1: alloca %[[CapStruct]]
  // CHECK1: alloca i32
  // CHECK1: alloca i32
  // CHECK1: }
}

void test2(void) {
  // CHECK2: test2
  _Cilk_for (long long i = 2; i < 12; i++) { }
  // CHECK2: store i64 2, i64*
  // CHECK2: icmp slt i64 {{.*}}, 12
  //
  // TODO: loop count
  //
  // CHECK2: call void @__cilkrts_cilk_for_64

  int foo(void);
  int bar(void);
  _Cilk_for (int i = foo(); i < bar(); i++) { }
  // CHECK2: [[Init:%[a-zA-Z0-9_]*]] = call i32 @foo()
  // CHECK2: store i32 [[Init]], i32*
  // CHECK2: [[Limit:%[a-zA-Z0-9_]*]] = call i32 @bar()
  // CHECK2: icmp slt i32 {{.*}}, [[Limit]]
  //
  // CHECK2: call void @__cilkrts_cilk_for_32
}
