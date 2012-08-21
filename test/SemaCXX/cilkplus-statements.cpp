// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify -std=c++11 %s

template<typename T>
void foo();

template<typename T>
struct bar {
  T mem_func(T);
  void nested() {
    _Cilk_spawn mem_func(T(5));
  }
};

struct baz {
  int operator()(int);
  int operator+(int);
};

int test_scope_func();

struct test_scope {
  int x;
  test_scope() : x(_Cilk_spawn test_scope_func()) { } // expected-error {{_Cilk_spawn not allowed in this scope}}
  void mem(int y = _Cilk_spawn test_scope_func()) { } // expected-error {{_Cilk_spawn not allowed in this scope}}
};

void test() {
  _Cilk_spawn foo<int>();

  bar<int> bar;
  _Cilk_spawn bar.mem_func(5);
  _Cilk_spawn bar.nested();

  baz baz;
  _Cilk_spawn baz(5);

  auto lambda = [](int x)->int {
    return x;
  };
  _Cilk_spawn lambda(5);

  _Cilk_spawn [](int x)->int { return x; }(5);

  _Cilk_spawn baz.operator+(1);
  _Cilk_spawn baz + 1; // expected-error {{the argument to _Cilk_spawn must be a function call}}
}

struct t1 {
};
t1 make_t1();

struct t2 {
  t2& operator=(const t2&);
  const t2& operator=(const t2&) const;
};
t2 make_t2();
t2& make_t2ref();

struct t3 {
  t3(const t3&);
};
t3 make_t3();
t3& make_t3ref();

struct t4 {
  ~t4();
};
t4 make_t4();

class t5 {
  ~t5(); // expected-note {{implicitly declared private here}} \
            expected-note {{implicitly declared private here}} \
            expected-note {{implicitly declared private here}} \
            expected-note {{implicitly declared private here}}
};
t5 make_t5();

void test_objects() {
  t1 a = _Cilk_spawn make_t1();
  a = _Cilk_spawn make_t1();

  t2 b = _Cilk_spawn make_t2();
  b = _Cilk_spawn make_t2();

  t2 c = _Cilk_spawn make_t2ref();
  t2& cr = _Cilk_spawn make_t2ref();
  c = _Cilk_spawn make_t2ref();

  t3 d = _Cilk_spawn make_t3();
  d = _Cilk_spawn make_t3();

  t3 e = _Cilk_spawn make_t3ref();
  t3& er = _Cilk_spawn make_t3ref();
  e = _Cilk_spawn make_t3ref();

  t4 f = _Cilk_spawn make_t4();
  f = _Cilk_spawn make_t4();

  t5 g = _Cilk_spawn make_t5(); // expected-error {{temporary of type 't5' has private destructor}} \
                                   expected-error {{variable of type 't5' has private destructor}}
  g = _Cilk_spawn make_t5();    // expected-error {{temporary of type 't5' has private destructor}}
}

template<typename T>
struct test_templates {
  T foo();

  void test() {
    _Cilk_spawn foo(); // expected-error {{temporary of type 't5' has private destructor}}
    _Cilk_sync;
  }
};

template<typename T1, typename T2>
struct test_templates2 {
  T1 foo();

  void test() {
    _Cilk_spawn foo();
    T1 b = _Cilk_spawn foo();
    b = _Cilk_spawn foo();
    if (1) _Cilk_spawn foo();
    if (1) {} else _Cilk_spawn foo();
    while (1) _Cilk_spawn foo();
    for (;;) _Cilk_spawn foo();
    switch (1) {
    case 0: _Cilk_spawn foo(); break;
    default: _Cilk_spawn foo(); break;
    }
    _Cilk_sync;

    _Cilk_spawn 5;         // expected-error {{the argument to _Cilk_spawn must be a function call}}
    _Cilk_spawn foo() + 5; // expected-error {{_Cilk_spawn is not at statement level}}
    _Cilk_spawn _Cilk_spawn foo(); // expected-error {{consecutive _Cilk_spawn tokens not allowed}}

    T2 c = _Cilk_spawn foo(); // expected-error {{no viable conversion from 'int' to 'tt'}}
    T2 d;
    d = _Cilk_spawn foo();    // expected-error {{no viable overloaded '='}}
    _Cilk_sync;
  }
};

struct tt { // expected-note {{candidate constructor (the implicit copy constructor) not viable: no known conversion from 'int' to 'const tt &' for 1st argument}} \
expected-note {{candidate constructor (the implicit move constructor) not viable: no known conversion from 'int' to 'tt &&' for 1st argument}} \
expected-note {{candidate function (the implicit copy assignment operator) not viable: no known conversion from 'int' to 'const tt' for 1st argument}} \
expected-note {{candidate function (the implicit move assignment operator) not viable: no known conversion from 'int' to 'tt' for 1st argument}}
};

void test_templates_helper() {
  test_templates<int> one;
  one.test();

  test_templates<t5> two;
  two.test(); // expected-note {{in instantiation of member function 'test_templates<t5>::test' requested here}}

  test_templates2<int, tt> three;
  three.test(); // expected-note {{in instantiation of member function 'test_templates2<int, tt>::test' requested here}}
}

// Not instantiated
template<typename T1, typename T2>
struct test_templates3 {
  T1 foo();

  void test() {
    _Cilk_spawn foo();
    T1 b = _Cilk_spawn foo();
    b = _Cilk_spawn foo();
    if (1) _Cilk_spawn foo();
    if (1) {} else _Cilk_spawn foo();
    while (1) _Cilk_spawn foo();
    for (;;) _Cilk_spawn foo();
    switch (1) {
    case 0: _Cilk_spawn foo(); break;
    default: _Cilk_spawn foo(); break;
    }
    _Cilk_sync;

    _Cilk_spawn 5;         // expected-error {{the argument to _Cilk_spawn must be a function call}}
    _Cilk_spawn foo() + 5; // expected-error {{_Cilk_spawn is not at statement level}}
    _Cilk_spawn _Cilk_spawn foo(); // expected-error {{consecutive _Cilk_spawn tokens not allowed}}

    // No errors, since T2 may declare constructors/assignment from type T1
    T2 c = _Cilk_spawn foo();
    T2 d;
    d = _Cilk_spawn foo();
    _Cilk_sync;
  }
};
