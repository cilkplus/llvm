// RUN: %clang_cc1 -fsyntax-only -verify %s

void keyword() {
  _Cilk_spawn; // expected-error {{use of undeclared identifier '_Cilk_spawn'}}
  _Cilk_sync; // expected-error {{use of undeclared identifier '_Cilk_sync'}}
  _Cilk_for; // expected-error {{use of undeclared identifier '_Cilk_for'}}
}

// CilkPlus vector attribute cannot be used without -fcilkplus
__attribute__((vector(vectorlength(4)))) int vec1(int a) { return a; } // expected-error {{Cilk Plus support disabled}}
__attribute__((vector)) int vec2(int a) { return a; } // expected-error {{Cilk Plus support disabled}}
