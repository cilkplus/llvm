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

  // check the errors from .c file, but type-dependent
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
