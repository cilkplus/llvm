// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify -Wall %s

__attribute__((vector(uniform(this)))) // OK
__attribute__((vector(linear(this))))  // OK
__attribute__((vector(uniform(goto)))) // expected-error {{expected identifier}}
__attribute__((vector(linear(goto))))  // expected-error {{expected identifier}}
__attribute__((vector(mask;, vectorlength(4))))  // expected-error {{expected ')' or ','}}
__attribute__((vector(mask;)))  // expected-error {{expected ')' or ','}}
__attribute__((vector(a)))  // expected-warning {{unknown attribute 'a' - ignored}}
__attribute__((vector(a, mask)))  // expected-warning {{unknown attribute 'a' - ignored}}
void f(int this);
