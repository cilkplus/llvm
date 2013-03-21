// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s
// XFAIL: *

int k;

extern int m;

void init() {
  int i, j;

  _Cilk_for (i = 0; i < 10; ++i); // OK

  _Cilk_for (; i < 10; ++i); // expected-error {{missing control variable declaration and initialization in '_Cilk_for'}}

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

void condition() {
  int j = 1;

  _Cilk_for (int i = 0; i <= 10; ++i); // OK

  _Cilk_for (int i = 0; i != 10; ++i); // OK

  _Cilk_for (int i = 10; i > 0; --i); // OK

  _Cilk_for (int i = 10; i >= 0; --i); // OK

  _Cilk_for (int i = 10; i >= (j << 2); --i); // OK

  _Cilk_for (int i = 0; 10 >= i; ++i); // OK

  _Cilk_for (int i = 0; 10 != i; ++i); // OK

  _Cilk_for (int i = 10; 0 < i; --i); // OK

  _Cilk_for (int i = 10; 0 <= i; --i); // OK

  _Cilk_for (int i = 10; (j << 2) <= i; --i); // OK

  _Cilk_for (int i = 1.2f; i < 10; ++i); // OK

  _Cilk_for (char i = 0; i < 10; ++i); // OK

  _Cilk_for (short i = 0; i < 10; ++i); // OK

  _Cilk_for (long i = 0; i < 10; ++i); // OK

  _Cilk_for (int i = 0; ; ++i); // expected-error {{missing loop conditional expression in '_Cilk_for'}}

  _Cilk_for (int i = 0; i & 0x20; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; k >> i; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i == 10; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; int k = i - 10; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; (i << 1) < 10; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; 10 > (i << 1); ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; j < 10; ++i); // expected-error {{loop condition does not test loop control variable in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10.2f; i++); // expected-error {{operator-(end, begin) must return an integer type in '_Cilk_for'}} \
                                         // expected-note {{loop begin expression is }} \
                                         // expected-note {{loop end expression is }}
}

extern int next();

void increment() {
  _Cilk_for (int i = 0; i < 10; ++i); // OK

  _Cilk_for (int i = 0; i < 10; i++); // OK

  _Cilk_for (int i = 10; i > 0; --i); // OK

  _Cilk_for (int i = 10; i > 0; i--); // OK

  _Cilk_for (int i = 10; i > 0; i -= 2); // OK

  _Cilk_for (int i = 0; i < 10; i += 2); // OK

  _Cilk_for (int i = 0; i < 10; i += next()); // OK

  enum E { a = 0, b = 5 };
  _Cilk_for (int i = 0; i < 10; i += b); // OK

  _Cilk_for (int i = 0; i < 10; ); // expected-error {{missing loop increment expression in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i--); // expected-error {{loop increment and condition are inconsistent in '_Cilk_for'}}

  _Cilk_for (int i = 10; i >= 0; i++); // expected-error {{loop increment and condition are inconsistent in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i *= 2); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i <<= 1); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}

  int j = 0;
  _Cilk_for (int i = 0; i < 10; j++); // expected-error {{loop increment does not modify loop variable in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i += 1.2f); // expected-error {{right-hand side of '+=' must have integeral or enum type in '_Cilk_for' increment}}

  _Cilk_for (int i = 10; i > 0; i -= 1.2f); // expected-error {{right-hand side of '-=' must have integeral or enum type in '_Cilk_for' increment}}
}

void other_types(int *p, int *q) {
  int *t;

  _Cilk_for (t = p; t < q; ++t); // OK

  _Cilk_for (t = p; t > q; --t); // OK

  _Cilk_for (t = p; t < q; t += 2); // OK

  _Cilk_for (t = p; t > q; t -= 2); // OK

  short i, n = 10;

  _Cilk_for (i = 0; i < n; ++i); // OK

  _Cilk_for (i = n; i > 0; --i); // OK

  _Cilk_for (i = 0; i < n; i += 2); // OK

  _Cilk_for (i = n; i > 0; i -= 2); // OK
}

int array[10] = { 0, 1, 2 , 3, 4, 5, 6, 7, 8, 9 };

void control() {
  _Cilk_for (int i = 0; i < 10; ++i) { // OK
    while (array[i] != 0);
  }

  _Cilk_for (int i = 0; i < 10; ++i) { // OK
    do { ; } while (array[i] != 5);
  }

  _Cilk_for (int i = 0; i < 10; ++i) { // OK
    for (int j = 0; j < 5; ++j);
  }

  _Cilk_for (int i = 0; i < 10; ++i) { // OK
    if (i % 2 == 0) { } else { }
  }
}

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
    for (int j = 0; j < n; ++j) {
      if (i > j + 1) break; // OK
    }
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    int j = 0
    while (j < i + 5) {
      if (i > j + 1) break; // OK
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
