// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o %t
// RUN: FileCheck -input-file=%t -check-prefix=CHECK1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK2 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK3 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK-SPAWN %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK-SPAWN2 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK-LCV1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK-LCV2 %s

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

void test_lcv1(void) {
  // CHECK-LCV1: test_lcv1
  long long ll;
  _Cilk_for (ll = 2; ll < 12; ll += 4) { }
  // CHECK-LCV1: call void @__cilkrts_cilk_for_64
  //
  // CHECK-LCV1-NEXT: [[R1:%[a-zA-Z0-9]+]] = load i64*
  // CHECK-LCV1-NEXT: [[R2:%[a-zA-Z0-9]+]] = mul i64 4, %
  // CHECK-LCV1-NEXT: [[R3:%[a-zA-Z0-9]+]] = add i64 [[R1]], [[R2]]
  // CHECK-LCV1-NEXT: store i64 [[R3]], i64*

  int i;
  _Cilk_for (i = 0; i < 10; i++) { }
  // CHECK-LCV1: call void @__cilkrts_cilk_for_32
  //
  // CHECK-LCV1-NEXT: [[R3:%[a-zA-Z0-9]+]] = load i32*
  // CHECK-LCV1-NEXT: [[R4:%[a-zA-Z0-9]+]] = add i32 [[R3]], %
  // CHECK-LCV1-NEXT: store i32 [[R4]], i32*

  char c;
  _Cilk_for (c = 20; c > 10; c--) { }
  // CHECK-LCV1: call void @__cilkrts_cilk_for_32
  //
  // CHECK-LCV1-NEXT: [[R5:%[a-zA-Z0-9]+]] = load i8*
  // CHECK-LCV1-NEXT: [[Trunc:%[a-zA-Z0-9]+]] = trunc i32 %{{.+}} to i8
  // CHECK-LCV1-NEXT: [[R6:%[a-zA-Z0-9]+]] = sub i8 [[R5]], [[Trunc]]
  // CHECK-LCV1-NEXT: store i8 [[R6]], i8*

  short s;
  _Cilk_for (s = 20; s > 10; s -= 2) { }
  // CHECK-LCV1: call void @__cilkrts_cilk_for_32
  //
  // CHECK-LCV1: [[R5:%[a-zA-Z0-9]+]] = load i16*
  // CHECK-LCV1-NEXT: [[R6:%[a-zA-Z0-9]+]] = mul i32 2, %
  // CHECK-LCV1-NEXT: [[Trunc:%[a-zA-Z0-9]+]] = trunc i32 [[R6]] to i16
  // CHECK-LCV1-NEXT: [[R7:%[a-zA-Z0-9]+]] = sub i16 [[R5]], [[Trunc]]
  // CHECK-LCV1-NEXT: store i16 [[R7]], i16*

  unsigned long long ull;
  _Cilk_for (ull = 0; ull < 50; ull += 10);
  // CHECK-LCV1: call void @__cilkrts_cilk_for_64
  //
  // CHECK-LCV1-NEXT: [[R7:%[a-zA-Z0-9]+]] = load i64*
  // CHECK-LCV1-NEXT: [[R8:%[a-zA-Z0-9]+]] = mul i64 10, %
  // CHECK-LCV1-NEXT: [[R9:%[a-zA-Z0-9]+]] = add i64 [[R7]], [[R8]]
  // CHECK-LCV1-NEXT: store i64 [[R9]], i64*

  enum { E = 2 };
  _Cilk_for (ll = 0; ll < 101; ll += E);
  // CHECK-LCV1: call void @__cilkrts_cilk_for_64
  //
  // CHECK-LCV1-NEXT: [[R10:%[a-zA-Z0-9]+]] = load i64
  // CHECK-LCV1-NEXT: [[R11:%[a-zA-Z0-9]+]] = mul i64 2, %
  // CHECK-LCV1-NEXT: [[R12:%[a-zA-Z0-9]+]] = add i64 [[R10]], [[R11]]
  // CHECK-LCV1-NEXT: store i64 [[R12]], i64*

}

void test_lcv2(void) {
  // CHECK-LCV2: test_lcv2
  char buf[100] = {0};
  char *b;
  _Cilk_for (b = &buf[0]; b != &buf[99]; ++b);
  // CHECK-LCV2: call void @__cilkrts_cilk_for_[[BIT_WIDTH:32|64]]
  //
  // CHECK-LCV2-NEXT: [[R1:%[a-zA-Z0-9]+]] = load i8**
  // CHECK-LCV2-NEXT: [[R2:%[a-zA-Z0-9]+]] = getelementptr i8* [[R1]], i[[BIT_WIDTH]] %
  // CHECK-LCV2-NEXT: store i8* [[R2]], i8**

  _Cilk_for (b = &buf[0]; b != &buf[99]; b += 4);
  // CHECK-LCV2: call void @__cilkrts_cilk_for_[[BIT_WIDTH:32|64]]
  //
  // CHECK-LCV2-NEXT: [[R3:%[a-zA-Z0-9]+]] = load i8**
  // CHECK-LCV2-NEXT: [[R4:%[a-zA-Z0-9]+]] = mul i64 4, %
  // CHECK-LCV2-NEXT: [[R5:%[a-zA-Z0-9]+]] = getelementptr i8* [[R3]], i[[BIT_WIDTH]] %
  // CHECK-LCV2-NEXT: store i8* [[R5]], i8**

  int i2;
  char x = 2;
  _Cilk_for (i2 = 0; i2 < 10; i2 += x);
  // CHECK-LCV2: call void @__cilkrts_cilk_for_32
  //
  // CHECK-LCV2-NEXT: [[R6:%[a-zA-Z0-9]+]] = load i32*
  // CHECK-LCV2-NEXT: [[R7:%[a-zA-Z0-9]+]] = load i8*
  // CHECK-LCV2-NEXT: [[R8:%[a-zA-Z0-9]+]] = sext i8 [[R7]] to i32
  // CHECK-LCV2-NEXT: [[R9:%[a-zA-Z0-9]+]] = mul i32 [[R8]], %
  // CHECK-LCV2-NEXT: [[R10:%[a-zA-Z0-9]+]] = add i32 [[R6]], [[R9]]
  // CHECK-LCV2-NEXT: store i32 [[R10]], i32* %i2

  char *p;
  short s = 2;
  _Cilk_for (p = &buf[0]; p != &buf[99]; p += s);
  // CHECK-LCV2: call void @__cilkrts_cilk_for_[[BIT_WIDTH:32|64]]
  //
  // CHECK-LCV2-NEXT: [[R11:%[a-zA-Z0-9]+]] = load i8**
  // CHECK-LCV2: [[R12:%[a-zA-Z0-9]+]] = mul i64 %{{.+}}, %
  // CHECK-LCV2-NEXT: [[R13:%[a-zA-Z0-9]+]] = getelementptr i8* [[R11]], i[[BIT_WIDTH]] %
  // CHECK-LCV2-NEXT: store i8* [[R13]], i8**

  _Cilk_for (int i = 0; i < 10; i++);
  // CHECK-LCV2: call void @__cilkrts_cilk_for_32
  //
  // CHECK-NEXT: br label,
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
