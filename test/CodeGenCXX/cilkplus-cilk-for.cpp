// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions  -disable-llvm-optzns -emit-llvm %s -o %t
// RUN: FileCheck -input-file=%t -check-prefix=CHECK1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK2 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK3 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK4 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK5 %s
//
struct Bool {
  ~Bool();
  operator bool();
};
struct T { };
Bool operator<(int, const T&);
int operator-(const T&, int);

void test_cond() {
  _Cilk_for (int i = 0; i < T(); i++) { }
  // CHECK1: [[Cond:%[a-zA-Z0-9_]*]] = invoke {{.*}} i1 @{{.*}}BoolcvbEv
  //
  // CHECK1: call void @{{.*}}BoolD1Ev
  // CHECK1: br i1 [[Cond]]
  //
  // CHECK1: call void @__cilkrts_cilk_for_32
  // CHECK1: br
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}BoolD1Ev
}

struct Int {
  Int(int);
  ~Int();
  operator int ();
  Int operator++(int);
};

void test_init() {
  _Cilk_for (int i = Int(19); i < 10; i++) { }
  // CHECK1: call void @{{.*}}IntC1Ei({{.*}}, i32 19)
  // CHECK1: [[Init:%[a-zA-Z0-9_]*]] = invoke i32 @{{.*}}IntcviEv
  //
  // CHECK1: call void @{{.*}}IntD1Ev
  // CHECK1: store i32
  // CHECK1: br i1
  //
  // CHECK1: call void @__cilkrts_cilk_for_32
  // CHECK1: br
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}IntD1Ev
}

struct Long {
  Long();
  Long(const Long&);
  ~Long();
};
bool operator<(int, Long) noexcept;
int operator-(Long, int) noexcept;

void test_cilk_for_loop_count() {
  // CHECK1: define void @{{.*}}test_cilk_for_loop_count

  extern void anchor(int) throw();

  anchor(1);
  // CHECK1: call void @{{.*}}anchori(i32 1)

  // Loop count should contain EWC from Int(10)
  _Cilk_for (int i = 0; i < Int(10); i++) { }
  // Condition:
  // CHECK1: call void @{{.*}}IntC1Ei
  // CHECK1: invoke i32 @{{.*}}IntcviEv
  // CHECK1: icmp slt
  // CHECK1: call void @{{.*}}IntD1Ev
  //
  // Loop count:
  // CHECK1: call void @{{.*}}IntC1Ei
  // CHECK1: invoke i32 @{{.*}}IntcviEv
  // CHECK1: sub nsw i32
  // CHECK1: add nsw i32
  // CHECK1: sdiv i32
  // CHECK1: call void @{{.*}}IntD1Ev
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}IntD1Ev
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}IntD1Ev

  anchor(2);
  // CHECK1: call void @{{.*}}anchori(i32 2)

  // Same as previous, but with the temporary in a subexpr
  _Cilk_for (int i = 0; i < 9 + Int(1); i++) { }
  // Condition:
  // CHECK1: call void @{{.*}}IntC1Ei
  // CHECK1: invoke i32 @{{.*}}IntcviEv
  // CHECK1: icmp slt
  // CHECK1: call void @{{.*}}IntD1Ev
  //
  // Loop count:
  // CHECK1: call void @{{.*}}IntC1Ei
  // CHECK1: invoke i32 @{{.*}}IntcviEv
  // CHECK1: sub nsw i32
  // CHECK1: add nsw i32
  // CHECK1: sdiv i32
  // CHECK1: call void @{{.*}}IntD1Ev
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}IntD1Ev
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}IntD1Ev

  anchor(3);
  // CHECK1: call void @{{.*}}anchori(i32 3)

  // Loop count should contain EWC from the increment
  _Cilk_for (int i = 0; i < 10000; i += 1 + Int(2012)) { }
  // Loop count:
  // CHECK1: sub nsw i32
  // CHECK1: call void @{{.*}}IntC1Ei
  // CHECK1: invoke i32 @{{.*}}IntcviEv
  // CHECK1: add nsw i32
  // CHECK1: sub nsw i32
  // CHECK1: add nsw i32
  // CHECK1: invoke void @{{.*}}IntC1Ei
  // CHECK1: invoke i32 @{{.*}}IntcviEv
  // CHECK1: add nsw i32
  // CHECK1: sdiv i32
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}IntD1Ev
  //
  // CHECK1: call void @{{.*}}IntD1Ev
  // CHECK1: call void @{{.*}}IntD1Ev

  anchor(4);
  // CHECK1: call void @{{.*}}anchori(i32 4)

  // Loop count should contain EWC from copying Long in operator<
  _Cilk_for (int i = 0; i < Long(); i++) { }
  // Condition:
  // CHECK1: call void @{{.*}}LongC1Ev
  // CHECK1-NEXT: call {{.*}}i1 @{{.*}}lti4Long
  // CHECK1-NEXT: call void @{{.*}}LongD1Ev
  //
  // Loop count:
  // CHECK1: call void @{{.*}}LongC1Ev
  // CHECK1: call i32 @{{.*}}mi4Longi
  // CHECK1: add nsw i32
  // CHECK1: sdiv i32
  // CHECK1: call void @{{.*}}LongD1Ev

  anchor(5);
  // CHECK1: call void @{{.*}}anchori(i32 5)

  Long l;
  // Loop count should contain EWC from building i < Long()
  _Cilk_for (int i = 0; i < l; i++) { }
  // Condition:
  // CHECK1: call void @{{.*}}LongC1Ev
  // CHECK1: invoke void @{{.*}}LongC1ERKS
  // CHECK1: call {{.*}}i1 @{{.*}}lti4Long
  // CHECK1-NEXT: call void @{{.*}}LongD1Ev
  //
  // Loop count:
  // CHECK1: invoke void @{{.*}}LongC1ERKS
  // CHECK1: call i32 @{{.*}}mi4Longi
  // CHECK1: add nsw i32
  // CHECK1: sdiv i32
  // CHECK1: call void @{{.*}}LongD1Ev
}

