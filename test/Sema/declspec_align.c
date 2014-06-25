// RUN: %clang_cc1 -verify -fsyntax-only %s
//***INTEL: declspec test
__declspec(align(128)) struct S {
   int i;   // size 4
   __declspec(align(64)) short j;   // size 2
   double k;   // size 8
};

struct S1 {
   int i;   // size 4
   short j;   // size 2
   double k;   // size 8
};

struct __declspec(align(128)) S2 {
   int i;   // size 4
   short __declspec(align(64)) j;   // size 2
   double k;   // size 8
};

__declspec(align(3)) struct S3{ // expected-error {{requested alignment is not a power of 2}}
  int i;
};

__attribute__((align(4))) struct S4{ // expected-warning {{attribute 'align' is ignored, place it after "struct" to apply attribute to type declaration}}
  int i;
};

struct test{
struct S a;
struct S1 b;
struct S2 c;
int end;
} test;

_Static_assert(__alignof(test.a) == 128, "struct S alignment is not 128");
_Static_assert(__alignof(test.a.j) == 64, "struct S.a.j alignment is not 128");
_Static_assert(__alignof(test.a) != __alignof(test.b), "struct S alignment is the same as S1");
_Static_assert(__alignof(test.a.j) != __alignof(test.b.j), "struct S.j alignment is the same as S1.j");
_Static_assert(__alignof(test.c) == 128, "struct S2 alignment is not 128");
_Static_assert(__alignof(test.c.j) == 64, "struct S2.a.j alignment is not 128");

