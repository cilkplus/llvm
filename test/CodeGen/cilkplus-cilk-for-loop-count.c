// RUN: %clang_cc1 -fcilkplus -emit-llvm -O1 %s -o %t-loop_count
// RUN: FileCheck -input-file=%t-loop_count -check-prefix=LOOP_COUNT %s
// RUN: %clang_cc1 -fcilkplus -emit-llvm -O2 %s -o %t-loop_count2
// RUN: FileCheck -input-file=%t-loop_count2 -check-prefix=LOOP_COUNT2 %s
// RUN: %clang_cc1 -fcilkplus -emit-llvm %s -o - | FileCheck %s

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

  char buf[100] = {0};
  // LOOP_COUNT: getelementptr inbounds [100 x i8]* %buf
  _Cilk_for(void *i = &buf[0]; i != &buf[99]; ++i);
  // LOOP_COUNT: call void @__cilkrts_cilk_for_[[BIT_WIDTH:[32|64]+]]({{.*}}, i[[BIT_WIDTH]] 99,

}

int init();
int limit();
int stride();

void test_cilk_for_ir() {
  // CHECK: test_cilk_for_ir

  _Cilk_for(int i = init(); i != limit(); i += stride());
  // CHECK: [[FIRST:%[A-Za-z0-9]+]] = alloca i32,
  // CHECK: [[R1:%[A-Za-z0-9]+]] = call i32 (...)* @init()
  // CHECK-NEXT: store i32 [[R1]], i32* [[FIRST]],
  // CHECK: [[R1:%[A-Za-z0-9]+]] = call i32 (...)* @stride()
  // CHECK-NEXT: [[SPAN_COMP:%[A-Za-z0-9]+]] = icmp slt i32 [[R1]], 0
  // CHECK-NEXT: br i1 [[SPAN_COMP]], label [[SPAN_NEG:%[A-Za-z0-9.]+]], label [[SPAN_POS:%[A-Za-z0-9.]+]]
  // CHECK: [[R1:%[A-Za-z0-9]+]] = call i32 (...)* @limit()
  // CHECK-NEXT: [[R2:%[A-Za-z0-9]+]] = load i32* [[FIRST]],
  // CHECK-NEXT: [[R3:%[A-Za-z0-9]+]] = sub nsw i32 [[R1]], [[R2]]
  // CHECK-NEXT: [[SPAN1:%[A-Za-z0-9]+]] = sub nsw i32 0, [[R3]]
  // CHECK: [[R1:%[A-Za-z0-9]+]] = call i32 (...)* @limit()
  // CHECK-NEXT: [[R2:%[A-Za-z0-9]+]] = load i32* [[FIRST]]
  // CHECK-NEXT: [[SPAN2:%[A-Za-z0-9]+]] = sub nsw i32 [[R1]], [[R2]]
  // CHECK: [[SPAN:%[A-Za-z0-9]+]] = phi i32 [ [[SPAN1]], [[SPAN_NEG]] ], [ [[SPAN2]], [[SPAN_POS]] ]
  // CHECK: [[R1:%[A-Za-z0-9]+]] = call i32 (...)* @stride()
  // CHECK-NEXT: [[STRIDE_COMP:%[A-Za-z0-9]+]] = icmp slt i32 [[R1]], 0
  // CHECK-NEXT: br i1 [[STRIDE_COMP]], label [[STRIDE_NEG:%[A-Za-z0-9.]+]], label [[STRIDE_POS:%[A-Za-z0-9.]+]]
  // CHECK: [[R1:%[A-Za-z0-9]+]] = call i32 (...)* @stride()
  // CHECK-NEXT: [[STRIDE1:%[A-Za-z0-9]+]] = sub nsw i32 0, [[R1]]
  // CHECK: [[STRIDE2:%[A-Za-z0-9]+]] = call i32 (...)* @stride()
  // CHECK: [[STRIDE:%[A-Za-z0-9]+]] = phi i32 [ [[STRIDE1]], [[STRIDE_NEG]] ], [ [[STRIDE2]], [[STRIDE_POS]] ]
  // CHECK: [[LOOP_LIMIT:%[A-Za-z0-9]+]] = sdiv i32 [[SPAN]], [[STRIDE]]
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}}, i32 [[LOOP_LIMIT]],
}

