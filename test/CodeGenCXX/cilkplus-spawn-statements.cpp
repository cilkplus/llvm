// RUN: %clang_cc1 -fcilkplus -std=c++11 -emit-llvm %s -o - | FileCheck %s

/*
 *  A _Cilk_spawn can only appear in 3 places
 *    as the *entire* body of an expression statement
 *      (covered by Test 1, 2, 6, 10)
 *
 *    as the *entire* right hand side of an assignment expression
 *      (covered by Test 4 and 5)
 *
 *    as the entire initializer-clause in a simple declaration
 *      (covered by Test 3, 7, 8, and 9)
 *
 *  Thus we have covered all possible cases for _Cilk_spawn
 */

int global = 0;

struct Foo {
  int x;

  Foo() = default;
  ~Foo() {global++;}
};

Foo operator+(const Foo& lhs, const Foo& rhs) {
  return Foo();
}


struct Bar {
  float x;

  Bar() : x(10.0f) {}
  ~Bar() = default;
};

Foo param1;
Foo param2;

int f(Foo shouldBeDestructed) {
  return 3;
}

Bar g(Foo shouldBeDestructed) {
  return Bar();
}

void h() {
  global++;
}


// Make sure Foo() is constructed before spawn point
// and not destructed until after spawn point
// (in the sequence of operations within the spawning
// statement that are sequence after the spawn point,
// i.e. in the child of the spawn)
void test1() {
  _Cilk_spawn f(Foo());
}


// Make sure unnamed temp variable (parem1 + parem2) is
// constructed before spawn point and not destructed
// until after spawn point (i.e. in the child of the spawn)
void test2() {
  _Cilk_spawn f(param1 + param2);
}


// Same as above, make sure Foo() is constructed and
// destructed at the right time. Also make sure that
// "x" is initialized in the child
void test3() {
  int x = _Cilk_spawn f(Foo());
}


// Same as above, make sure unnamed temp variable
// (parem1 + parem2)  is constructed and destructed
// at the right time. Also make sure that "y" is
// initialized before the spawn and assigned in the child
void test4() {
  int y;
  y = _Cilk_spawn f(param1 + param2);
}


// make sure calculating the index of "z" (as well as
// constructing and destroying the first temp Foo)
// is done before spawn point as well as confirming
// that construction of the second Foo happens before
// the spawn point and destruction happens in the child
// Make sure the assignment to "z" happens in the child
void test5() {
  int z[10];
  z[f(Foo())] = _Cilk_spawn f(Foo());
}


// Make sure param1 is *not* destructed in the child,
// as it is not an unnamed, temporary variable. Since
// param1 is passed by value we should make sure that
// the copy is destructed as required
void test6() {
  _Cilk_spawn f(param1);
}


// Same as Test 6, make sure param2 is *not* destructed
// Also make sure that "a" is initialized in the child
void test7() {
  Bar a = _Cilk_spawn g(param2);
}


// Make sure conversion from int to float takes place
// in the child
void test8() {
  float b = _Cilk_spawn f(Foo());
}


// Make sure lambda variable is handled correctly
// along with use of temp variable
void test9() {
  auto test_lambda = [](Foo shouldBeDestructed) {
      return 3;
  };
  int d = _Cilk_spawn test_lambda(Foo());
}


// tests most basic _Cilk_spawn functionality
void test10() {
  _Cilk_spawn h();
}


int main() {
  // Need to have a CHECK statement somewhere
  // When _Cilk_spawn functionality is added need to add CHECK
  // statements to make sure all the tests work as expected
  // CHECK: alloca
  return 0;
}

