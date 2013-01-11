// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK_PARENT %s
// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK_HELPER_F1 %s
// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK_HELPER_F2 %s
// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK_IMPLICIT_SYNC %s
//
namespace stack_frame_cleanup {
  extern void touch();

  struct C {
    C() { touch(); }
    ~C(){ touch(); }
  };

  template <typename T> void f1(T);
  template <typename T> T f2(T);

  template <typename T, int x>
  void test_f1() {
    _Cilk_spawn f1(T());
  }

  template <typename T, int x>
  void test_f2(T &ret) {
    ret = _Cilk_spawn f2(T());
  }

  void parent_stack_frame_test() {
    test_f1<int, 23>();
    // CHECK_PARENT: define {{.*}} @_ZN19stack_frame_cleanup7test_f1IiLi23EEEvv
    // CHECK_PARENT: alloca %__cilkrts_stack_frame
    // CHECK_PARENT-NEXT: call void @__cilk_parent_prologue
    // CHECK_PARENT: invoke void @_ZN19stack_frame_cleanup{{[0-9]+}}__cilk_spawn_helperV{{[0-9]+}}EPZNS{{.*}}i23{{.*}}capture
    //
    // * Normal exit *
    //
    // CHECK_PARENT: call void @__cilk_parent_epilogue
    // CHECK_PARENT-NEXT: ret void
    //
    // * Exit due to exception *
    //
    // CHECK_PARENT: call void @__cilk_parent_epilogue
    // CHECK_PARENT-NEXT: br label
  }

  void helper_stack_frame_test() {
    test_f1<C, 29>();
    // CHECK_HELPER_F1: define internal void @_ZN19stack_frame_cleanup{{[0-9]+}}__cilk_spawn_helperV{{[0-9]+}}EPZNS{{.*}}i29{{.*}}capture
    // CHECK_HELPER_F1: alloca %__cilkrts_stack_frame
    // CHECK_HELPER_F1: call void @__cilk_reset_worker
    //
    // Call C's constructor
    // CHECK_HELPER_F1: invoke void @_ZN19stack_frame_cleanup1CC1Ev
    //
    // CHECK_HELPER_F1: call void @__cilk_helper_prologue
    // CHECK_HELPER_F1-NEXT: invoke void @_ZN19stack_frame_cleanup2f1INS_1CEEEvT_
    //
    // * Normal exit *
    //
    // Call C's destructor
    // CHECK_HELPER_F1: call void @_ZN19stack_frame_cleanup1CD1Ev
    // CHECK_HELPER_F1-NEXT: call void @__cilk_helper_epilogue
    // CHECK_HELPER_F1-NEXT: ret void
    //
    // * Exit due to exception *
    //
    // CHECK_HELPER_F1: call void @_ZN19stack_frame_cleanup1CD1Ev
    // CHECK_HELPER_F1-NEXT: br label
    // CHECK_HELPER_F1: call void @__cilk_helper_epilogue
    // CHECK_HELPER_F1-NEXT: br label
  }

  void helper_check_assignment() {
    int x = 0;
    test_f2<int, 37>(x);
    // CHECK_HELPER_F2: define internal void @_ZN19stack_frame_cleanup{{[0-9]+}}__cilk_spawn_helperV{{[0-9]+}}EPZNS{{.*}}i37{{.*}}capture
    //
    // CHECK_HELPER_F2: [[REG:%[a-zA-Z0-9]+]] = getelementptr inbounds %struct.capture
    // CHECK_HELPER_F2-NEXT: load i32** [[REG]]
    // CHECK_HELPER_F2-NEXT: call void @__cilk_helper_prologue
    // CHECK_HELPER_F2-NEXT: [[RET_REG:%[a-zA-Z0-9]+]] = invoke i32 @_ZN19stack_frame_cleanup2f2IiEET_S1_
    //
    // * Normal exit *
    //
    // CHECK_HELPER_F2: store i32 [[RET_REG]]
    // CHECK_HELPER_F2-NEXT: call void @__cilk_helper_epilogue
    // CHECK_HELPER_F2-NEXT: ret void
  }
}

