// RUN: %clang_cc1 -std=c++11 -fsyntax-only -verify -fcilkplus %s
// REQUIRES: cilkplus
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
  #pragma simd private(u, MD, ND, DD)
  for (int i = 0; i < 10; ++i);

  // expected-error@+3 {{variable in lastprivate clause has ambiguous default constructors}}
  // expected-error@+2 {{variable in lastprivate clause has no default constructor or it is deleted}}
  // expected-error@+1 {{variable in lastprivate clause has no default constructor or it is deleted}}
  #pragma simd lastprivate(u, MD, ND, DD)
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
    #pragma simd vectorlength(ce_bad(4))
    for (int i = 0; i < 10; ++i) ;

    int a;
    #pragma simd linear(a:ce(4))
    for (int i = 0; i < 10; ++i) ;
  }
} // namespace

namespace TypeOpCheck1 {
  struct A {
    A operator+=(A);
    A operator-=(A);
    A operator*=(A);
    A operator|=(A);
    A operator&=(A);
    A operator^=(A);
    bool operator&&(A); bool operator||(A);
  };
  void test1() {
    A a;
    #pragma simd reduction(+:a)
    for (int i = 0; i < 10; ++i) ;
    #pragma simd reduction(-:a)
    for (int i = 0; i < 10; ++i) ;
    #pragma simd reduction(*:a)
    for (int i = 0; i < 10; ++i) ;
    #pragma simd reduction(|:a)
    for (int i = 0; i < 10; ++i) ;
    #pragma simd reduction(&:a)
    for (int i = 0; i < 10; ++i) ;
    #pragma simd reduction(^:a)
    for (int i = 0; i < 10; ++i) ;
    #pragma simd reduction(&&:a)
    for (int i = 0; i < 10; ++i) ;
    #pragma simd reduction(||:a)
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+1 {{reduction operator max requires arithmetic type}} */
    #pragma simd reduction(max:a)
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+1 {{reduction operator min requires arithmetic type}} */
    #pragma simd reduction(min:a)
    for (int i = 0; i < 10; ++i) ;
  }
  struct B {
    B operator+(B);
    B operator-(B);
  };
  B operator^=(B, B);
  bool operator||(B, A); // expected-note {{candidate function not viable: no known conversion from}}
  void test2() {
    B b;
    /* expected-error@+1 {{no viable overloaded '+='}} */
    #pragma simd reduction(+:b)
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+1 {{no viable overloaded '-='}} */
    #pragma simd reduction(-:b)
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+1 {{no viable overloaded '*='}} */
    #pragma simd reduction(*:b)
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+1 {{no viable overloaded '|='}} */
    #pragma simd reduction(|:b)
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+2 {{not contextually convertible to 'bool'}} */
    /* expected-error@+1 {{invalid operand}} */
    #pragma simd reduction(&&:b)
    for (int i = 0; i < 10; ++i) ;
  }
} // namespace

namespace TypeOpCheck2 {
  TypeOpCheck1::B b;
  void test() {
    #pragma simd reduction(^:b)
    for (int i = 0; i < 10; ++i) ;
    /* expected-error@+2 {{not contextually convertible to 'bool'}} */
    /* expected-error@+1 {{invalid operand}} */
    #pragma simd reduction(||:b)
    for (int i = 0; i < 10; ++i) ;
  }
} // namespace

