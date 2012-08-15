// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify -std=c++11 %s

struct bar {
  int mem_func(int);
};

struct baz {
  int operator()(int);
};

void test() {
  bar bar;
  _Cilk_spawn bar.mem_func(5);

  baz baz;
  _Cilk_spawn baz(5);

  auto lambda = [](int x)->int {
    return x;
  };
  _Cilk_spawn lambda(5);

  _Cilk_spawn [](int x)->int { return x; }(5);
}
