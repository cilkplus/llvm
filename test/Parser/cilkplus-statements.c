// RUN: %clang_cc1 -fcilkplus -fsyntax-only -verify %s

int foo();
int bar(int);

void test() {
  _Cilk_sync;
  _Cilk_sync;_Cilk_sync;
  _Cilk_sync,_Cilk_sync; // expected-error {{expected ';' after _Cilk_sync statement}}
  _Cilk_sync _Cilk_sync; // expected-error {{expected ';' after _Cilk_sync statement}}
  _Cilk_sync foo;        // expected-error {{expected ';' after _Cilk_sync statement}}
  if (_Cilk_sync);       // expected-error {{expected expression}}
  while (_Cilk_sync);    // expected-error {{expected expression}}
  int a = _Cilk_sync;    // expected-error {{expected expression}}
  a = _Cilk_sync;        // expected-error {{expected expression}}
_Cilk_sync:              // expected-error {{expected ';' after _Cilk_sync statement}}
  goto _Cilk_sync;

  _Cilk_spawn foo();
  int b = _Cilk_spawn foo();
  b = _Cilk_spawn foo();
  _Cilk_spawn;            // expected-error {{expected expression}}
  _Cilk_spawn _Cilk_sync; // expected-error {{expected expression}}
  _Cilk_spawn { foo(); }  // expected-error {{expected expression}}
}
