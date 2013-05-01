// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck %s

#define BIG_NUM 30
int global = 0;

struct Foo {
  int x;
  float y;

  Foo() : x(10), y(11.0f) {};
  ~Foo() {global++;}
};


unsigned int Fib(unsigned int n) {
  if(n < 2) return n;

  return Fib(n - 1) + Fib(n - 2);
}

unsigned int ThrowingFib(unsigned int n) throw (int) {
  if(n > BIG_NUM) throw n;

  if(n < 2) return n;

  return Fib(n - 1) + Fib(n - 2);
}


// Should have an implicit sync at the end of the function
void test1() {
  // CHECK: define void @_Z5test1v()

  // CHECK: %__cilkrts_sf = alloca %__cilkrts_stack_frame
  // CHECK: call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %__cilkrts_sf)

  global = _Cilk_spawn Fib(BIG_NUM);

  // Make sure implicit sync is inserted
  // CHECK: [[Flag:%[0-9]+]] = getelementptr inbounds %__cilkrts_stack_frame* %__cilkrts_sf, i32 0, i32 0
  // CHECK-NEXT: [[Load:%[0-9]+]] = load i32* [[Flag]]
  // CHECK-NEXT: [[Check:%[0-9]+]] = and i32 [[Load]], 2
  // CHECK-NEXT: [[Br:%[0-9]+]] = icmp eq i32 [[Check]], 0
  // CHECK-NEXT: br i1 [[Br]], label %{{.*}}, label %{{.*}}

  // CHECK: call i8* @llvm.frameaddress
  // CHECK: call i8* @llvm.stacksave
  // CHECK: call i32 @llvm.eh.sjlj.setjmp
  // CHECK: br i1 {{%[0-9]+}}, label %{{.*}}, label %{{.*}}

  // CHECK: call void @__cilkrts_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  //
  // CHECk: call void @__cilkrts_rethrow
  //
  // Make sure Cilk exits the function properly
  // CHECK: call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
  // CHECK-NEXT: ret
}

// Should still have the implicit sync, even though we have
// an explicit sync
// Note: the compiler may elide a sync if it can statically
// determine that the sync will have no observable efect
void test2() {
  // CHECK: define void @_Z5test2v()

  // CHECK: %__cilkrts_sf = alloca %__cilkrts_stack_frame
  // CHECK: call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %__cilkrts_sf)

  global = _Cilk_spawn Fib(BIG_NUM);

  // Make sure explicit call to sync is made
  // CHECK: [[Flag:%[0-9]+]] = getelementptr inbounds %__cilkrts_stack_frame* %__cilkrts_sf, i32 0, i32 0
  // CHECK-NEXT: [[Load:%[0-9]+]] = load i32* [[Flag]]
  // CHECK-NEXT: [[Check:%[0-9]+]] = and i32 [[Load]], 2
  // CHECK-NEXT: [[Br:%[0-9]+]] = icmp eq i32 [[Check]], 0
  // CHECK-NEXT: br i1 [[Br]], label %{{.*}}, label %{{.*}}

  // CHECK: call i8* @llvm.frameaddress
  // CHECK: call i8* @llvm.stacksave
  // CHECK: call i32 @llvm.eh.sjlj.setjmp
  // CHECK: br i1 {{%[0-9]+}}, label %{{.*}}, label %{{.*}}

  // CHECK: call void @__cilkrts_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  _Cilk_sync;

  // Make sure implicit sync is still added
  // CHECK: call void @__cilkrts_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  //
  // CHECk: call void @__cilkrts_rethrow
  //
  // Make sure Cilk exits the function properly
  // CHECK: call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
  // CHECK-NEXT: ret void
}


// Should have an implicit sync at the end of the function
// Automatic variables should be destructed before the sync
void test3() {
  // CHECK: define void @_Z5test3v()

  // CHECK: invoke void @_ZN3FooC1Ev(
  // CHECK: call void @_ZN3FooD1Ev
  // CHECK: call void @__cilkrts_sync

  Foo x;
  int local = _Cilk_spawn Fib(BIG_NUM);
}


// Should have an implicit sync at the end of the function
// Return values should be assigned before the sync
// Automatic variables should be destructed before the sync
int test4() {
  // CHECK: define i32 @_Z5test4v()

  // CHECK: [[Retval:%[0-9]+]] = load i32* %local

  // CHECK: call void @__cilkrts_sync

  // CHECK: ret i32 [[Retval]]

  int local = _Cilk_spawn Fib(BIG_NUM);
  return local;
}

// Should elide unnecessary syncs
void test5() {
  global = Fib(BIG_NUM);
  // CHECK: define void @_Z5test5v()
  // CHECK-NOT: alloca %__cilkrts_stack_frame
  // CHECK: call i32 @_Z3Fibj(
  // CHECK-NEXT: store i32
  // CHECK-NOT:  call void @__cilkrts_sync
  // CHECK-NEXT: ret void
  _Cilk_sync;
}

void test6_anchor() throw ();

// Should have implicit sync at end of try block
void test6() {
  // CHECK: define void @_Z5test6v()
  //
  try {
    global = _Cilk_spawn ThrowingFib(BIG_NUM * BIG_NUM);
    // normal and exception handling implicit sync
  } catch (int except) {
    // CHECK: call void @__cilkrts_sync
    return;
  } catch (float except) {
    // CHECK: call void @__cilkrts_sync
  }
  // finally {
  //   CHECK: call void @__cilkrts_sync
  // }

  test6_anchor();
  // Elide the function implicit sync
  //
  // CHECK: call void @_Z12test6_anchorv()
  // CHECK-NOT:  call void @__cilkrts_sync
  // CHECK: ret
}


// Should have sync before throw, right before the exception object
// has been created
void test7() throw (int) {
  try {
    global = _Cilk_spawn Fib(BIG_NUM);
    throw BIG_NUM;
  } catch (...) { }

  // CHECK: define void @_Z5test7v()
  //
  // * Implicit sync while entering the try block
  // CHECK: call void @__cilkrts_sync
  //
  // implicit sync before throwing
  // CHECK: call void @__cilkrts_sync
  // CHECK: call i8* @__cxa_allocate_exception
  // CHECK: store i32 30
  // CHECK: invoke void @__cxa_throw
}
