// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s
// REQUIRES: cilkplus

int foo();
int bar(int);

int warn_func1() __attribute__((pure));
int warn_func2() __attribute__((const));
int warn_func3() __attribute__((warn_unused_result));

struct baz { };

struct baz bazmaker();

void test() {
  _Cilk_spawn foo();

  int x, y = _Cilk_spawn foo(), z; // OK
  int x1 = _Cilk_spawn foo(), y1, z1 = _Cilk_spawn foo(); // OK

  int y2 = bar(_Cilk_spawn foo()), y3; // expected-error {{_Cilk_spawn is not at statement level}}
  int y4 = _Cilk_spawn bar(_Cilk_spawn foo()), y5; // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}

  int b = _Cilk_spawn foo();
  b = _Cilk_spawn foo();
  if (1) _Cilk_spawn foo();
  if (1) {} else _Cilk_spawn foo();
  while (1) _Cilk_spawn foo();
  for (;;) _Cilk_spawn foo();
  switch (1) {
  case 0: _Cilk_spawn foo(); break;
  default: _Cilk_spawn foo(); break;
  }

  { { { _Cilk_spawn foo(); } } }
  { { { _Cilk_spawn 5; } } } // expected-error {{the argument to _Cilk_spawn must be a function call}}

  _Cilk_spawn foo() + 5;           // expected-error {{_Cilk_spawn is not at statement level}}
  b = _Cilk_spawn foo() + 5;       // expected-error {{_Cilk_spawn is not at statement level}}
  b = 5 + _Cilk_spawn foo();       // expected-error {{_Cilk_spawn is not at statement level}}
  b = (_Cilk_spawn foo());
  b = bar(_Cilk_spawn foo());      // expected-error {{_Cilk_spawn is not at statement level}}
  int b1 = _Cilk_spawn foo() + 5;  // expected-error {{_Cilk_spawn is not at statement level}}
  int b2 = 5 + _Cilk_spawn foo();  // expected-error {{_Cilk_spawn is not at statement level}}
  int b3 = (_Cilk_spawn foo());
  int b4 = bar(_Cilk_spawn foo()); // expected-error {{_Cilk_spawn is not at statement level}}

  if (1) _Cilk_spawn bar(_Cilk_spawn foo()); // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}
  if (1) {} else _Cilk_spawn bar(_Cilk_spawn foo()); // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}
  while (1) _Cilk_spawn bar(_Cilk_spawn foo()); // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}
  for (;;) _Cilk_spawn bar(_Cilk_spawn foo()); // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}
  switch (1) {
  case 0: _Cilk_spawn bar(_Cilk_spawn foo()); break; // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}
  default: _Cilk_spawn bar(_Cilk_spawn foo()); break; // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}} 
  }
  switch (_Cilk_spawn foo()) { // expected-error {{_Cilk_spawn is not at statement level}}
  default: ;
  }
  _Cilk_spawn bar(_Cilk_spawn foo());         // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}
  b = _Cilk_spawn bar(_Cilk_spawn foo());     // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}
  int c = _Cilk_spawn bar(_Cilk_spawn foo()); // expected-error {{multiple spawn calls within a full expression}} expected-note {{another spawn here}}


  _Cilk_spawn _Cilk_spawn foo();   // expected-error {{consecutive _Cilk_spawn tokens not allowed}}

  _Cilk_spawn ( foo() + 5 );  // expected-error {{the argument to _Cilk_spawn must be a function call}}
  b = _Cilk_spawn (foo());
  _Cilk_spawn (_Cilk_spawn foo()); // expected-error {{consecutive _Cilk_spawn tokens not allowed}}
  _Cilk_spawn 5;              // expected-error {{the argument to _Cilk_spawn must be a function call}}
  _Cilk_spawn (5);            // expected-error {{the argument to _Cilk_spawn must be a function call}}
  _Cilk_spawn foo;            // expected-error {{the argument to _Cilk_spawn must be a function call}}
  _Cilk_spawn (foo);          // expected-error {{the argument to _Cilk_spawn must be a function call}}

  _Cilk_spawn warn_func1(); // expected-warning {{ignoring return value of function declared with pure attribute}}
  _Cilk_spawn warn_func2(); // expected-warning {{ignoring return value of function declared with const attribute}}
  _Cilk_spawn warn_func3(); // expected-warning {{ignoring return value of function declared with warn_unused_result attribute}}

  struct baz d1 = _Cilk_spawn foo(); // expected-error {{initializing 'struct baz' with an expression of incompatible type 'int'}}
  struct baz d2;
  d2 = _Cilk_spawn foo(); // expected-error {{assigning to 'struct baz' from incompatible type 'int'}}
  struct baz e1 = _Cilk_spawn bazmaker();
  struct baz e2;
  e2 = _Cilk_spawn bazmaker();
  int f1 = _Cilk_spawn bazmaker(); // expected-error {{initializing 'int' with an expression of incompatible type 'struct baz'}}
  int f2;
  f2 = _Cilk_spawn bazmaker(); // expected-error {{assigning to 'int' from incompatible type 'struct baz'}}
  float g1 = _Cilk_spawn foo();
  float g2;
  g2 = _Cilk_spawn foo();
}

int test2 = _Cilk_spawn foo(); // expected-error {{_Cilk_spawn not allowed in this scope}}

void test3() {
  struct X { int a; int b; };
  _Cilk_spawn __builtin_offsetof(struct X, a); // expected-error {{the argument to _Cilk_spawn must be a function call}}
  _Cilk_spawn sizeof(int); // expected-error {{the argument to _Cilk_spawn must be a function call}}

  int x = 0, y = 1;
  _Cilk_spawn __sync_fetch_and_add(&x, y);
}

void test4() {
  auto int x = _Cilk_spawn foo(); // OK
  register int y = _Cilk_spawn foo(); // OK
  extern int z = _Cilk_spawn foo(); // expected-error{{'extern' variable cannot have an initializer}}
}

void test5() {
  int b;
  b = ({int c; c = _Cilk_spawn foo();c;});// expected-error {{_Cilk_spawn disabled in statement expression}}
  b = ({int c; c = _Cilk_spawn foo();});  // expected-error {{_Cilk_spawn disabled in statement expression}}
  b = ({b = _Cilk_spawn foo();});         // expected-error {{_Cilk_spawn disabled in statement expression}}
  b = ({b = _Cilk_spawn foo();xxx;});     // expected-error {{use of undeclared identifier 'xxx'}} \
                                          // expected-error {{_Cilk_spawn disabled in statement expression}}
}
