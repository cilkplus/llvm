// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify -Wall %s

__attribute__((vector(uniform(this)))) // OK
__attribute__((vector(linear(this))))  // OK
__attribute__((vector(uniform(goto)))) // expected-error {{expected identifier}}
__attribute__((vector(linear(goto))))  // expected-error {{expected identifier}}
void f(int this);
