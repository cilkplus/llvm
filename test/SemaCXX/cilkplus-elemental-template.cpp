// RUN: %clang_cc1 -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s

# define ATTR(x)  __attribute__((x))

namespace test_vectorlength {

// FIXME: source location of each note is incorrect here.

template <int VL>
__attribute__((vector(vectorlength(VL))))  // expected-error {{vectorlength expression must be a power of 2}} \
                                           // expected-error {{vectorlength must be positive}}
void non_type(); // expected-note 2{{here}}

template <int V1, int V2>
__attribute__((vector(vectorlength(V1 + V2)))) // expected-error {{vectorlength expression must be a power of 2}}
void non_type(); // expected-note {{here}}

template <typename T>
__attribute__((vector(vectorlength(sizeof(T)))))
void simple();

enum {
  C0 = 0, C1 = 1, C2 = 2, C3 = 3
};

const int VL = 4;
template <int C>
__attribute__((vector(vectorlength(VL + C)))) // expected-error 3{{vectorlength expression must be a power of 2}}
void constant(); // expected-note 3{{here}}

extern const int E;

template <int C>
__attribute__((vector(vectorlength(E + C)))) // expected-error {{vectorlength attribute requires an integer constant}}
void extern_constant(); // expected-note {{here}}

void test1() {
  non_type<4>();    // OK
  non_type<0>();    // not OK
  non_type<3>();    // not OK
  non_type<2, 3>(); // not OK
  non_type<4, 4>(); // OK

  simple<char>();   // OK
  simple<int>();    // OK
  simple<float>();  // OK
  simple<double>(); // OK

  constant<C0>();  // OK
  constant<C1>();  // not OK
  constant<C2>();  // not OK
  constant<C3>();  // not OK

  extern_constant<4>(); // not OK
}

template <typename... Ts>
__attribute__((vector(vectorlength(sizeof...(Ts))))) // expected-error {{vectorlength expression must be a power of 2}}
void pack(); // expected-note {{here}}

void test2() {
  pack<char, int>();        // OK
  pack<char, int, float>(); // not OK
}

} // test_vectorlength


namespace test_no_instantiation {
#if 0
template <class C>
ATTR(vector(linear(linear_var))) // OK
void test_template_linear0(C linear_var);

// spec issue (linear step)
template <int *Step>
ATTR(vector(linear(linear_var:Step))) // expected-error {{linear-step must be an integer constant expression}}
void test_template_step1(int linear_var);
#endif

template <class C, int *Step>
struct Y {
  ATTR(vector(linear(linear_var))) // expected-error {{linear parameter must have integral or pointer type}}
  void test_template_struct1(C linear_var);

#if 0
  // Spec issue (linear step)
  ATTR(vector(linear(linear_var:Step))) // expected-error {{linear-step must be an integer constant expression}}
  void test_template_step1(int linear_var);

  ATTR(vector(vectorlength(Step)))  // expected-error {{'vectorlength' attribute requires integer constant}}
  void test_template_vectorlength1();
#endif
};
} // test_no_instantiation

// disabled tests - re-enable when features are implemented
#if 0
// tests with instantiation
template <typename T>
ATTR(vector(vectorlengthfor(T))) // OK
void test_template_vectorlengthfor1();

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
ATTR(vector(linear(linear_var)))
void test_template_linear2(T linear_var);

template <typename T, int I, typename N>
ATTR(vector(linear(linear_var), vectorlength(I), uniform(uniform_var)))
void test_template_mixed1(T linear_var, N uniform_var);

template <typename T, int I>
struct S {
  ATTR(vector(vectorlengthfor(T), uniform(uniform_var)))
  void test_template_struct1(T uniform_var);

  ATTR(vector(vectorlength(I), linear(linear_var:I))) // expected-error {{linear parameter must have integral or pointer type}}
  void test_template_struct2(T linear_var);

  ATTR(vector(vectorlength(I)))
  void test_template_struct3();

  template <typename N>
  ATTR(vector(vectorlengthfor(N)))
  void test_template_struct4(T arg);
};
#endif // template tests

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

#if 0
template <typename... Ts>
ATTR(vector(vectorlengthfor(Ts...))) // expected-error {{parameter pack not allowed in vectorlengthfor}}
void test_template_parameterpack4(Ts... args);

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

  test_template_vectorlengthfor1<int>();         // OK
  test_template_vectorlengthfor1<decltype(i)>(); // OK
  test_template_vectorlengthfor1<X>(); // OK
  test_template_vectorlengthfor1<E>(); // OK

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
  test_template_linear2<X>(x);   // OK
  test_template_linear2<X*>(&x); // OK
  test_template_linear2<decltype(i)>(i); // OK

  test_template_mixed1<int, 16, char *>(i, c); // OK
  test_template_mixed1<decltype(i), sizeof(e), decltype(c)>(i, c); // OK

  S<int, 8> s1; // OK
  s1.test_template_struct1(i); // OK
  s1.test_template_struct2(i); // OK
  s1.test_template_struct3();  // OK
  s1.test_template_struct4<X>(i); // OK

  S<decltype(i), sizeof(int)> s2; // OK
  (void)s2;
  S<int, CC> s3; // OK
  (void)s3;

  S<X, 4> s4;

  test_template_parameterpack1<int, int*, char*>(i, &i, c); // OK
  test_template_parameterpack2<int, X, int*, char*, E>(i, x, &i, c, e); // OK
  test_template_parameterpack3<int, int*, char*, E>(i, &i, c, e); // OK

  sfinae1<int>(8); // OK
}

#endif //template tests
