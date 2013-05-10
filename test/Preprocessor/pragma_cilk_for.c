// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify -Wall %s

#define pragma_cilk _Pragma("cilk grainsize = 4")

void f1() {
    while (1);
    #pragma cilk grainsize = 4 // OK
    _Cilk_for (int i = 0; i < 10; i++);

    while (1);
    pragma_cilk // OK, with macros
    _Cilk_for (int i = 0; i < 10; i++);

    while (1); // expected-warning {{while loop has empty body}} \
               // expected-note {{put the semicolon on a separate line to silence this warning}}
        #pragma cilk grainsize = 4
    _Cilk_for (int i = 0; i < 10; i++);

    while (1);
#pragma cilk grainsize = 4 // OK
    _Cilk_for (int i = 0; i < 10; i++);
}
