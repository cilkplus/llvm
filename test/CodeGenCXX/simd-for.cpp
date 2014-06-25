// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -emit-llvm %s     -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++11 -fcxx-exceptions -fexceptions -emit-llvm %s -O2 -o - | FileCheck %s --check-prefix=CHECKO2

void anchor(int) throw();
void touch(float) throw();
void touch(float*) throw();

// Scalar types
void test_private_variables1() {
  float a = 1.0f;
  #pragma simd private(a)
  for (int i = 0; i < 100; ++i) {
    a = i;
    anchor(101);
    touch(a);
    anchor(102);
  }
  // CHECK: call void @_Z6anchori(i32 101)
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: load float*
  // CHECK-NEXT: call void @_Z5touchf
  // CHECK-NEXT: call void @_Z6anchori(i32 102)

  float *pa = &a;
  #pragma simd private(pa)
  for (int i = 0; i < 100; ++i) {
    pa = 0;
    anchor(103);
    touch(pa);
    anchor(104);
  }
  // CHECK: call void @_Z6anchori(i32 103)
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: load float**
  // CHECK-NEXT: call void @_Z5touchPf
  // CHECK-NEXT: call void @_Z6anchori(i32 104)
}

struct A {
  A();
  ~A();
};

struct B {
  B(A a = A());
  ~B();
  void touch();
};

// default constructor with default arguments.
void test_private_variables2() {
  B b;
  anchor(200);
  #pragma simd private(b)
  for (int i = 0; i < 100; ++i) {
    anchor(201);
    b.touch();
    anchor(202);
  }
  anchor(203);
  // CHECK: call void @_Z6anchori(i32 200)
  //
  // CHECK: call void @_ZN1AC1Ev
  // CHECK-NEXT: call void @_ZN1BC1E1A
  // CHECK-NEXT: call void @_ZN1AD1Ev
  // CHECK-NEXT: call void @_Z6anchori(i32 201)
  // CHECK-NEXT: call void @_ZN1B5touchEv
  // CHECK-NEXT: call void @_Z6anchori(i32 202)
  //
  // CHECK: call void @_ZN1BD1Ev
  // CHECK: call void @_Z6anchori(i32 203)
}

struct D {
  D();
  mutable int val;
  void touch() const;
};

// const object with a mutable member.
void test_private_variables3() {
  const D d;

  anchor(300);
  #pragma simd private(d)
  for (int i = 0; i < 100; ++i) {
    anchor(301);
    d.touch();
    anchor(302);
  }
  anchor(303);
  // CHECK: call void @_Z6anchori(i32 300)
  // CHECK: call void @_ZN1DC1Ev
  // CHECK-NEXT: call void @_Z6anchori(i32 301)
  // CHECK-NEXT: call void @_ZNK1D5touchEv
  // CHECK-NEXT: call void @_Z6anchori(i32 302)
  // CHECK: call void @_Z6anchori(i32 303)
}

// FIXME: handle array type
void test_private_variables4() {
  float Arr[100] = {0};
  #pragma simd private(Arr)
  for (int i = 0; i < 100; ++i) {
    touch(Arr + i);
  }
}

void test_firstprivate_variables1() {
  float a = 1.0f;
  #pragma simd firstprivate(a)
  for (int i = 0; i < 100; ++i) {
    anchor(601);
    touch(a);
    anchor(602);
  }
  // CHECK: call void @_Z6anchori(i32 601)
  // CHECK-NEXT: load float*
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: call void @_Z5touchf
  // CHECK-NEXT: call void @_Z6anchori(i32 602)

  float *pa = &a;
  #pragma simd firstprivate(pa)
  for (int i = 0; i < 100; ++i) {
    anchor(603);
    touch(pa);
    anchor(604);
  }
  // CHECK: call void @_Z6anchori(i32 603)
  // CHECK-NEXT: load float**
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: call void @_Z5touchPf
  // CHECK-NEXT: call void @_Z6anchori(i32 604)
}

struct E {
  E();
  E(const E&);
  ~E();
  void touch();
};

void test_firstprivate_variables2() {
  E e;

  anchor(605);
  #pragma simd firstprivate(e)
  for (int i = 0; i < 100; ++i) {
    anchor(606);
    e.touch();
    anchor(607);
  }
  anchor(608);
  // CHECK: call void @_Z6anchori(i32 605)
  // CHECK: call void @_ZN1EC1ERKS_
  // CHECK-NEXT: call void @_Z6anchori(i32 606)
  // CHECK-NEXT: call void @_ZN1E5touchEv
  // CHECK-NEXT: call void @_Z6anchori(i32 607)
  // CHECK: call void @_ZN1ED1Ev
  // CHECK: call void @_Z6anchori(i32 608)
}

namespace ns_noinvoke {

struct S { S(); ~S(); };
S createS();
S useS(S);

void test1() {
  S s;
  #pragma simd
  for (int i = 0; i < 10; i++) {
    s = useS(createS());
  }
  // CHECK: test1
  // CHECK-NOT: invoke void
  // CHECK: ret void
}

} //namespace

void extern1();
void extern2();

void test2() {
  try {
    #pragma simd
    for (int i = 0; i < 10; i++) {
      extern1();
    }
    extern2();
  } catch (...) {}

  // CHECK: test2
  // CHECK: call void @_Z7extern1v()
  // CHECK: invoke void @_Z7extern2v()
}

void test_last_iteration_optimization() {
  int a = 0;
  #pragma simd linear(a)
  for (int i = 0; i < 1; i++) {
    anchor(700);
    (void)a;
  }

  #pragma simd lastprivate(a)
  for (int i = 0; i < 1; i++) {
    anchor(701);
    (void)a;
  }

  // CHECKO2: define void @_Z32test_last_iteration_optimizationv
  // CHECKO2: call void @_Z6anchori(i32 700)
  // CHECKO2: call void @_Z6anchori(i32 701)
  // CHECKO2-NOT: unreachable
  // CHECKO2: ret void 
}

class Private{
protected:
  int i;
public:
  Private () {
    i = 1234;
  }
  int change (int x) {
    i += x;
    return i;
  }
};

void check(){
  Private lastX;

#pragma simd lastprivate(lastX)
  for (int i = 0; i < 111; i++)
    lastX.change(i);

  // CHECK: entry:
  // CHECK-NOT: %agg-temp.i{{[1-9]*}} = alloca %class.Private
  // CHECK-NOT: %agg-temp.i = alloca %class.Private
  // CHECK: if.then:

}