void test_increment() {
  _Cilk_for (int i = 0; i < 10000; i += Int(2012)) {
    extern void anchor() throw();
    anchor();
  }
  // CHECK2: call void @_ZN3IntC1Ei({{.*}}, i32 2012)
  // CHECK2-NEXT: invoke i32 @_ZN3IntcviEv
  // CHECK2-NOT: call @_ZN14test_increment6anchorEv
  //
  // Normal:
  // CHECK2: call void @_ZN3IntD1Ev
  //
  // Excepting:
  // CHECK2: call void @_ZN3IntD1Ev
  //
  // CHECK2: ret void
}

void test_cilk_for_body() {
  // Confirm this compiles; do not check the generated code.
  _Cilk_for (int i = 0; i < 10; i++) {
    try { throw 2013; } catch (...) { }
  }
}

void foo();

void test_cilk_for_try_cilk_spawn() {
  _Cilk_for (int i = 0; i < 10; i++) {
    try {
      _Cilk_spawn foo();
    } catch (...) { }
  }
  // CHECK3: define void @_Z28test_cilk_for_try_cilk_spawnv
  // CHECK3-NOT: call void @__cilk_parent_prologue
  // CHECK3: call void @__cilkrts_cilk_for_32({{.*}} [[helper1:@__cilk_for_helper[0-9]*]]

  // CHECK3: define internal void [[helper1]](
  // CHECK3: call void @__cilk_parent_prologue
  //
  // First implicit sync before entering the try-block
  // CHECK3: call void @__cilk_sync
  //
  // CHECK3: invoke void @__cilk_spawn_helper
  //
  // Second implicit sync before existing the try-block, normal path
  // CHECK3: call void @__cilk_sync
  //
  // Thrid implicit sync before existing the try-block, exception path
  // CHECK3: @__gxx_personality_v
  // CHECK3: call void @__cilk_sync
  //
  // No implicit sync until the end of the cilk for helper function, normal or exception path
  // CHECK3: call void @__cilk_parent_epilogue
  // CHECK3: call void @__cilk_parent_epilogue
  // CHECK3-NOT:  call void @__cilk_sync
  //
  // CHECK3: resume { i8*, i32 }
}

void test_cilk_for_catch_cilk_spawn() {
  _Cilk_for (int i = 0; i < 10; i++) {
    try {
      foo();
    } catch (...) {
      _Cilk_spawn foo();
    }
  }
  // CHECK4: define void @_Z30test_cilk_for_catch_cilk_spawnv
  // CHECK4-NOT: call void @__cilk_parent_prologue
  // CHECK4: call void @__cilkrts_cilk_for_32({{.*}} [[helper2:@__cilk_for_helper[0-9]*]]
  //
  // CHECK4: define internal void [[helper2]](
  // CHECK4: call void @__cilk_parent_prologue
  //
  // CHECK4: invoke void @__cxa_end_catch()
  //
  // An implicit sync is needed for the cilk for helper function, normal or exception path.
  //
  // CHECK4: call void @__cilk_sync
  // CHECK4-NEXT: call void @__cilk_parent_epilogue
  // CHECK4-NEXT: ret void
  //
  // CHECK4: call void @__cilk_sync
  // CHECK4-NEXT: call void @__cilk_parent_epilogue
  // CHECK4-NEXT: br
}

template <typename T>
void simple_template(const T &a, const T &b) {
  _Cilk_for (T i = a; i < b; i++) { }
}

void test_simple_template() {
  int begin = 0, end = 10;
  simple_template<int>(begin, end);

  int *pbegin = nullptr;
  int *pend = pbegin + 10;
  simple_template(pbegin, pend);

  // CHECK5: define {{.*}} void @_Z15simple_templateIiEvRKT_S2_
  // CHECK5: call void @__cilkrts_cilk_for_32
  // CHECK5: ret void
  //
  // CHECK5: define {{.*}} void @_Z15simple_templateIPiEvRKT_S3_
  // CHECK5: call void @__cilkrts_cilk_for_64
  // CHECK5: ret void
}
