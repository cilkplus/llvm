// RUN: %clang_cc1 -fcilkplus -emit-llvm -O2 %s -o %t
// RUN: FileCheck -input-file=%t %s

class Foo {
public:
  Foo() : Val(0) {}
  Foo(int x) : Val(x) {}
  virtual ~Foo() {}
  operator int() { return Val; }

private:
  int Val;
};

int func(Foo f) { return f; }

void test_loop_count() {
  // CHECK: test_loop_count

  _Cilk_for(int i = 0; i < Foo(7); i += 4);
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}}, i32 2,

  _Cilk_for(int i = 0; i < 10; i += Foo(2));
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}}, i32 5,

  _Cilk_for(int i = 13; i >= func(Foo(7)); i -= 4);
  // CHECK: call void @__cilkrts_cilk_for_32({{.*}}, i32 2,
}
