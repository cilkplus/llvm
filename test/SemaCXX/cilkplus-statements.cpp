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
