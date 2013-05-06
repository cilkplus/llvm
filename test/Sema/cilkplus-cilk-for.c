// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s

extern int array[];
int k;
extern int m; // expected-note {{'m' declared here}}
extern int* get_intptr();

struct struct_t {
  int x;
};

int k;

void init() {
  int i, j;

  _Cilk_for (i = 0; i < 10; ++i); // OK

  _Cilk_for (((i) = 0); i < 10; ++i); // OK

  _Cilk_for (int i = 0, j = 0; i < 10; ++i); // expected-error{{cannot declare more than one loop control variable in '_Cilk_for'}}

  _Cilk_for (i = 0, j = 0; i < 10; ++i); // expected-error{{cannot initialize more than one loop control variable in '_Cilk_for'}}

  _Cilk_for (auto int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot have storage class 'auto' in '_Cilk_for'}}

  _Cilk_for (static int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot have storage class 'static' in '_Cilk_for'}}

  _Cilk_for (register int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot have storage class 'register' in '_Cilk_for'}}

  _Cilk_for (k = 0; k < 10; ++k); // expected-error {{non-local loop control variable in '_Cilk_for'}}

  _Cilk_for (m = 0; m < 10; ++m); // expected-error {{loop control variable cannot have storage class 'extern' in '_Cilk_for'}}

  _Cilk_for (volatile int i = 0; i < 10; ++i); // expected-error {{loop control variable cannot be 'volatile' in '_Cilk_for'}}

  _Cilk_for (const int i = 0; i < 10; i++); // expected-error {{read-only variable is not assignable}}

  _Cilk_for (*&i = 0; *&i < 10; (*&i)++); // expected-error {{expected a variable for control variable in '_Cilk_for'}}
  _Cilk_for (*get_intptr() = 0; *&i < 10; (*&i)++); // expected-error {{expected a variable for control variable in '_Cilk_for'}}

  float f;                              // expected-note {{'f' declared here}}
  _Cilk_for (f = 0.0f; f < 10.0f; ++f); // expected-error {{loop control variable must have an integral, pointer, or class type in '_Cilk_for'}}

  enum E { a = 0, b };
  _Cilk_for (enum E i = a; i < b; i += 1); // OK

  union { int i; void *p; } u, v;          // expected-note {{'u' declared here}}
  _Cilk_for (u.i = 0; u.i < 10; u.i += 1); // expected-error {{expected a variable for control variable in '_Cilk_for'}}
  _Cilk_for (u = v; u.i < 10; u.i += 1);   // expected-error {{loop control variable must have an integral, pointer, or class type in '_Cilk_for'}}

  struct struct_t w;
  _Cilk_for (w.x = 0; w.x < 10; w.x++); // expected-error {{expected a variable for control variable in '_Cilk_for'}}

  _Cilk_for (extern int foo(); i < 10; i++); // expected-error {{expected control variable declaration in initializer in '_Cilk_for'}}
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

  _Cilk_for (int i = 0; i < 10; ); // expected-error {{missing loop increment expression in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; !i); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}
  _Cilk_for (int i = 0; i < 10; i *= 2); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}
  _Cilk_for (int i = 0; i < 10; i <<= 1); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; i += 1.2f); // expected-error {{right-hand side of '+=' must have integral or enum type in '_Cilk_for' increment}}
  _Cilk_for (int i = 10; i > 0; i -= 1.2f); // expected-error {{right-hand side of '-=' must have integral or enum type in '_Cilk_for' increment}}

  _Cilk_for (int i = 0; i < 10; (0, ++i)); // expected-warning {{expression result unused}} expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}

  _Cilk_for (int i = 0; i < 10; (i++)); // OK
  _Cilk_for (int i = 0; i < 10; ((i++))); // OK
  _Cilk_for (int i = 0; i < 10; (i *= 2)); // expected-error {{loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'}}
  _Cilk_for (int i = 0; i < 10; (i += 1.2f)); // expected-error {{right-hand side of '+=' must have integral or enum type in '_Cilk_for' increment}}

  // Tests for inconsistency between loop condition and increment
  _Cilk_for (int i = 0; i < 10; i--); // expected-error {{loop increment is inconsistent with condition in '_Cilk_for': expected positive stride}} \
                                      // expected-note  {{constant stride is -1}}
  _Cilk_for (int i = 0; i < 10; i -= 1); // expected-error {{loop increment is inconsistent with condition in '_Cilk_for': expected positive stride}} \
                                         // expected-note  {{constant stride is -1}}
  _Cilk_for (int i = 10; i >= 0; i++); // expected-error {{loop increment is inconsistent with condition in '_Cilk_for': expected negative stride}} \
                                       // expected-note  {{constant stride is 1}}
  _Cilk_for (int i = 10; i >= 0; i += 0); // expected-error {{loop increment must be non-zero in '_Cilk_for'}}

  int j = -1;
  _Cilk_for (int i = 0; i < 10; j++); // expected-error {{loop increment does not modify control variable 'i' in '_Cilk_for'}}
  _Cilk_for (int i = 0; i < 10; (++i)); // OK
  _Cilk_for (int i = 0; i < 10; i -= j); // OK
  _Cilk_for (int i = 0; i < 10; i -= -1); // OK
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
    if (i == 5) continue; // OK
  }

  _Cilk_for (int i = 0; i < n; ++i) {
    if (i == 5) goto exit; // expected-error {{use of undeclared label 'exit'}}
  }

