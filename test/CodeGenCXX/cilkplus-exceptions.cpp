// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK_PARENT %s
// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK_HELPER_F1 %s
// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -fcilkplus -emit-llvm %s -o - | FileCheck -check-prefix=CHECK_HELPER_F2 %s
//
namespace stack_frame_cleanup {
  extern void touch();

  struct C {
    C() { touch(); }
    ~C(){ touch(); }
  };

  template <typename T> void f1(T);
  template <typename T> T f2(T);

  template <typename T>
  void test_f1() {
    _Cilk_spawn f1(T());
  }

  template <typename T>
  void test_f2(T &ret) {
    ret = _Cilk_spawn f2(T());
  }

  void parent_stack_frame_test() {
    test_f1<int>();
    // CHECK_PARENT: define {{.*}} @_ZN19stack_frame_cleanup7test_f1IiEEvv
    // CHECK_PARENT: alloca %__cilkrts_stack_frame
    // CHECK_PARENT-NEXT: call void @__cilk_parent_prologue
    // CHECK_PARENT: invoke void @_ZN19stack_frame_cleanup21__cilk_spawn_helperV{{[0-9]+}}EPZNS_7test_f1IiEEvvE7capture(
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
    test_f1<C>();
    // CHECK_HELPER_F1: define {{.*}} void @_ZN19stack_frame_cleanup21__cilk_spawn_helperV{{[0-9]+}}EPZNS_7test_f1INS_1CEEEvvE7capture(
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
    test_f2<int>(x);

    // CHECK_HELPER_F2: define {{.*}} @_ZN19stack_frame_cleanup21__cilk_spawn_helperV{{[0-9]+}}EPZNS_7test_f2IiEEvRT_E7capture
    //
    // CHECK_HELPER_F2: [[REG:%[0-9]+]] = getelementptr inbounds %struct.capture*
    // CHECK_HELPER_F2-NEXT: load i32** [[REG]]
    // CHECK_HELPER_F2-NEXT: call void @__cilk_helper_prologue
    // CHECK_HELPER_F2-NEXT: [[RET_REG:%[0-9]+]] = invoke i32 @_ZN19stack_frame_cleanup2f2IiEET_S1_
    //
    // * Normal exit *
    //
    // CHECK_HELPER_F2: store i32 [[RET_REG]]
    // CHECK_HELPER_F2-NEXT: call void @__cilk_helper_epilogue
    // CHECK_HELPER_F2-NEXT: ret void
  }
}
