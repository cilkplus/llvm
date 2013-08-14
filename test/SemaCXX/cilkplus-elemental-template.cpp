// RUN: %clang_cc1 -fcilkplus -std=c++11 -fsyntax-only -verify -Wall %s

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
