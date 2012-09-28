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
  global = _Cilk_spawn Fib(BIG_NUM);
  _Cilk_sync;
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
  global = Fib(BIG_NUM);
  _Cilk_sync;
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


int main() {
  // Need to have a CHECK statement somewhere
  // When _Cilk_spawn functionality is added need to add CHECK
  // statements to make sure all the tests work as expected
  // CHECK: alloca
  return 0;
}
