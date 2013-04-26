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
