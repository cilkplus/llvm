// RUN: %clang_cc1 -fcilkplus -x c++ -triple x86_64-unknown-unknown -fexceptions -fcxx-exceptions -O2 -emit-llvm %s -o - | FileCheck %s
// REQUIRES: cilkplus

// See cq369391.
void foo() {
  throw 5;
}

int main() {
  // CHECK: define {{.*}}@main
  try {
    _Cilk_spawn foo();
  } catch (int &i) {
    // CHECK: [[EHID:%.+]] = call {{.*}}@llvm.eh.typeid.for
    // CHECK-NEXT: [[MATCHES:%.+]] = icmp eq i32 {{%.+}}, [[EHID]]
    // CHECK-NEXT: br i1 [[MATCHES]]
    // CHECK: call {{.*}}__cxa_begin_catch
    //
    // CHECK: [[LPAD1:%.+]] = insertvalue {{.+}}, 0
    // CHECK-NEXT: [[LPAD2:%.+]] = insertvalue {{.+}}[[LPAD1]]{{.+}}, 1
    // CHECK-NEXT: resume {{.*}}[[LPAD2]]
    if (i == 5) return 0;
  }
  return 1;
}