exit:

  _Cilk_for (int i = 0; i < n; ++i) {
    if (i == 5) goto inner; // OK
    (void)i;
  inner:
    (void)i;
  }

  if (1) goto inner; // expected-error {{use of undeclared label 'inner'}}

  _Cilk_for (int i = 0; i < n; ++i) {
    switch (i) {
    case 0:
      continue; // OK
    case 1:
      goto l_out; // expected-error {{use of undeclared label 'l_out'}}
    default:
      break; // OK
    }
  }

  // indirect goto
  void *addr = 0;

l_out:
  _Cilk_for (int i = 0; i < n; ++i) {
    addr = &&l_out; // expected-error {{use of undeclared label 'l_out'}}
    goto *addr;
  }

  _Cilk_for (int i = 0; i < n; ++i) {
l_in:
    addr = &&l_in;
    goto *addr;
  }
}

void empty_body() {
  _Cilk_for (int i = 0; i < 10; ++i); // expected-warning {{Cilk for loop has empty body}} \
                                      // expected-note {{put the semicolon on a separate line to silence this warning}}
  { }

  _Cilk_for (int i = 0; i < 10; ++i) // OK, the semicolon is on a separate line.
    ;
  { }

  _Cilk_for (int i = 0; i < 10; ++i); // expected-warning {{Cilk for loop has empty body}} \
                                      // expected-note {{put the semicolon on a separate line to silence this warning}}
    next();

  _Cilk_for (int i = 0; i < 10; ++i); // OK, the next statement is not a compound statement and is aligned with _Cilk_for.
  next();
}

int bar();
int baz(int);

void cilk_for_cilk_spawn() {
  _Cilk_for (int i = _Cilk_spawn bar(); i < 10; ++i); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < _Cilk_spawn bar(); ++i); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; i += _Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; ++i) {
    _Cilk_spawn bar();
    _Cilk_spawn bar();
  }

  _Cilk_for (int i = 0; i < 10; ++i)
    _Cilk_spawn bar();

  _Cilk_for (int i = 0; i < 10; ++i) {
    _Cilk_sync;
  }

  _Cilk_for (int i = 0; i < 10; ++i)
    _Cilk_sync;

  _Cilk_for (int i = 0; i < 10; ++i); // expected-warning {{Cilk for loop has empty body}} \
                                      // expected-note {{put the semicolon on a separate line to silence this warning}}
   _Cilk_spawn bar();

  _Cilk_for (int i = 0; i < 10; ++i)
    if (i == 5)
      _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}
    else
      _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  int x = 0;
  _Cilk_for (int i = 0; i < 10; ++i)
    x = _Cilk_spawn bar(); // OK

  _Cilk_for (int i = 0; i < 10; ++i)
    x = baz(_Cilk_spawn baz(i)); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_sync;

  _Cilk_for (int i = 0; i < 10; ++i)
   if (1) _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; ++i)
    if (1) {} else _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; ++i)
    while (1) _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; ++i)
    for (;;) _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; ++i)
    switch (1) {
    case 0: _Cilk_spawn baz(_Cilk_spawn bar()); break; // expected-error {{_Cilk_spawn is not at statement level}}
    default: _Cilk_spawn baz(_Cilk_spawn bar()); break; // expected-error {{_Cilk_spawn is not at statement level}}
    }

  _Cilk_for (int i = 0; i < 10; ++i)
    switch (_Cilk_spawn bar()) { // expected-error {{_Cilk_spawn is not at statement level}}
    default: ;
    }

  _Cilk_for (int i = 0; i < 10; ++i)
    do {
      _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}
    } while (_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; ++i)
