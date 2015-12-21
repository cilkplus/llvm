// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions  -disable-llvm-optzns -emit-llvm %s -o %t
// RUN: FileCheck -input-file=%t -check-prefix=CHECK1 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK2 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK5 %s
// RUN: FileCheck -input-file=%t -check-prefix=CHECK6 %s
// REQUIRES: cilkplus
// XFAIL: win

struct Bool {
  ~Bool();
  operator bool();
};
struct T { };
Bool operator<(int, const T&);
int operator-(const T&, int);

void test_cond() {
  _Cilk_for (int i = 0; i < T(); i++) { }
  // CHECK1: [[Cond:%[a-zA-Z0-9_]*]] = invoke {{.*}} i1 @{{.*}}Bool
  //
  // CHECK1: call void @{{.*}}Bool
  // CHECK1: br i1 [[Cond]]
  //
  // CHECK1: call void @__cilkrts_cilk_for_32
  // CHECK1: br
  //
  // CHECK1: landingpad
  // CHECK1: call void @{{.*}}Bool
}

struct Int {
  Int(int);
  ~Int();
  operator int ();
  Int operator++(int);
};

void test_init() {
  _Cilk_for (int i = Int(19); i < 10; i++) { }
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: [[Init:%[a-zA-Z0-9_]*]] = invoke i32 @{{.*}}Int
  //
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: store i32
  // CHECK1: br i1
  //
  // CHECK1: call void @__cilkrts_cilk_for_32
  // CHECK1: br
  //
  // CHECK1: landingpad
  // CHECK1: call {{.*}}@{{.*}}Int
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
  // CHECK1: call {{.*}}@{{.*}}anchor

  // Loop count should contain EWC from Int(10)
  _Cilk_for (int i = 0; i < Int(10); i++) { }
  // Condition:
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: invoke i32 @{{.*}}Int
  // CHECK1: icmp slt
  // CHECK1: call {{.*}}@{{.*}}Int
  //
  // Loop count:
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: invoke i32 @{{.*}}Int
  // CHECK1: sub nsw i32
  // CHECK1: sub nsw i32
  // CHECK1: sdiv i32
  // CHECK1: add nsw i32
  // CHECK1: call {{.*}}@{{.*}}Int
  //
  // CHECK1: landingpad
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: landingpad
  // CHECK1: call {{.*}}@{{.*}}Int

  anchor(2);
  // CHECK1: call {{.*}}@{{.*}}anchor

  // Same as previous, but with the temporary in a subexpr
  _Cilk_for (int i = 0; i < 9 + Int(1); i++) { }
  // Condition:
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: invoke i32 @{{.*}}Int
  // CHECK1: icmp slt
  // CHECK1: call {{.*}}@{{.*}}Int
  //
  // Loop count:
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: invoke i32 @{{.*}}Int
  // CHECK1: sub nsw i32
  // CHECK1: sub nsw i32
  // CHECK1: sdiv i32
  // CHECK1: add nsw i32
  // CHECK1: call {{.*}}@{{.*}}Int
  //
  // CHECK1: landingpad
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: landingpad
  // CHECK1: call {{.*}}@{{.*}}Int

  anchor(3);
  // CHECK1: call {{.*}}@{{.*}}anchor

  // Loop count should contain EWC from the increment
  _Cilk_for (int i = 0; i < 10000; i += 1 + Int(2012)) { }
  // Loop count:
  // CHECK1: sub nsw i32
  // CHECK1: sub nsw i32
  // CHECK1: call {{.*}}@{{.*}}Int
  // CHECK1: invoke i32 @{{.*}}Int
  // CHECK1: add nsw i32
  // CHECK1: sdiv i32
  // CHECK1: add nsw i32
  // CHECK1: call {{.*}}@{{.*}}Int
  //
  // CHECK1: landingpad
  // CHECK1: call {{.*}}@{{.*}}Int

  anchor(4);
  // CHECK1: call {{.*}}@{{.*}}anchor

  // Loop count should contain EWC from copying Long in operator<
  _Cilk_for (int i = 0; i < Long(); i++) { }
  // Condition:
  // CHECK1: call {{.*}}@{{.*}}Long
  // CHECK1: call {{.*}}@{{.*}}Long
  //
  // Loop count:
  // CHECK1: call {{.*}}@{{.*}}Long
  // CHECK1: call {{.*}}@{{.*}}Long
  // CHECK1: sub nsw i32
  // CHECK1: sdiv i32
  // CHECK1: add nsw i32
  // CHECK1: call {{.*}}@__cilkrts_cilk_for

  anchor(5);
  // CHECK1: call {{.*}}@{{.*}}anchor

  Long l;
  // Loop count should contain EWC from building i < Long()
  _Cilk_for (int i = 0; i < l; i++) { }
  // Condition:
  // CHECK1: call {{.*}}@{{.*}}Long
  // CHECK1: invoke {{.*}}@{{.*}}Long
  // CHECK1: call {{.*}}@{{.*}}Long
  //
  // Loop count:
  // CHECK1: invoke {{.*}}@{{.*}}Long
  // CHECK1: sub nsw i32
  // CHECK1: sdiv i32
  // CHECK1: add nsw i32
  // CHECK1: call {{.*}}@__cilkrts_cilk_for
}

void test_increment() {
  _Cilk_for (int i = 0; i < 10000; i += Int(2012)) {
    extern void anchor() throw();
    anchor();
  }
  // CHECK2: call {{.*}}@{{.*}}Int
  // CHECK2-NEXT: invoke i32 @{{.*}}Int
  // CHECK2-NOT: call @{{.*}}test_increment{{.*}}anchor
  //
  // Normal:
  // CHECK2: call {{.*}}@{{.*}}Int
  //
  // Excepting:
  // CHECK2: call {{.*}}@{{.*}}Int
  //
  // CHECK2: ret void
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

  // CHECK5: define {{.*}} void @{{.*}}simple_template
  // CHECK5: call {{.*}}@__cilkrts_cilk_for_32
  // CHECK5: ret void
  //
  // CHECK5: define {{.*}} void @{{.*}}simple_template
  // CHECK5: call {{.*}}@__cilkrts_cilk_for_64
  // CHECK5: ret void
}

namespace ns_void_increment {

struct A {
  A();
  operator int();
};

struct Int {
  Int(int);
  bool operator<(int);
};

void operator+=(Int&, int);
int operator-(int, Int);

void test_void_increment() {
  _Cilk_for (Int i = 0; i < 10; i += A());
  // CHECK6: define void @{{.*}}test_void_increment
  // CHECK6: call {{.*}}@__cilkrts_cilk_for_32
  // CHECK6: ret void
}

} // namespace

namespace ptr_conversion {
  struct X {
    X() {}
    operator int() { return 1; }
  };

  void test(int *p, int *q) {
    X x;
    _Cilk_for(int *r = p; r != q; r += x); // OK, should not crash.
  }
}

namespace adjust_expression {

struct C { };

C &operator+= (const C&, const int&);
bool operator< (const C&, const C&);
int operator- (const C&, const C&);
void operator++ (C&);

void foo(const C& limit) {
  _Cilk_for (C c;  c < limit; ++c); // OK, should not crash.
}

} // namespace
