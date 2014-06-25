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
ATTR(vector(processor)) // expected-error {{'processor' attribute takes one argument}}
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

// vectorlength clause
const int VL = 2;
ATTR(vector(vectorlength))      // expected-error {{attribute takes one argument}}
ATTR(vector(vectorlength(-1)))  // expected-error {{vectorlength must be positive}}
ATTR(vector(vectorlength(0)))   // expected-error {{vectorlength must be positive}}
ATTR(vector(vectorlength(3)))
ATTR(vector(vectorlength(int))) // expected-error {{expected '(' for function-style cast or type construction}}
ATTR(vector(vectorlength(4), vectorlength(8))) // expected-error {{repeated vectorlength attribute is not allowed}} \
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
ATTR(vector(uniform(x), uniform(x))) // expected-error {{parameter 'x' cannot be the subject of two elemental clauses}} \
                                     // expected-note {{another uniform clause here}}
int test_uniform_1(int x, int &y, float z); // expected-note {{parameter here}}

// linear clause
ATTR(vector(linear(x))) // OK
ATTR(vector(linear))    // expected-error {{'linear' attribute requires an identifier}}
ATTR(vector(linear()))  // expected-error {{expected identifier}}
ATTR(vector(linear(1))) // expected-error {{expected identifier}}
ATTR(vector(linear(w))) // expected-error {{not a function parameter}}
ATTR(vector(linear(x:)))             // expected-error {{expected expression}}
ATTR(vector(linear(x), linear(x)))   // expected-error {{parameter 'x' cannot be the subject of two elemental clauses}} \
                                     // expected-note {{clause here}}
ATTR(vector(linear(x), linear(x:2))) // expected-error {{parameter 'x' cannot be the subject of two elemental clauses}} \
                                     // expected-note {{another linear clause here}}
ATTR(vector(linear(x), uniform(x)))  // expected-error {{parameter 'x' cannot be the subject of two elemental clauses}} \
                                     // expected-note {{here}}
ATTR(vector(linear(x:w)))            // expected-error {{use of undeclared identifier 'w'}}
ATTR(vector(linear(x:y)))            // expected-error {{linear step parameter must also be uniform}}
ATTR(vector(linear(z))) // expected-error {{linear parameter type 'float' is not an integer or pointer type}}
ATTR(vector(linear(r))) // OK
int test_linear_1(int &r, int x, int y, float z); // expected-note 3{{parameter here}}

const int s = 1;
ATTR(vector(linear(x:s)))           // OK
ATTR(vector(linear(x:ss)))          // expected-error {{use of undeclared identifier}}
ATTR(vector(linear(x:s, y:ss)))     // expected-error {{use of undeclared identifier}}
ATTR(vector(linear(x:s+1, y:1+s)))  // OK
int test_linear_step(int x, int y);

template <int N>
ATTR(vector(linear(x:N)))           // OK
int test_linear_step(int x);

struct Class {};                    // expected-note {{declared here}}
ATTR(vector(linear(x:Class)))       // expected-error {{does not refer to a value}}
int test_linear_step(int x);

struct Class2 {
  static const int ss = 2;
  enum { sss = 3 };
  ATTR(vector(linear(x:s)))         // OK
  ATTR(vector(linear(x:ss)))        // OK  
  ATTR(vector(linear(x:sss)))       // OK  
  int test_linear_step(int x);
};

// mask clause
ATTR(vector(mask(1))) // expected-error {{attribute takes no arguments}}
ATTR(vector(mask, nomask)) // expected-error {{elemental function cannot have both mask and nomask attributes}} \
                           // expected-note {{here}}
ATTR(vector(nomask, mask)) // expected-error {{elemental function cannot have both mask and nomask attributes}} \
                           // expected-note {{here}}
int test_mask_1(int x);

// linear and uniform clauses with this
namespace test_this {

ATTR(vector(uniform(this))) // expected-error {{invalid use of 'this' outside of a non-static member function}}
ATTR(vector(linear(this)))  // expected-error {{invalid use of 'this' outside of a non-static member function}}
int f1();

class Class {
  enum { s = 2 };

  ATTR(vector(uniform(this)))                  // OK
  ATTR(vector(uniform(this, x)))               // OK
  ATTR(vector(linear(this)))                   // OK
  ATTR(vector(linear(x, this)))                // OK
  ATTR(vector(linear(this:x), uniform(x)))     // OK
  ATTR(vector(linear(this:s)))                 // OK
  ATTR(vector(linear(this:4)))                 // OK
  void f2(int x);

  ATTR(vector(uniform(this), uniform(this))) // expected-error {{'this' cannot be the subject of two elemental clauses}} expected-note {{here}}
  ATTR(vector(uniform(this), linear(this)))  // expected-error {{'this' cannot be the subject of two elemental clauses}} expected-note {{here}}
  ATTR(vector(linear(this), uniform(this)))  // expected-error {{'this' cannot be the subject of two elemental clauses}} expected-note {{here}}
  ATTR(vector(linear(this), linear(this)))   // expected-error {{'this' cannot be the subject of two elemental clauses}} expected-note {{here}}
  void f3();

  ATTR(vector(uniform(this))) // expected-error {{invalid use of 'this' outside of a non-static member function}}
  ATTR(vector(linear(this)))  // expected-error {{invalid use of 'this' outside of a non-static member function}}
  static void f4();

  // Note: "friend [[vector]] void foo();" is invalid.
  friend
  __attribute__((vector(linear(this))))  // expected-error {{invalid use of 'this' outside of a non-static member function}}
  __attribute__((vector(uniform(this)))) // expected-error {{invalid use of 'this' outside of a non-static member function}}
  void f5();
};

template <typename T>
class Class2 {
public:
  ATTR(vector(uniform(this)))    // OK
  ATTR(vector(linear(this)))     // OK
  void f6() {}
};

void test() {
  Class2<float> Obj;
  Obj.f6();
}

} // test_this

namespace test_evaluation_linear_step {

const int STEP = 2;
__attribute__((vector(linear(x:STEP))))
void f1(int x) { } // OK.

__attribute__((vector(linear(x:step), uniform(step))))
void f2(int x, int step) { } // OK.

template <typename T, T s>
__attribute__((vector(linear(x:s))))
void f3(int x) { }

template <unsigned N>
struct S {
  __attribute__((vector(linear(x:N))))
  void f4(int x) { }
};

void test() {
  f3<unsigned int, 4>(0); // OK.
  S<4>().f4(0); // OK.
}

} // test_evaluation_linear_step
