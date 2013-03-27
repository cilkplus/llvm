// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s

extern int array[];

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
