// RUN: %clang_cc1 -std=c++11 -fcilkplus -fsyntax-only -verify %s

struct C { // expected-note {{candidate constructor (the implicit copy constructor) not viable: no known conversion from 'int' to 'const C &' for 1st argument}} \
           // expected-note {{candidate constructor (the implicit move constructor) not viable: no known conversion from 'int' to 'C &&' for 1st argument}}
  explicit C(int v = 0);
  void operator++(int);
  void operator++();
  void operator+=(int);
  bool operator<(int);
  int operator-(int);
  friend int operator-(int, const C&) { return 0; }
};

template <typename T>
void init1() {
  _Cilk_for (T i = 0; i < 10; ++i); // expected-error {{comparison between pointer and integer ('void *' and 'int')}} \
                                    // expected-error {{no viable conversion from 'int' to 'C'}}
}

template <typename T>
void init2() {
  _Cilk_for (T i; i < 10; ++i); // expected-error {{_Cilk_for loop control variable must be initialized}}
}

template <typename T>
void init3(const T &a, const T &b) {
  _Cilk_for (T i = a ; i < b; ++i);  // expected-error {{loop control variable must have an integral, pointer, or class type in '_Cilk_for'}}
}

template<typename T>
struct MyIterator {
  MyIterator();
  MyIterator(T);
  MyIterator& operator++();
  MyIterator& operator+=(T);
  bool operator!=(MyIterator<T>);
  operator int();
};

template<typename T>
struct MyClass {
  typedef MyIterator<T> iterator;

  iterator begin();
  iterator end();
};

void init3() {
  MyClass<float> m;
  _Cilk_for(MyClass<float>::iterator i = m.begin(); i != m.end(); ++i); // OK
  _Cilk_for(decltype(m)::iterator i = m.begin(); i != m.end(); ++i);    // OK
}

void test_init() {
  init1<char>();   // OK
  init1<short>();  // OK
  init1<int>();    // OK
  init1<long>();   // OK
  init1<void *>(); // expected-note {{in instantiation of function template specialization 'init1<void *>' requested here}}
  init1<C>();      // expected-note {{in instantiation of function template specialization 'init1<C>' requested here}}

  init2<int>();    // expected-note {{in instantiation of function template specialization 'init2<int>' requested here}}
  init2<C>();      // OK

  int a = 0, b = 10;
  init3<int&>(a, b); // expected-note {{in instantiation of function template specialization 'init3<int &>' requested here}}
}

template <typename T>
void cond1(T t) {
  _Cilk_for (int i = 0; i < t; ++i); // expected-error {{comparison between pointer and integer ('int' and 'void *')}}
}

template <typename T>
void cond2() {
  _Cilk_for (int i = 0; i < sizeof(T); ++i);
  _Cilk_for (int i = 0; i < sizeof(sizeof(T)); ++i);
}

template <typename... T>
void cond3(T... val) {
  _Cilk_for (int i = 0; i < sizeof...(T); ++i);
  _Cilk_for (int i = 0; i < sizeof(sizeof...(T)); ++i);
}

void test_cond() {
  cond1<int>(0);          // OK
  cond1<void *>(nullptr); // expected-note {{in instantiation of function template specialization 'cond1<void *>' requested here}}
  cond2<int>();           // OK
  cond2<void *>();        // OK
  cond3(1, 2, 3, 4, 5);   // OK
  cond3<int, char*>(1, nullptr); // OK
}

template <typename T>
void inc1(T t) {
  _Cilk_for (int i = 0; i < 10; i += t); // expected-error {{arithmetic on a pointer to void}}
}

template <typename T>
void inc2() {
  _Cilk_for (int i = 0; i < 10; i += sizeof(T));
}

template <typename T>
void inc3() {
  _Cilk_for (int i = 0; i < 10; i += sizeof(sizeof(T)));
}

void test_inc() {
  inc1<int>(1);         // OK
  inc1<void*>(nullptr); // expected-note {{in instantiation of function template specialization 'inc1<void *>' requested here}}

  inc2<int>();          // OK
  inc2<void*>();        // OK
  inc3<int>();          // OK
  inc3<void*>();        // OK
}

template <typename T>
void foo(const T &t);

template <typename T>
void body(T *data) {
  _Cilk_for (int i = 0; i < 10; ++i) {
    foo(data[i]);
  }
}

void test_body(int *idata, float *fdata) {
  body(idata); // OK
  body(fdata); // OK
}

void bar(int); // expected-note {{candidate function not viable: cannot convert argument of incomplete type 'void *' to 'int'}}

template <typename T, typename A, typename B, typename C, typename D>
void for_errors(A a, B b, C c, D d) {
  _Cilk_for(T i = a; i < 10; i++); // expected-error {{cannot initialize a variable of type 'int' with an lvalue of type 'void *}}
  _Cilk_for(T i = 0; i < b; i++); // expected-error {{comparison between pointer and integer ('int' and 'void *')}}
  _Cilk_for(T i = 0; i < 10; i += c); // expected-error {{arithmetic on a pointer to void}}
  _Cilk_for(T i = 0; i < 10; i++) { bar(d); } // expected-error {{no matching function for call to 'bar'}}
  _Cilk_for(int i = 0; i < 10; i++); // OK
}

