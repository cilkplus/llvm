// RUN: %clang -fcilkplus -target x86_64-unknown-linux %s -### 2>&1 | FileCheck -check-prefix=CHECK1 %s
// CHECK1: -lcilkrts
//
// RUN: %clang -fcilkplus -target x86_64-apple-darwin %s -### 2>&1 | FileCheck -check-prefix=CHECK2 %s
// CHECK2: -lcilkrts
//
// RUN: %clang %s -### 2>&1 | FileCheck %s
// CHECK-NOT: -lcilkrts
