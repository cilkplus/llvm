// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o %t
// RUN: FileCheck -input-file=%t -check-prefix=CHECK1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK2 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK3 %s
// RUN: %clang_cc1 -fcilkplus -emit-llvm -O1 %s -o %t-loop_count
// RUN: FileCheck -input-file=%t-loop_count -check-prefix=LOOP_COUNT %s
// RUN: %clang_cc1 -fcilkplus -emit-llvm -O2 %s -o %t-loop_count2
// RUN: FileCheck -input-file=%t-loop_count2 -check-prefix=LOOP_COUNT2 %s

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
  // CHECK1: call void @__cilkrts_cilk_for_32(void (i8*, i32, i32)* bitcast (void (%{{[a-z0-9\.]+}}*, i32, i32)* [[helper1:@__captured_stmt[0-9]*]]
  //
  // CHECK1: define internal void [[helper1]]
  // CHECK1: alloca %struct.cilk.for.capture
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

int foo() { return 10; }
struct Bar {
  int Val;
};
int baz(struct Bar b) { return b.Val; }

void test_loop_count() {
  // LOOP_COUNT: test_loop_count
  int limit;

  _Cilk_for(int i = 0; i < 10; ++i);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 10,

  _Cilk_for(long long i = 0; i < 10; ++i);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_64({{.*}}, i64 10,

  _Cilk_for(short i = 10; i > 0; --i);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 10,

  limit = 20;
  _Cilk_for(char i = 0; i < limit; i += 3);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 7,

  _Cilk_for(int i = 0; i != 27; i += 9);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 3,

  _Cilk_for(int i = 55; 27 != i; i -= 4);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 7,

  limit = -27;
  _Cilk_for(int i = 55; limit != i; i -= 4);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 20,

  _Cilk_for(int i = 17; i <= 17; i += 1);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 1,

  int increment = 3;
  _Cilk_for(unsigned int i = 0u; i <= 17u; i += increment);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 6,

  increment = -3;
  _Cilk_for(int i = 13; i >= 7; i += increment);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 3,

  enum { FOO = 10 };
  increment = -1;
  _Cilk_for(long long i = 0; i != FOO; i -= increment);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_64({{.*}}, i64 10,

  _Cilk_for(int i = 3; i < foo(); i++);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_32({{.*}}, i32 7,

  struct Bar b = {9};
  _Cilk_for(int i = 3; i < baz(b); i++);
  // LOOP_COUNT2: call void @__cilkrts_cilk_for_32({{.*}}, i32 6,
}
