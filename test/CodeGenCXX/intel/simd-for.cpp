// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions -emit-llvm %s     -o - | FileCheck %s
// RUN: %clang_cc1 -std=c++11 -fcilkplus -fcxx-exceptions -fexceptions -emit-llvm %s -O2 -o - | FileCheck --check-prefix=CHECKO2 %s
// REQUIRES: cilkplus

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
  // CHECK: call void @{{.*}}anchor
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: load float, float*
  // CHECK-NEXT: call void @{{.*}}touch
  // CHECK-NEXT: call void @{{.*}}anchor

  float *pa = &a;
  #pragma simd private(pa)
  for (int i = 0; i < 100; ++i) {
    pa = 0;
    anchor(103);
    touch(pa);
    anchor(104);
  }
  // CHECK: call void @{{.*}}anchor
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: load float*, float**
  // CHECK-NEXT: call void @{{.*}}touch
  // CHECK-NEXT: call void @{{.*}}anchor
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
  // CHECK: call void @{{.*}}anchor
  //
  // CHECK: call void @{{.+}}
  // CHECK: call void @{{.*}}anchor
  // CHECK: call void @{{.*}}touch
  // CHECK: call void @{{.*}}anchor
  //
  // CHECK: call void @{{.+}}
  // CHECK: call void @{{.*}}anchor
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
  // CHECK: call void @{{.*}}anchor
  // CHECK: call {{.*}}@{{.+}}
  // CHECK: call void @{{.*}}anchor
  // CHECK-NEXT: call void @{{.*}}touch
  // CHECK-NEXT: call void @{{.*}}anchor
  // CHECK: call void @{{.*}}anchor
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
  // CHECK: call void @{{.*}}anchor
  // CHECK-NEXT: load float, float*
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: call void @{{.*}}touch
  // CHECK-NEXT: call void @{{.*}}anchor

  float *pa = &a;
  #pragma simd firstprivate(pa)
  for (int i = 0; i < 100; ++i) {
    anchor(603);
    touch(pa);
    anchor(604);
  }
  // CHECK: call void @{{.*}}anchor
  // CHECK-NEXT: load float*, float**
  // CHECK-NOT: getelementptr
  // CHECK-NEXT: call void @{{.*}}touch
  // CHECK-NEXT: call void @{{.*}}anchor
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
  // CHECK: call void @{{.*}}anchor
  // CHECK: call void @{{.*}}anchor
  // CHECK-NEXT: call void @{{.*}}touch
  // CHECK-NEXT: call void @{{.*}}anchor
  // CHECK: call void @{{.*}}anchor
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

  // CHECKO2: define void @{{.*}}test_last_iteration_optimization
  // CHECKO2: call void @{{.*}}anchor
  // CHECKO2: call void @{{.*}}anchor
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

  // CHECK-NOT: %agg-temp.i{{[1-9]*}} = alloca %class.Private
  // CHECK-NOT: %agg-temp.i = alloca %class.Private

}
