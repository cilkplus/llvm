// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o %t
// RUN: FileCheck -input-file=%t -check-prefix=CHECK1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK2 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK3 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK-SPAWN %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK-SPAWN2 %s

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

void test_jump(void) {
  // CHECK3: test_jump
  extern int skip(void);
  extern void skipped_func(void);

  _Cilk_for (int i = 0; i < 10; i++) {
label:
    if (skip())
      continue;
    else
      goto label;
    // CHECK3: call i32 @skip()
    // CHECK3-NOT: call void @skipped_func
    skipped_func();
  }
}

void test_pointer(float *p, float *q) {
  _Cilk_for (float *i = p; i < q; i++) { }
}

void spawn_anchor(void);
void spawn_foo(void);

void test_cilk_for_cilk_spawn() {
  _Cilk_for (int i = 0; i < 10; i++) {
    spawn_anchor();
    _Cilk_spawn spawn_foo();
    spawn_anchor();
  }
  // The function is not a spawning function any more, and it should not
  // initialize a stack frame.
  //
  // CHECK-SPAWN: define void @test_cilk_for_cilk_spawn
  // CHECK-SPAWN-NOT: call void @__cilk_parent_prologue
  // CHECK-SPAWN: call void @__cilkrts_cilk_for_32({{.*}} [[helper2:@__cilk_for_helper[0-9]*]]
  //
  // The cilk for helper function is a spawning function, and it should
  // initialize a stack frame.
  //
  // CHECK-SPAWN: define internal void [[helper2]](
  // CHECK-SPAWN: call void @__cilk_parent_prologue
  // CHECK-SPAWN: call void @spawn_anchor()
  // CHECK-SPAWN: call void @__cilk_spawn_helper
  // CHECK-SPAWN: call void @spawn_anchor()
  // CHECK-SPAWN: call void @__cilk_parent_epilogue
  // CHECK-SPAWN-NEXT: ret void
}

int spawn_switch(void);
void spawn_bar(void);

void test_cilk_for_cilk_spawn_no_compound() {
  _Cilk_for (int i = 0; i < 10; i++)
    if (spawn_switch()) {
      spawn_anchor();
      _Cilk_spawn spawn_foo();
      spawn_anchor();
    } else {
      spawn_anchor();
      _Cilk_spawn spawn_bar();
      spawn_anchor();
    }
  // CHECK-SPAWN2: define void @test_cilk_for_cilk_spawn_no_compound
  // CHECK-SPAWN2-NOT: call void @__cilk_parent_prologue
  // CHECK-SPAWN2: call void @__cilkrts_cilk_for_32({{.*}} [[helper3:@__cilk_for_helper[0-9]*]]

  // CHECK-SPAWN2: define internal void [[helper3]](
  // CHECK-SPAWN2: call void @__cilk_parent_prologue
  // CHECK-SPAWN2: call void @spawn_anchor()
  // CHECK-SPAWN2: call void @__cilk_spawn_helper
  // CHECK-SPAWN2: call void @spawn_anchor()
  // CHECK-SPAWN2: call void @spawn_anchor()
  // CHECK-SPAWN2: call void @__cilk_spawn_helper
  // CHECK-SPAWN2: call void @spawn_anchor()
  // CHECK-SPAWN2: call void @__cilk_parent_epilogue
  // CHECK-SPAWN2-NEXT: ret void
}

void test_ptr(int *p, int *q, short s) {
  int *r;
  _Cilk_for (r = p; r != q; r += s); // OK, should not crash.
}
