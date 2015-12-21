// RUN: %clang_cc1 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -O1 -o %t
// RUN: FileCheck -check-prefix=CHECK_INVOKE --input-file=%t %s
// REQUIRES: cilkplus
// XFAIL: win

namespace spawn_helper_invoke {

void foo();
void bar() throw();
void callee_throw() { foo(); }
void callee_nothrow() { bar(); }
void callee_nothrow_spec() throw ();
void callee_nothrow_attr() __attribute__((nothrow));

void test_throw() {
  _Cilk_spawn callee_throw();
  // CHECK_INVOKE: define {{.*}}@{{.*}}test_throw
  // CHECK_INVOKE: invoke {{.*}}__cilk_spawn_helper
  // CHECK_INVOKE: resume {
}

void test_nothrow() {
  _Cilk_spawn callee_nothrow();
  // CHECK_INVOKE: define {{.*}}@{{.*}}test_nothrow
  // CHECK_INVOKE: call {{.*}}__cilk_spawn_helper
  // CHECK_INVOKE: ret void
}

void test_nothrow_spec() {
  _Cilk_spawn callee_nothrow_spec();
  // CHECK_INVOKE: define {{.*}}@{{.*}}test_nothrow_spec
  // CHECK_INVOKE: call {{.*}}__cilk_spawn_helper
  // CHECK_INVOKE: ret void
}

void test_nothrow_attr() {
  _Cilk_spawn callee_nothrow_attr();
  // CHECK_INVOKE: define {{.*}}@{{.*}}test_nothrow_attr
  // CHECK_INVOKE: call {{.*}}__cilk_spawn_helper
  // CHECK_INVOKE: ret void
}

} // invoke_spawn_helper