namespace TemplateTests {

template <int length>
void test_simd_length() {
  #pragma simd vectorlength(length)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_length2(T length) {
  #pragma simd vectorlength(length)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_length_for() {
  #pragma simd
  for (int i = 0; i < 10; i++) {}
}

struct S { int i; };

template <typename T, typename S>
void test_simd_linear() {
  T t;
  S step;
  #pragma simd linear(t:step)
  for (int i = 0; i < 10; i++) {}

  #pragma simd linear(t:sizeof(T))
  for (int i = 0; i < 10; i++) {}
}

template <typename T1, typename T2>
void test_simd_private() {
  T1 x;
  T2 y;
  T1 z;
  #pragma simd private(x, y, z)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_private2() {
  T x;
  #pragma simd private(x.i)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_private3() {
  const T x;
  #pragma simd private(x)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_private4() {
  T x;
  T &r = x;
  #pragma simd private(r)
  for (int i = 0; i < 10; i++) {}
}

template <typename T1, typename T2>
void test_simd_firstprivate() {
  T1 x;
  T2 y;
  #pragma simd firstprivate(x, y)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_firstprivate2() {
  T x;
  #pragma simd firstprivate(x.x)
  for (int i = 0; i < 10; i++) {}
}

template <typename T1, typename T2>
void test_simd_lastprivate() {
  T1 x;
  T2 y;
  #pragma simd lastprivate(x, y)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_lastprivate2() {
  T x;
  #pragma simd lastprivate(x.x)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_reduction() {
  T x;
  #pragma simd reduction(+:x)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_reduction2() {
  T x[2];
  #pragma simd reduction(+:x)
  for (int i = 0; i < 10; i++) {}
}

template <typename T1, typename T2, typename T3>
void test_simd_multiple_clauses() {
  T1 x, x2;
  const T2 y = 4;
  T3 z;
  #pragma simd vectorlength(y) reduction(+:x,x2) lastprivate(z)
  for (int i = 0; i < 10; i++) {}
}

template <typename T>
void test_simd_multiple_clauses2() {
  T x;
  #pragma simd vectorlength(x) reduction(-:x) private(x)
  for (int i = 0; i < 10; ++i) {}
}

void templated_tests() {
  test_simd_length<2>(); // OK
  test_simd_length<-2>();

  test_simd_length2(2); // expected-error@249 {{vectorlength expression must be an integer constant}} \
                        // expected-note {{in instantiation of function template specialization}}

  test_simd_length_for<long>(); // OK
  test_simd_length_for<void>(); // OK
                                //

  test_simd_linear<int, int>(); // OK
  test_simd_linear<int, float>(); // expected-error@265 {{invalid linear step: expected integral constant or variable reference}} \
                                  // expected-note {{in instantiation of function template specialization}}
  test_simd_linear<S, int>(); // expected-error@265 {{invalid linear variable}} \
                              // expected-error@268 {{invalid linear variable}} \
                              // expected-note {{in instantiation of function template specialization}}

  test_simd_private<float, long>(); // OK
  test_simd_private2<S>(); // expected-error@284 {{expected ','}} \
                           // expected-error@284 {{expected unqualified-id}}

  test_simd_private3<S>(); // expected-error@291 {{variable in private clause shall not be const-qualified}} \
                           // expected-error@290 {{default initialization of an object of const type}} \
                           // expected-note@290 {{variable declared here}} \
                           // expected-note {{in instantiation of function template specialization}}

  test_simd_private4<int>(); // expected-error@299 {{variable in private clause shall not be a reference}}

  test_simd_firstprivate<long, float>(); // OK

  test_simd_firstprivate2<S>(); // expected-error@314 {{expected ','}} \
                                // expected-error@314 {{expected unqualified-id}}

  test_simd_lastprivate<double, float>();

  test_simd_lastprivate2<S>(); // expected-error@329 {{expected ','}} \
                               // expected-error@329 {{expected unqualified-id}}

  test_simd_reduction<int>(); // OK

  test_simd_reduction2<int>(); // expected-error@343 {{variable in reduction clause shall not be of array type}} \
                               // expected-note@342 {{declared here}}

  test_simd_multiple_clauses<float, long, double>(); // OK

  test_simd_multiple_clauses2<float>(); // expected-error@359 {{private variable shall not appear in multiple simd clauses}} \
                                        // expected-note@359 {{first used here}} \
                                        // expected-error@359 {{vectorlength expression must be an integer constant}} \
                                        // expected-note {{in instantiation of function template specialization}}
}

void use(int x);

struct X {
  static int x;
};
static int G;
void test_private() {
  static int L;
  #pragma simd private(X::x)
  for (int i = 0; i < 10; ++i) use(X::x); // OK
  #pragma simd private(G)
  for (int i = 0; i < 10; ++i) use(G); // OK
  #pragma simd private(L)
  for (int i = 0; i < 10; ++i) use(L); // OK

  [&]{
    #pragma simd private(L) private(G) private(X::x)
    for (int i = 0; i < 10; ++i) {
      use(L);
      use(G);
      use(X::x);
    }
  }();
  #pragma simd private(L) private(G) private(X::x)
  for (int i = 0; i < 10; ++i) {
    [&] {
      [&] {
        use(L);
        use(G);
        use(X::x);
      }();
    }();
  }
}

} // namespace

namespace test_vectorlength
{
const int C0 = 0;
const int C1 = 1;
const int C2 = 2;
const char CC = (char)32;
const short CS = (short)32;

void test() {
  int j;
  #pragma simd vectorlength(CC)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd vectorlength(CS)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd vectorlength(C1)
  for (int i = 0; i < 10; ++i) ;
  #pragma simd vectorlength(C2+C2)
  for (int i = 0; i < 10; ++i) ;

  /* expected-error@+1 {{expected for statement following '#pragma simd'}} */
  #pragma simd vectorlength(C1+C2)
  /* expected-error@+1 {{vectorlength expression must be an integer constant}} */
  #pragma simd vectorlength(C1+j)
}
} //namespace
