// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify %s

namespace A {
  // expected-note@+1 4{{candidate found by name lookup is 'A::a'}}
  int a;
  namespace AA {
    // expected-note@+1 4{{candidate found by name lookup is 'A::AA::a'}}
    int a;
  }

  void test() {
    using namespace AA;

    // expected-error@+3 {{invalid variable in private clause}}
    // expected-error@+2 {{use of undeclared identifier 'b'}}
    // expected-error@+1 {{reference to 'a' is ambiguous}}
    #pragma simd private(a, b, test)
    for(int i = 0; i < 10; ++i);

    // expected-error@+3 {{invalid variable in lastprivate clause}}
    // expected-error@+2 {{use of undeclared identifier 'b'}}
    // expected-error@+1 {{reference to 'a' is ambiguous}}
    #pragma simd lastprivate(a, b, test)
    for(int i = 0; i < 10; ++i);

    // expected-error@+3 {{invalid variable in firstprivate clause}}
    // expected-error@+2 {{use of undeclared identifier 'b'}}
    // expected-error@+1 {{reference to 'a' is ambiguous}}
    #pragma simd firstprivate(a, b, test)
    for(int i = 0; i < 10; ++i);

    // expected-error@+3 {{invalid linear variable}}
    // expected-error@+2 {{use of undeclared identifier 'b'}}
    // expected-error@+1 {{reference to 'a' is ambiguous}}
    #pragma simd linear(a, b, test)
    for(int i = 0; i < 10; ++i);
  }
}

template <typename T>
void test(T t, int j0, int &j1, int &&j2) {
  extern int v[];
  // expected-error@+3 {{variable in private clause shall not be incomplete: 'int []'}}
  // expected-error@+2 {{variable in private clause shall not be a reference: 'int &'}}
  // expected-error@+1 {{variable in private clause shall not be a reference: 'int &&'}}
  #pragma simd private(t, v, j0, j1, j2)
  for(int i = 0; i < 10; ++i);

  // expected-error@+3 {{variable in lastprivate clause shall not be incomplete: 'int []'}}
  // expected-error@+2 {{variable in lastprivate clause shall not be a reference: 'int &'}}
  // expected-error@+1 {{variable in lastprivate clause shall not be a reference: 'int &&'}}
  #pragma simd lastprivate(t, v, j0, j1, j2)
  for(int i = 0; i < 10; ++i);

  // expected-error@+3 {{variable in firstprivate clause shall not be incomplete: 'int []'}}
  // expected-error@+2 {{variable in firstprivate clause shall not be a reference: 'int &'}}
  // expected-error@+1 {{variable in firstprivate clause shall not be a reference: 'int &&'}}
  #pragma simd firstprivate(t, v, j0, j1, j2)
  for(int i = 0; i < 10; ++i);
}

namespace ConstCheck {

struct ClassWithMutableField {
  mutable int x;
};

struct ClassWithoutMutableField {
  int x;
};

void test(const ClassWithMutableField x, const ClassWithoutMutableField y) {
  // expected-note@-1 2{{variable declared here}}

  // expected-error@+1 {{variable in private clause shall not be const-qualified}}
  #pragma simd private(x, y)
  for(int i = 0; i < 10; ++i);

  // expected-error@+1 {{variable in lastprivate clause shall not be const-qualified}}
  #pragma simd lastprivate(x, y)
  for(int i = 0; i < 10; ++i);

  #pragma simd firstprivate(x, y)
  for(int i = 0; i < 10; ++i);
}

} // namespace

namespace SpecialMemberCheck {

union U {
  int i;
  float f;
} u;

struct MultipleDefalutCtor {
  MultipleDefalutCtor(int x = 0);
  MultipleDefalutCtor(double x = 0);
};

struct NoDefalutCtor {
  NoDefalutCtor(int);
};

struct DeletedDefalutCtor {
  DeletedDefalutCtor() = delete;
};

void test_default_ctor(MultipleDefalutCtor MD, NoDefalutCtor ND, DeletedDefalutCtor DD) {
  // expected-error@+3 {{variable in private clause has ambiguous default constructors}}
  // expected-error@+2 {{variable in private clause has no default constructor or it is deleted}}
  // expected-error@+1 {{variable in private clause has no default constructor or it is deleted}}
  #pragma simd private(u, MD, ND, DD);
  for (int i = 0; i < 10; ++i);

  // expected-error@+3 {{variable in lastprivate clause has ambiguous default constructors}}
  // expected-error@+2 {{variable in lastprivate clause has no default constructor or it is deleted}}
  // expected-error@+1 {{variable in lastprivate clause has no default constructor or it is deleted}}
  #pragma simd lastprivate(u, MD, ND, DD);
  for (int i = 0; i < 10; ++i);
}

struct DeletedCopyConstructor{
  DeletedCopyConstructor();
  DeletedCopyConstructor(const DeletedCopyConstructor &) = delete;
};

void test_copy_ctor() {
  DeletedCopyConstructor DC;
  // expected-error@+1 {{variable in firstprivate clause has no default copy constructor or it is deleted}}
  #pragma simd firstprivate(DC)
  for (int i = 0; i < 10; ++i);
}

struct DeletedCopyAssignment {
  DeletedCopyAssignment();
  DeletedCopyAssignment(const DeletedCopyAssignment&);
  DeletedCopyAssignment& operator=(const DeletedCopyAssignment&) = delete;
};

void test_copy_assignment() {
  DeletedCopyAssignment DC;
  // expected-error@+1 {{variable in lastprivate clause has no copy assignment operator or it is deleted}}
  #pragma simd lastprivate(DC)
  for (int i = 0; i < 10; ++i);
}

} // namespace

namespace ConstExprCheck {
  constexpr int ce(const int i) { return 1 << i; }
  constexpr int ce_bad(const int i) { return 3 << i; }
  void test() {
    #pragma simd vectorlength(ce(2))
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+1 {{invalid vectorlength expression: must be a power of two}} */
    #pragma simd vectorlength(ce_bad(4))
    for (int i = 0; i < 10; ++i) ;

    int a;
    #pragma simd linear(a:ce(4))
    for (int i = 0; i < 10; ++i) ;
  }
} // namespace
