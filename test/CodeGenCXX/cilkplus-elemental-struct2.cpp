// RUN: %clang_cc1 -triple x86_64-unknown-unknown -fcilkplus -emit-llvm -verify %s

struct X { int a, b, c, d; };

__attribute__((vector))
void foo(X x){ } // expected-error {{the parameter type for this elemental function is not supported yet}}
