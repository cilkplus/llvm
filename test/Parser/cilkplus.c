// RUN: %clang_cc1 -fsyntax-only -verify %s

void keyword() {
  _Cilk_spawn; // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_sync; // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_for; // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_spawn keyword(); // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
  _Cilk_for (i = 0; i < 10; ++i); // expected-error {{Cilk Plus support disabled - compile with -fcilkplus}}
}

// CilkPlus vector attribute cannot be used without -fcilkplus
__attribute__((vector(vectorlength(4)))) int vec1(int a) { return a; } // expected-error {{Cilk Plus support disabled}}
__attribute__((vector)) int vec2(int a) { return a; } // expected-error {{Cilk Plus support disabled}}
__declspec(vector) int vec3(int a) { return a; } // expected-error {{Cilk Plus support disabled}}
