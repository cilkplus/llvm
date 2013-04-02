// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s
// XFAIL: *

void increment() {
  _Cilk_for (int i = 0; i < 10; i--); // expected-error {{loop increment and condition are inconsistent in '_Cilk_for'}}

  _Cilk_for (int i = 10; i >= 0; i++); // expected-error {{loop increment and condition are inconsistent in '_Cilk_for'}}

  int j = 0;
  _Cilk_for (int i = 0; i < 10; j++); // expected-error {{loop increment does not modify loop variable in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; (++i)); // OK
}

int gs();

void grainsize() {
  #pragma cilk grainsize = 65
  _Cilk_for (int i = 0; i < 100; ++i); // OK

  #pragma cilk grainsize = 'a'
  _Cilk_for (int i = 0; i < 100; ++i); // OK

  #pragma cilk grainsize = 65.3f
  _Cilk_for (int i = 0; i < 100; ++i); // OK

  #pragma cilk grainsize = gs()
  _Cilk_for (int i = 0; i < 100; ++i); // OK

  #pragma cilk grainsize = 32
  /* expected-warning {{cilk grainsize pragma ignored}} */#pragma cilk grainsize = 64
  _Cilk_for (int i = 0; i < 100; ++i);

  /* cilk grainsize expression type must be convertible to signed long */ #pragma cilk grainsize = "65"
  _Cilk_for (int i = 0; i < 100; ++i);
}

void capture() {
  _Cilk_for (int i = 0; i < n; ++i) {
    i += 1; // expected-error {{cannot modify control variable ‘i’ in the body of '_Cilk_for'}}
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    _Cilk_for (int j = 0; j < n; ++j) {
      i += 1; // expected-error {{cannot modify control variable ‘i’ in the body of '_Cilk_for'}}
    }
  }
}
