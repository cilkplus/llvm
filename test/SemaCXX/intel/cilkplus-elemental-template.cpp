// RUN: %clang_cc1 -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s
// REQUIRES: cilkplus

# define ATTR(x)  __attribute__((x))

namespace test_vectorlength {

// FIXME: source location of each note is incorrect here.

template <int VL>
__attribute__((vector(vectorlength(VL))))  // expected-error {{vectorlength must be positive}}
                                           
void non_type();

template <int VL>
  __attribute__((vector(vectorlength(VL))))  // expected-error {{vectorlength expression must be a power of 2}}
  void non_type2();
template <int V1, int V2>
  __attribute__((vector(vectorlength(V1 + V2)))) // expected-error {{vectorlength expression must be a power of 2}}
  void non_type();

template <typename T>
__attribute__((vector(vectorlength(sizeof(T)))))
void simple();

enum {
  C0 = 0, C1 = 1, C2 = 2, C3 = 3
};

const int VL = 4;
template <int C>
  __attribute__((vector(vectorlength(VL + C)))) // expected-error {{vectorlength expression must be a power of 2}} \
                                                // expected-error {{vectorlength expression must be a power of 2}} \
                                                // expected-error {{vectorlength expression must be a power of 2}}
  void constant();
  
extern const int E;

template <int C>
__attribute__((vector(vectorlength(E + C)))) // expected-error {{vectorlength attribute requires an integer constant}}
void extern_constant();

void test1() {
  non_type<4>();    // OK
  non_type<0>();    // expected-note {{in instantiation of function template specialization 'test_vectorlength::non_type<0>' requested here}}
  non_type2<128>(); // OK
  non_type2<3>();   // expected-note {{in instantiation of function template specialization 'test_vectorlength::non_type2<3>' requested here}}
  non_type<2, 3>(); // expected-note {{in instantiation of function template specialization 'test_vectorlength::non_type<2, 3>'}}
  non_type<4, 4>(); // OK

  simple<char>();   // OK
  simple<int>();    // OK
  simple<float>();  // OK
  simple<double>(); // OK

  constant<C0>();  // OK
  constant<C1>();  // expected-note {{in instantiation of function template specialization 'test_vectorlength::constant<1>' requested here}}
  constant<C2>();  // expected-note {{in instantiation of function template specialization 'test_vectorlength::constant<2>' requested here}}
  constant<C3>();  // expected-note {{in instantiation of function template specialization 'test_vectorlength::constant<3>' requested here}}

  extern_constant<4>(); // expected-note {{in instantiation of function template specialization 'test_vectorlength::extern_constant<4>' requested here}}
}

template <typename... Ts>
__attribute__((vector(vectorlength(sizeof...(Ts)))))  // expected-error {{vectorlength expression must be a power of 2}}
void pack();

void test2() {
  pack<char, int>();        // OK
  pack<char, int, float>(); // expected-note {{in instantiation of function template specialization 'test_vectorlength::pack<char, int, float>'}}
}

} // test_vectorlength

namespace test_no_instantiation {
template <class C>
ATTR(vector(linear(linear_var))) // OK
void test_template_linear0(C linear_var);

// spec issue (linear step)
template <int *Step>
ATTR(vector(linear(linear_var:Step))) // expected-error {{linear step has non-integer type 'int *'}}
void test_template_step1(int linear_var);

template <class C, int *Step>
struct Y {
  ATTR(vector(linear(linear_var)))
  void test_template_struct1(C linear_var);

  // Spec issue (linear step)
  ATTR(vector(linear(linear_var:Step))) // expected-error {{linear step has non-integer type 'int *'}}
  void test_template_step1(int linear_var);

  ATTR(vector(vectorlength(Step)))  // expected-error {{vectorlength attribute requires an integer constant}}
  void test_template_vectorlength1();
};
} // test_no_instantiation

// tests with instantiation
template <int I>
ATTR(vector(vectorlength(I))) // OK
void test_template_vectorlength1();

template <typename T>
ATTR(vector(uniform(uniform_var))) // OK
void test_template_uniform1(T uniform_var);

template <int I>
ATTR(vector(linear(linear_var:I))) // OK
void test_template_linear1(int linear_var);

template <typename T>
ATTR(vector(linear(linear_var))) // expected-error{{linear parameter type 'X' is not an integer or pointer type}}
void test_template_linear2(T linear_var);

template <typename T, int I, typename N>
ATTR(vector(linear(linear_var), vectorlength(I), uniform(uniform_var)))
void test_template_mixed1(T linear_var, N uniform_var);

template <typename T, int I>
struct S {
  ATTR(vector(uniform(uniform_var)))
  void test_template_struct1(T uniform_var);

