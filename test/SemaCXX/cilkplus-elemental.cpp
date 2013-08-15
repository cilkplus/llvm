// RUN: %clang_cc1 -DCXX11 -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s
// RUN: %clang_cc1 -DGNU   -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s
// RUN: %clang_cc1 -DMS    -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s

#ifdef GNU
# define ATTR(x)  __attribute__((x))
#endif
#ifdef MS
# define ATTR(x)  __declspec(x)
#endif
#ifndef ATTR
# define ATTR(x)  [[x]]
#endif

// processor clause
ATTR(vector(processor)) // expected-error {{'processor' attribute requires an identifier}}
ATTR(vector(processor(ceci_nest_pas_un_processor))) // expected-error {{unrecognized processor}}
ATTR(vector(processor(pentium_4), processor(pentium_4_sse3))) // expected-warning {{inconsistent processor attribute}} \
                                                            // expected-note {{previous attribute is here}}
ATTR(vector(processor(pentium_4)))         // OK
ATTR(vector(processor(pentium_4_sse3)))    // OK
ATTR(vector(processor(core_2_duo_ssse3)))  // OK
ATTR(vector(processor(core_2_duo_sse4_1))) // OK
ATTR(vector(processor(core_i7_sse4_2)))    // OK
ATTR(vector(processor(core_2nd_gen_avx)))  // OK
ATTR(vector(processor(core_3rd_gen_avx)))  // OK
ATTR(vector(processor(core_4th_gen_avx)))  // OK

int test_processor_1(int x);

// vectorlengthfor clause
ATTR(vector(vectorlengthfor)) // expected-error {{attribute takes one argument}}
ATTR(vector(vectorlengthfor(int), vectorlengthfor(float))) // expected-warning {{inconsistent vectorlengthfor attribute}} \
                                                           // expected-note {{previous attribute is here}}
int test_vectorlengthfor_1(int x);

// vectorlength clause
const int VL = 2;
ATTR(vector(vectorlength))      // expected-error {{attribute takes one argument}}
ATTR(vector(vectorlength(-1)))  // expected-error {{vectorlength must be positive}}
ATTR(vector(vectorlength(0)))   // expected-error {{vectorlength must be positive}}
ATTR(vector(vectorlength(3)))   // expected-error {{vectorlength expression must be a power of 2}}
ATTR(vector(vectorlength(int))) // expected-error {{attribute takes one argument}}
ATTR(vector(vectorlength(4), vectorlength(8))) // expected-warning {{repeated vectorlength attribute}} \
                                               // expected-note {{previous attribute is here}}
ATTR(vector(vectorlength(VL)))      // OK
ATTR(vector(vectorlength(VL + 2)))  // OK
int test_vectorlength_1(int x);

// uniform clause
ATTR(vector(uniform))    // expected-error {{'uniform' attribute requires an identifier}}
ATTR(vector(uniform()))  // expected-error {{expected identifier}}
ATTR(vector(uniform(1))) // expected-error {{expected identifier}}
ATTR(vector(uniform(w))) // expected-error {{not a function parameter}}
ATTR(vector(uniform(z))) // OK
ATTR(vector(uniform(y))) // OK
ATTR(vector(uniform(x), uniform(x))) // expected-error {{uniform attribute already specified for parameter 'x'}} \
                                     // expected-note {{previous attribute is here}}
int test_uniform_1(int x, int &y, float z);

// linear clause
ATTR(vector(linear(x))) // OK
ATTR(vector(linear))    // expected-error {{'linear' attribute requires an identifier}}
ATTR(vector(linear()))  // expected-error {{expected identifier}}
ATTR(vector(linear(1))) // expected-error {{expected identifier}}
ATTR(vector(linear(w))) // expected-error {{not a function parameter}}
ATTR(vector(linear(x:)))             // expected-error {{expected expression}}
ATTR(vector(linear(x), linear(x)))   // expected-error {{linear attribute already specified for parameter 'x'}} \
                                     // expected-note {{previous attribute is here}}
ATTR(vector(linear(x), linear(x:2))) // expected-error {{linear attribute inconsistent with previous linear attribute}} \
                                     // expected-note {{previous attribute is here}}
ATTR(vector(linear(x), uniform(x)))  // expected-error {{linear attribute inconsistent with previous uniform attribute}} \
                                     // expected-note {{previous attribute is here}}
ATTR(vector(linear(x:w)))            // expected-error {{not a function parameter}}
ATTR(vector(linear(x:y)))            // expected-error {{linear step parameter must also be uniform}}
ATTR(vector(linear(z))) // expected-error {{linear parameter must have integral or pointer type}}
ATTR(vector(linear(r))) // expected-error {{linear parameter must have integral or pointer type}}
int test_linear_1(int &r, int x, int y, float z);

// mask clause
ATTR(vector(mask(1))) // expected-error {{attribute takes no arguments}}
int test_mask_1(int x);

#if 0 // template tests disabled for now
namespace { // templates
struct X {
  X();
};

// tests with no instantiation
template <class C>
ATTR(vector(linear(linear_var))) // OK
void test_template_linear0(C linear_var);

#if 0 // spec issue (linear step)
template <int *Step>
ATTR(vector(linear(linear_var:Step))) // expected-error {{linear-step must be an integer constant expression}}
void test_template_step1(int linear_var);
#endif

template <class C, int *Step>
struct Y {
  ATTR(vector(linear(linear_var))) // expected-error {{linear parameter must have integral or pointer type}}
  void test_template_struct1(C linear_var);

#if 0 // Spec issue (linear step)
  ATTR(vector(linear(linear_var:Step))) // expected-error {{linear-step must be an integer constant expression}}
  void test_template_step1(int linear_var);
#endif

  ATTR(vector(vectorlength(Step)))  // expected-error {{'vectorlength' attribute requires integer constant}}
  void test_template_vectorlength1();
};

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

// parameter packs
template <typename T, typename... Ts>
ATTR(vector(uniform(uniform_var))) // OK
void test_template_parameterpack1(T uniform_var, Ts... args);

template <typename... Ts>
ATTR(vector(uniform(args...))) // OK
void test_template_parameterpack2(Ts... args);

template <typename... Ts>
ATTR(vector(linear(args...))) // OK
void test_template_parameterpack3(Ts... args);

#if 0
template <typename... Ts>
ATTR(vector(vectorlengthfor(Ts...))) // expected-error {{parameter pack not allowed in vectorlengthfor}}
void test_template_parameterpack4(Ts... args);
#endif

// SFINAE
template <typename T>
ATTR(vector(linear(linear_var)))
void sfinae1(typename T::foo linear_var) {}

template <typename T>
ATTR(vector(linear(linear_var)))
void sfinae1(T linear_var) {}

// Instantiations
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
} // namespace
#endif //template tests