namespace implicit_sync_elision_basic {

void foo();
void bar();

void test1_anchor() throw ();

// No implicit sync for the function
void test1() {
  try {
    _Cilk_spawn foo(); 
  } catch (...) {
    bar();
  }

  test1_anchor();
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test1Ev
  //
  // CHECK_IMPLICIT_SYNC: call void @_ZN27implicit_sync_elision_basic12test1_anchorEv
  // CHECK_IMPLICIT_SYNC-NEXT: call void @__cilk_parent_epilogue
  // CHECK_IMPLICIT_SYNC-NEXT: ret void
}

void test2_anchor() throw ();

// Should have an implicit sync for the function
void test2() {
  try {
    foo(); 
  } catch (...) {
    _Cilk_spawn bar();
  }

  test2_anchor();
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test2Ev
  // CHECK_IMPLICIT_SYNC: call void @_ZN27implicit_sync_elision_basic12test2_anchorEv
  // CHECK_IMPLICIT_SYNC: call void @__cilkrts_sync
}

void test3_anchor() throw ();

// Should have an implicit sync for the function
void test3() {
  try {
    _Cilk_spawn foo(); 
  } catch (...) {
    _Cilk_spawn bar();
  }
  
  test3_anchor();
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test3Ev
  // CHECK_IMPLICIT_SYNC: call void @_ZN27implicit_sync_elision_basic12test3_anchorEv
  // CHECK_IMPLICIT_SYNC: call void @__cilkrts_sync
}

void test4_anchor() throw ();

// No implicit sync for the function
void test4() {
  try {
    try {
      _Cilk_spawn foo();
    } catch (...) {
      _Cilk_spawn bar();
    }
  } catch (...) {
    bar();
  }

  test4_anchor();
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test4Ev
  // CHECK_IMPLICIT_SYNC: call void @_ZN27implicit_sync_elision_basic12test4_anchorEv
  // CHECK_IMPLICIT_SYNC-NEXT: call void @__cilk_parent_epilogue
  // CHECK_IMPLICIT_SYNC-NEXT: ret void
}

// Should have an implicit sync before throw
void test5() {
  _Cilk_spawn foo();
  throw 13;
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test5Ev
  // CHECK_IMPLICIT_SYNC: call i8* @__cxa_allocate_exception
  // CHECK_IMPLICIT_SYNC: store i32 13
  // CHECK_IMPLICIT_SYNC: call void @__cilkrts_sync
  // CHECK_IMPLICIT_SYNC: invoke void @__cxa_throw
}

// Should have an implicit sync before throw
void test6() {
  try {
    _Cilk_spawn foo();
    throw 17;
  } catch (...) {
    bar();
  }
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test6Ev
  // CHECK_IMPLICIT_SYNC: call i8* @__cxa_allocate_exception
  // CHECK_IMPLICIT_SYNC: store i32 17
  // CHECK_IMPLICIT_SYNC: call void @__cilkrts_sync
  // CHECK_IMPLICIT_SYNC: invoke void @__cxa_throw
}

// No implicit sync before throw
void test7() {
  try {
    _Cilk_spawn foo();
  } catch (...) {
    bar();
  }
  throw 19;
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test7Ev
  // CHECK_IMPLICIT_SYNC: call i8* @__cxa_allocate_exception
  // CHECK_IMPLICIT_SYNC: store i32 19
  // CHECK_IMPLICIT_SYNC-NOT: call void @__cilkrts_sync
  // CHECK_IMPLICIT_SYNC: invoke void @__cxa_throw
}

// No implicit sync inside try-block
void test8() {
  try {
    foo();
  } catch (...) {
    _Cilk_spawn bar();
  }
  // CHECK_IMPLICIT_SYNC: define void @_ZN27implicit_sync_elision_basic5test8Ev
  // CHECK_IMPLICIT_SYNC: invoke void @_ZN27implicit_sync_elision_basic3fooEv()
  // CHECK_IMPLICIT_SYNC-NOT: call void @__cilkrts_sync
  // CHECK_IMPLICIT_SYNC: call i8* @__cxa_begin_catch
}

} // namespace
