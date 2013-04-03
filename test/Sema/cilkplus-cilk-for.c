// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s

extern int array[];

int k;

void init() {
  int i;

  _Cilk_for (i = 0; i < 10; ++i); // OK
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

  _Cilk_for (int i = 1.2f; i < 10; ++i); // expected-warning {{implicit conversion from 'float' to 'int' changes value from 1.2 to 1}}

  _Cilk_for (char i = 0; i < 10; ++i); // OK

  _Cilk_for (short i = 0; i < 10; ++i); // OK

  _Cilk_for (long i = 0; i < 10; ++i); // OK

  _Cilk_for (int i = 0; i; ++i); // expected-error {{expected binary comparison operator in '_Cilk_for' loop condition}}

  _Cilk_for (int i = 0; i & 0x20; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; k >> i; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i == 10; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; (i << 1) < 10; ++i); // expected-error {{loop condition does not test control variable 'i' in '_Cilk_for'}} \
                                             // expected-note {{allowed forms are 'i' OP expr, and expr OP 'i'}}

  _Cilk_for (int i = 0; 10 > (i << 1); ++i); // expected-error {{loop condition does not test control variable 'i' in '_Cilk_for'}} \
                                             // expected-note {{allowed forms are 'i' OP expr, and expr OP 'i'}}

  _Cilk_for (int i = 0; j < 10; ++i); // expected-error {{loop condition does not test control variable 'i' in '_Cilk_for'}} \
                                      // expected-note {{allowed forms are 'i' OP expr, and expr OP 'i'}}

  _Cilk_for (int i = 0; // expected-error {{end - begin must have integral type in '_Cilk_for' - got 'float'}} \
                        // expected-note {{loop begin expression here}}
             i < 10.2f; // expected-note {{loop end expression here}}
             i++);

  _Cilk_for (int i = 100; // expected-error {{end - begin must have integral type in '_Cilk_for' - got 'float'}} \
                          // expected-note {{loop end expression here}}
      10.2f < i;          // expected-note {{loop begin expression here}}
      i--);
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
    for (int j = 0; j < n; ++j) {
      if (i > j + 1) break; // OK
    }
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    int j = 0;
    while (j < i + 5) {
      if (i > j + 1) break; // OK
    }
  }
}