void test_for_errors() {
  for_errors<int, void*, int, int, int>(nullptr, 10, 1, 0); // expected-note {{in instantiation of function template specialization 'for_errors<int, void *, int, int, int>' requested here}}
  for_errors<int, int, void*, int, int>(0, nullptr, 1, 0); // expected-note {{in instantiation of function template specialization 'for_errors<int, int, void *, int, int>' requested here}}
  for_errors<int, int, int, void*, int>(0, 10, nullptr, 0); // expected-note {{in instantiation of function template specialization 'for_errors<int, int, int, void *, int>' requested here}}
  for_errors<int, int, int, int, void*>(0, 10, 1, nullptr); // expected-note {{in instantiation of function template specialization 'for_errors<int, int, int, int, void *>' requested here}}
}

namespace NS1 {
  struct Int {
    Int();
    Int(const Int&);
  };
  void operator++(const Int&, int);
  bool operator<(const Int&, int);
  int operator-(int, const Int&);
  void operator+=(const Int&, int);
}

namespace NS2 {
  template <typename T>
  void foo() {
    _Cilk_for (T i; i < 10; i++);
  }

  void test_foo() {
    // ADL works.
    foo<NS1::Int>();
  }
}

template <typename T>
void grainsize_test1(T gs) {
  #pragma cilk grainsize = gs
  _Cilk_for(int i = 0; i < 100; ++i);
}

template <int gs>
void grainsize_test2() {
  #pragma cilk grainsize = gs
  _Cilk_for(int i = 0; i < 100; ++i);
}

template <typename T>
void grainsize_test3(T) {
  #pragma cilk grainsize = sizeof(sizeof(T))
  _Cilk_for(int i = 0; i < 100; ++i);
}

void test_grainsize() {
  C c;
  grainsize_test1(c); // expected-error@178 {{no viable conversion from 'C' to 'int'}} \
                      // expected-note@178 {{grainsize must evaluate to a type convertible to 'int'}} \
                      // expected-note {{in instantiation of function template specialization 'grainsize_test1<C>' requested here}}

  grainsize_test2<100>(); // OK

  grainsize_test2<-100>(); // expected-error@184 {{the behavior of Cilk for is unspecified for a negative grainsize}} \
                           // expected-note {{in instantiation of function template specialization 'grainsize_test2<-100>' requested here}}

  grainsize_test3(c); // OK
}

template <typename T>
void wraparound1() {
  _Cilk_for(T i = 0; i != 10; i--);
}

template <typename T>
void wraparound2() {
  _Cilk_for(T i = 100; i != 10; i++);
}

template <typename T>
void wraparound3() {
  _Cilk_for(T i = 0; i != 10; i += 3);
}

template <typename T>
void wraparound4(T x) {
  _Cilk_for(int i = x; i != 10; i--);
}

template <typename T>
void wraparound5() {
  _Cilk_for (int i = 0; i < sizeof(T); ++i);
  _Cilk_for (int i = sizeof(T); i < 10; ++i);
}

template <int x>
void wraparound6() {
  _Cilk_for(int i = x; i != 10; ++i);
}

template <int x>
void wraparound7() {
  _Cilk_for(int i = 0; i != 10; i -= x);
}

void test_wraparound() {
  wraparound1<unsigned>(); // expected-warning@210 {{negative stride causes unsigned wraparound}} \
                           // expected-note {{in instantiation of function template specialization 'wraparound1<unsigned int>' requested here}} \
                           // expected-note@210 {{Wraparounds cause undefined behavior in Cilk for}}

  wraparound2<unsigned>(); // expected-warning@215 {{positive stride causes unsigned wraparound}} \
                           // expected-note {{in instantiation of function template specialization 'wraparound2<unsigned int>' requested here}} \
                           // expected-note@215 {{Wraparounds cause undefined behavior in Cilk for}}

  wraparound3<unsigned>(); // expected-warning@220 {{positive stride causes unsigned wraparound}} \
                           // expected-note {{in instantiation of function template specialization 'wraparound3<unsigned int>' requested here}} \
                           // expected-note@220 {{Wraparounds cause undefined behavior in Cilk for}}

  wraparound1<int>(); // expected-warning@210 {{negative stride causes signed wraparound}} \
                      // expected-note {{in instantiation of function template specialization 'wraparound1<int>' requested here}} \
                      // expected-note@210 {{Wraparounds cause undefined behavior in Cilk for}}

  wraparound2<int>(); // expected-warning@215 {{positive stride causes signed wraparound}} \
                      // expected-note {{in instantiation of function template specialization 'wraparound2<int>' requested here}} \
                      // expected-note@215 {{Wraparounds cause undefined behavior in Cilk for}}

  wraparound3<int>(); // expected-warning@220 {{positive stride causes signed wraparound}} \
                      // expected-note {{in instantiation of function template specialization 'wraparound3<int>' requested here}} \
                      // expected-note@220 {{Wraparounds cause undefined behavior in Cilk for}}

  wraparound4(0); // OK

  wraparound5<float>(); // OK

  wraparound6<0>(); // OK

  wraparound6<100>(); // expected-warning@236 {{positive stride causes signed wraparound}} \
                     // expected-note {{in instantiation of function template specialization 'wraparound6<100>' requested here}} \
                     // expected-note@236 {{Wraparounds cause undefined behavior in Cilk for}}

  wraparound7<-1>(); // OK
}

