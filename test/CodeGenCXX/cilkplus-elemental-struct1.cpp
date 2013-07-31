// RUN: %clang_cc1 -triple x86_64-unknown-unknown -fcilkplus -emit-llvm -verify %s

struct X { int a, b, c, d; };

__attribute__((vector))
X foo() // expected-error {{the return type for this elemental function is not supported yet}}
{ return X(); }