  ATTR(vector(vectorlength(I), linear(linear_var:I))) // expected-error {{linear parameter type 'X' is not an integer or pointer type}}
  void test_template_struct2(T linear_var);

  ATTR(vector(vectorlength(I)))
  void test_template_struct3();
};

// parameter packs
template <typename T, typename... Ts>
ATTR(vector(uniform(uniform_var))) // OK
void test_template_parameterpack1(T uniform_var, Ts... args);

template <typename... Ts>
ATTR(vector(uniform(args...))) // expected-error {{template parameter pack is not supported in 'uniform' attribute}}
void test_template_parameterpack2(Ts... args);

template <typename... Ts>
ATTR(vector(linear(args...))) // expected-error {{template parameter pack is not supported in 'linear' attribute}}
void test_template_parameterpack3(Ts... args);


// SFINAE
template <typename T>
ATTR(vector(linear(linear_var)))
void sfinae1(typename T::foo linear_var) {}

template <typename T>
ATTR(vector(linear(linear_var)))
void sfinae1(T linear_var) {}

// Instantiations
struct X { X(); };
const char CC = 4;
constexpr int ConstVal() { return 2; }
enum E{ E1, E2 };
void template_tests() {
  X x;
  int i;
  char *c;
  E e;

  test_template_vectorlength1<1>();  // OK
  test_template_vectorlength1<CC>(); // OK
  test_template_vectorlength1<ConstVal()>(); // OK
  test_template_vectorlength1<sizeof(e)>();  // OK

  test_template_uniform1<int>(i);  // OK
  test_template_uniform1<decltype(i)>(i); // OK
  test_template_uniform1<X*>(&x); // OK
  test_template_uniform1<X>(x);   // OK

  test_template_linear1<64>(i); // OK
  test_template_linear1<CC>(i); // OK
  test_template_linear1<ConstVal()>(i); // OK
  test_template_linear1<sizeof(X)>(i);  // OK

  test_template_linear2<int>(i); // OK
  test_template_linear2<X>(x);   // expected-note {{in instantiation of function template specialization 'test_template_linear2<X>' requested here}}
  test_template_linear2<X*>(&x); // OK
  test_template_linear2<decltype(i)>(i); // OK

  test_template_mixed1<int, 16, char *>(i, c); // OK
  test_template_mixed1<decltype(i), sizeof(e), decltype(c)>(i, c); // OK

  S<int, 8> s1; // OK
  s1.test_template_struct1(i); // OK
  s1.test_template_struct2(i); // Not OK
  s1.test_template_struct3();  // OK

  S<decltype(i), sizeof(int)> s2; // OK
  (void)s2;
  S<int, CC> s3; // OK
  (void)s3;

  S<X, 4> s4; // expected-note {{here}}

  test_template_parameterpack1<int, int*, char*>(i, &i, c); // OK
  test_template_parameterpack2<int, X, int*, char*, E>(i, x, &i, c, e); // Not OK
  test_template_parameterpack3<int, int*, char*, E>(i, &i, c, e); // Not OK

  sfinae1<int>(8); // OK
}

namespace test_linear_step {

constexpr int ConstVal() { return 5; }

template <int I>
__attribute__((vector(linear(linear_var:I)))) // OK
void integer1(int linear_var);

template <int I>
__attribute__((vector(linear(linear_var:2+I)))) // OK
void integer2(int linear_var);

template <int I, int J>
__attribute__((vector(linear(linear_var:I+J)))) // OK
void integer3(int linear_var);

template <typename T>
__attribute__((vector(linear(linear_var:step), uniform(step)))) // expected-error 2{{linear step has non-integer type}}
void typed(int linear_var, T step);

void test1() {
  int x, y;
  char *c;
  integer1<4>(x); // OK
  integer1<ConstVal()>(x); // OK
  integer1<sizeof(x)>(x); // OK

  integer2<7>(x); // OK
  integer2<ConstVal()>(x); // OK
  integer2<sizeof(x)>(x); // OK

  integer3<7, 11>(x); // OK
  integer3<ConstVal(), 64>(x); // OK
  integer3<sizeof(x), ConstVal()>(x); // OK

  typed<int>(x, y); // OK
  typed<void*>(x, c); // expected-note {{in instantiation of function template specialization 'test_linear_step::typed<void *>' requested here}}
  typed<decltype(c)>(x, c); // expected-note {{in instantiation of function template specialization 'test_linear_step::typed<char *>' requested here}}
}

const int s = 1;
template <int ssss>
struct Class {
  static const int ss = 2;
  enum { sss = 3 };
  __attribute__((vector(linear(x:s))))       // OK  
  __attribute__((vector(linear(x:ss))))      // OK  
  __attribute__((vector(linear(x:sss))))     // OK  
  __attribute__((vector(linear(x:ssss))))    // OK  
  int foo(int x);
};

} // test_linear_step
