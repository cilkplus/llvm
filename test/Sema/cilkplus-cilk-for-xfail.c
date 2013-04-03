// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s
// XFAIL: *

int k;

extern int m;

void init() {
  int i, j;

  _Cilk_for (int i = 0, j = 0; i < 10; ++i); // expected-error{{cannot declare more than one loop control variables in '_Cilk_for'}}

  _Cilk_for (i = 0, j = 0; i < 10; ++i); // expected-error{{cannot declare more than one loop control variables in '_Cilk_for'}}

  _Cilk_for (auto int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot have storage class 'auto' in '_Cilk_for'}}

  _Cilk_for (static int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot have storage class 'static' in '_Cilk_for'}}

  _Cilk_for (register int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot have storage class 'register' in '_Cilk_for'}}

  _Cilk_for (k = 0; k < 10; ++k); // expected-error {{loop control variable cannot have storage class 'static' in '_Cilk_for'}}

  _Cilk_for (m = 0; m < 10; ++m); // expected-error {{loop control variable cannot have storage class 'extern' in '_Cilk_for'}}

  _Cilk_for (volatile int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot be 'volatile' in '_Cilk_for'}}


  _Cilk_for (const int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot be 'const' in '_Cilk_for'}} \
                                            // expected-error {{read-only variable is not assignable}}

  float f;
  _Cilk_for (f = 0.0f; f < 10.0f; ++f); // expected-error {{loop control variable shall have integral, pointer, or class type in '_Cilk_for'}}

  enum E { a = 0, b };
  _Cilk_for (enum E i = a; i < b; i += 1); // expected-error {{loop control variable shall have integral, pointer, or class type in '_Cilk_for'}}

  union { int i; void *p; } u;
  _Cilk_for (u.i = 0; u.i < 10; u.i += 1); // expected-error {{loop control variable shall have integral, pointer, or class type in '_Cilk_for'}}
}

extern int next();

void increment() {
  _Cilk_for (int i = 0; i < 10; i--); // expected-error {{loop increment and condition are inconsistent in '_Cilk_for'}}

  _Cilk_for (int i = 10; i >= 0; i++); // expected-error {{loop increment and condition are inconsistent in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i *= 2); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i <<= 1); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}

  int j = 0;
  _Cilk_for (int i = 0; i < 10; j++); // expected-error {{loop increment does not modify loop variable in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i += 1.2f); // expected-error {{right-hand side of '+=' must have integeral or enum type in '_Cilk_for' increment}}

  _Cilk_for (int i = 10; i > 0; i -= 1.2f); // expected-error {{right-hand side of '-=' must have integeral or enum type in '_Cilk_for' increment}}
}

int array[10] = { 0, 1, 2 , 3, 4, 5, 6, 7, 8, 9 };

void jump(int n) {
  _Cilk_for (int i = 0; i < n; ++i) {
    if (i > n / 2) break; // expected-error {{cannot break from a '_Cilk_for' loop}}
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    _Cilk_for (int j = 0; j < n; ++j) {
      if (i > j + 1) break; // expected-error {{cannot break from a '_Cilk_for' loop}}
    }
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    return; // expected-error {{cannot return from within a '_Cilk_for' loop}}
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      return; // expected-error {{cannot return from within a '_Cilk_for' loop}}
    }
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    if (i == 5) goto exit; // expected-error {{cannot use 'goto' to leave a '_Cilk_for' loop}}
  }

exit:

  _Cilk_for (int i = 0; i < n; ++i) {
    if (i == 5) goto inner; // OK
    (void)i;
  inner:
    (void)i;
  }

  if (true) goto inner; // expected-error {{cannot use 'goto' to enter a '_Cilk_for' loop}}

  _Cilk_for (int i = 0; i < n; ++i) {
    if (i == 5) continue; // OK
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    switch (i) {
    case 0:
      continue; // OK
    case 1:
      goto l_out; // expected-error {{cannot use 'goto' to leave a '_Cilk_for' loop}}
    default:
      break; // OK
    }
  }

  // indirect goto
l_out:
  void *addr;

  _Cilk_for (int i = 0; i < n; ++i) {
    addr = &&l_out;
    goto *addr;
  }

  _Cilk_for (int i = 0; i < n; ++i) {
l_in:
    addr = &&l_in;
    goto *addr;
  }
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
