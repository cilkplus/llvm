// RUN: %clang_cc1 -pedantic -Wunused-label -x c %s 2>&1 | FileCheck %s -strict-whitespace

//AVT: I removed CRLF (don't know how to deal with GIT and CRLF)
// This file intentionally uses a CRLF newline style
// <rdar://problem/12639047>
// CHECK: warning: unused label 'ddd'
// CHECK-NEXT: {{^  ddd:}}
// CHECK-NEXT: {{^  \^~~~$}}
void f() {
  ddd:
  ;
}
