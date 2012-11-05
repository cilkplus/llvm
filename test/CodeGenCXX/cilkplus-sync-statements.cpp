// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK1 %s
// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK-CILK2 %s

#define BIG_NUM 30
int global = 0;


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
  global = _Cilk_spawn Fib(BIG_NUM);
}

// Should still have the implicit sync, even though we have
// an explicit sync
// Note: the compiler may elide a sync if it can statically
// determine that the sync will have no observable efect
void test2() {
  // CHECK-CILK1: define void @_Z5test2v()

  // CHECK-CILK1: %__cilkrts_sf = alloca %__cilkrts_stack_frame
  // CHECK-CILK1: call void @__cilk_parent_prologue(%__cilkrts_stack_frame* %__cilkrts_sf)

  global = _Cilk_spawn Fib(BIG_NUM);

  // Make sure explicit call to sync is made
  // CHECK-CILK1: [[Flag:%[0-9]+]] = getelementptr inbounds %__cilkrts_stack_frame* %__cilkrts_sf, i32 0, i32 0
  // CHECK-CILK1-NEXT: [[Load:%[0-9]+]] = load i32* [[Flag]]
  // CHECK-CILK1-NEXT: [[Check:%[0-9]+]] = and i32 [[Load]], 2
  // CHECK-CILK1-NEXT: [[Br:%[0-9]+]] = icmp eq i32 [[Check]], 0
  // CHECK-CILK1-NEXT: br i1 [[Br]], label %{{.*}}, label %{{.*}}

  // CHECK-CILK1: call i8* @llvm.frameaddress
  // CHECK-CILK1: call i8* @llvm.stacksave
  // CHECK-CILK1: call i32 @llvm.eh.sjlj.setjmp
  // CHECK-CILK1: br i1 {{%[0-9]+}}, label %{{.*}}, label %{{.*}}

  // CHECK-CILK1: call void @__cilkrts_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  _Cilk_sync;

  // Make sure Cilk exits the function properly
  // CHECK-CILK1: call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)

  // CHECK-CILK1: call void @__cilk_parent_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
  // CHECK-CILK1-NEXT: ret void
}


// Should have an implicit sync at the end of the function
// Automatic variables should be destructed before the sync
void test3() {
  int local = _Cilk_spawn Fib(BIG_NUM);
}


// Should have an implicit sync at the end of the function
// Return values should be assigned before the sync
// Automatic variables should be destructed before the sync
int test4() {
  int local = _Cilk_spawn Fib(BIG_NUM);
  return local;
}

// Sync without corresponding spawn
// FIXME: If we implement an optimization that can elide unnecessary
// syncs then flip this check to make sure that the sync isn't emitted
void test5() {
  // CHECK-CILK2: define void @_Z5test5v()
  // CHECK-CILK1: %__cilkrts_sf = alloca %__cilkrts_stack_frame
  global = Fib(BIG_NUM);

  // Make sure explicit call to sync is made
  // CHECK-CILK1: call void @__cilkrts_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  _Cilk_sync;
}


// Should have implicit sync at end of try block
void test6() {
  // CHECK-CILK2: define void @_Z5test6v()
  try {
    global = ThrowingFib(BIG_NUM * BIG_NUM);
  } catch (int except) {
    return;
  }
}


// Should have sync before throw, right after the exception object
// has been created
void test7() throw (int) {
  global = _Cilk_spawn Fib(BIG_NUM);
  throw BIG_NUM;
}
