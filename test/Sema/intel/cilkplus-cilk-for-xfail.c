// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s
// REQUIRES: cilkplus

void capture(int n) {
  _Cilk_for (int i = 0; i < n; ++i) { // expected-note {{'_Cilk_for' loop control variable declared here}}
    i += 1; // expected-error {{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}}
  }

  _Cilk_for (int i = 0; i < n; ++i) { // expected-note {{'_Cilk_for' loop control variable declared here}}
    _Cilk_for (int j = 0; j < n; ++j) {
      i += 1; // expected-error {{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}}
    }
  }
}