L: _Cilk_spawn baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}

  _Cilk_for (int i = 0; i < 10; ++i)
    baz(_Cilk_spawn bar()); // expected-error {{_Cilk_spawn is not at statement level}}
}

void jump_between_spawning_blocks() {
  _Cilk_for (int i = 0; i < 10; ++i) {
  l1: (void)i;
    _Cilk_for (int j = 0; j < 10; ++j) {
  l2: (void)j;
       goto l1; // expected-error {{use of undeclared label 'l1'}}
       goto l2;
       goto l3; // expected-error {{use of undeclared label 'l3'}}
    }
    goto l1;
    goto l2; // expected-error {{use of undeclared label 'l2'}}
    goto l3; // expected-error {{use of undeclared label 'l3'}}
  }
l3:
  _Cilk_spawn jump_between_spawning_blocks();
  goto l1; // expected-error {{use of undeclared label 'l1'}}
  goto l2; // expected-error {{use of undeclared label 'l2'}}
  goto l3;
}

void test_wide_integers() {
  _Cilk_for (__int128 i = 0; i < 10; ++i); // expected-warning {{implicit loop count downcast from '__int128' to 'unsigned long long' in '_Cilk_for'}}
  _Cilk_for (int i = 0; i < (__int128)10; ++i); // expected-warning {{implicit loop count downcast from '__int128' to 'unsigned long long' in '_Cilk_for'}}
  _Cilk_for (int i = 0; i < 10; i += (__int128)1); // expected-warning {{implicit loop count downcast from '__int128' to 'unsigned long long' in '_Cilk_for'}}
}

int grainsize();
struct Foo {};
struct Foo get_foo();

void cilk_for_grainsize() {
  #pragma cilk grainsize = 65
  _Cilk_for (int i = 0; i < 100; ++i); // OK

  #pragma cilk grainsize = 'a'
  _Cilk_for (int i = 0; i < 100; ++i); // OK

  #pragma cilk grainsize = 65.3f // expected-warning-re {{implicit conversion from 'float' to 'int' changes value from 65.[0-9]+ to 65}}
  _Cilk_for (int i = 0; i < 100; ++i);

  int n = 20;
  #pragma cilk grainsize = n
  _Cilk_for(int i = 0; i < 10; ++i); // OK

  #pragma cilk grainsize = n/2
  _Cilk_for(int i = 0; i < 10; ++i); // OK

  #pragma cilk grainsize = ++n // OK
  _Cilk_for(int i = 0; i < 10; ++i);

  #pragma cilk grainsize = grainsize() // OK
  _Cilk_for(int i = 0; i < 10; ++i);

  #pragma cilk grainsize = "65" // expected-warning {{incompatible pointer to integer conversion initializing 'int' with an expression of type 'char [3]'}}
  _Cilk_for(int i = 0; i < 100; ++i);

  #pragma cilk grainsize = -1 // expected-error {{the behavior of Cilk for is unspecified for a negative grainsize}}
  _Cilk_for(int i = 0; i < 100; ++i);

  #pragma cilk grainsize = get_foo() // expected-error {{initializing 'int' with an expression of incompatible type 'struct Foo'}} \
                                     // expected-note {{grainsize must evaluate to a type convertible to 'int'}}
  _Cilk_for(int i = 0; i < 10; ++i);
}
