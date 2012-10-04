// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify -fcxx-exceptions -fexceptions -std=c++11 -emit-llvm %s -o - | FileCheck %s

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
  // Make sure Cilk enters the function properly
  // CHECK: alloca %__cilkrts_stack_frame
  // CHECK: alloca %__cilkrts_worker*
  // CHECK: call void @__cilk_prologue(%__cilkrts_stack_frame* %__cilkrts_sf, %__cilkrts_worker** %__cilkrts_w)

  global = _Cilk_spawn Fib(BIG_NUM);

  // Make sure explicit call to sync is made
  // CHECK: call void @__cilk_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  _Cilk_sync;

  // Make sure Cilk exits the function properly
  // CHECK: call void @__cilk_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
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
// Note: the compiler may elide a sync if it can statically
// determine that the sync will have no observable efect
void test5() {
  // Make sure Cilk enters the function properly
  // CHECK: alloca %__cilkrts_stack_frame
  // CHECK: alloca %__cilkrts_worker*
  // CHECK: call void @__cilk_prologue(%__cilkrts_stack_frame* %__cilkrts_sf, %__cilkrts_worker** %__cilkrts_w)

  global = Fib(BIG_NUM);

  // Make sure explicit call to sync is made
  // CHECK: call void @__cilk_sync(%__cilkrts_stack_frame* %__cilkrts_sf)
  _Cilk_sync;

  // Make sure Cilk exits the function properly
  // CHECK: call void @__cilk_epilogue(%__cilkrts_stack_frame* %__cilkrts_sf)
}


// Should have implicit sync at end of try block
void test6() {
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
