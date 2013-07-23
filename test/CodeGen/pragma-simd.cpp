// RUN: %clang_cc1 -std=c++11 -emit-llvm %s -o - | FileCheck %s

void anchor(int);

void test() {
  #pragma simd
  for (int i = 0; i < 10; ++i) ;
// CHECK: define void @_Z4testv(
// CHECK: llvm.mem.parallel_loop_access !0
// CHECK: llvm.loop !0
  anchor(50);
// CHECK: call void @_Z6anchori(i32 50)
}

void test2(int *A, int N) {
  int *IE = A + N;
  #pragma simd
  for (int *I = A; I != IE; ++I)
    *I *= 2;
// CHECK: define void @_Z5test2Pii
// CHECK: llvm.mem.parallel_loop_access !1
// CHECK: llvm.loop !1

  anchor(60);
// CHECK: call void @_Z6anchori(i32 60)
}

constexpr int width(int i) { return 1 << i; }

void test_vectorlength() {
  #pragma simd vectorlength(4)
  for (int i = 0; i < 10; ++i) ;
// CHECK: define void @_Z17test_vectorlengthv
// CHECK: llvm.mem.parallel_loop_access !2
// CHECK: llvm.loop !2

  anchor(101);
// CHECK: call void @_Z6anchori(i32 101)

  const int W = 1 << 3;
  #pragma simd vectorlength(W)
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !4
// CHECK: llvm.loop !4

  anchor(102);
// CHECK: call void @_Z6anchori(i32 102)

  #pragma simd vectorlength(width(4))
  for (int i = 0; i < 10; ++i) ;
// CHECK: llvm.mem.parallel_loop_access !6
// CHECK: llvm.loop !6

  anchor(103);
// CHECK: call void @_Z6anchori(i32 103)
}

void touch(int);

void test_access_metadata() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    anchor(201);
    touch(i);
    anchor(202);
  }
  anchor(203);
  // CHECK: define void @_Z20test_access_metadatav
  // CHECK: call void @_Z6anchori(i32 201)
  // CHECK: llvm.mem.parallel_loop_access !8
  // CHECK: call void @_Z6anchori(i32 202)
  // CHECK: !llvm.loop !8
  // CHECK: call void @_Z6anchori(i32 203)
}

void test_continue() {
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    anchor(401);
    if (i == 5) continue;
    anchor(402);

    if (true) continue;
    anchor(403);
  }
  anchor(410);
  // CHECK: call void @_Z6anchori(i32 401)
  // CHECK: icmp eq i32
  // CHECK: br label [[CONTINUE_BLOCK:%[_A-Za-z0-9]+]]
  // CHECK: call void @_Z6anchori(i32 402)
  // CHECK-NEXT: br label [[CONTINUE_BLOCK]]
  // CHECK-NOT: call void @_Z6anchori(i32 403)
  // CHECK: call void @_Z6anchori(i32 410)
}

struct X {
  int i;
  X();
  ~X();
};
struct S {
  S();
  ~S();
  void operator=(S s);
};
void test_lastprivate() {

  // Ensure that update block is created, d is stored
  double d;
  #pragma simd lastprivate(d)
  for (int i = 0; i < 10; ++i) {
    (void)d;
    anchor(501);
  }
  anchor(502);
  // CHECK: call void @_Z6anchori(i32 501)
  // CHECK: icmp ne i8 %{{.+}}, 0
  // CHECK: call void @_Z6anchori(i32 501)
  // CHECK: load double*
  // CHECK-NEXT: getelementptr
  // CHECK-NEXT: load double**
  // CHECK-NEXT: store double
  // CHECK: call void @_Z6anchori(i32 502)

  // Check that local x being copied into x, x destructor being called
  X x;
  #pragma simd lastprivate(x)
  for (int i = 0; i < 10; ++i) {
    (void)x;
    anchor(511);
  }
  anchor(512);
  // CHECK: call void @_Z6anchori(i32 511)
  // CHECK: icmp ne i8 %{{.+}}, 0
  // CHECK: call void @_Z6anchori(i32 511)
  // CHECK: call void @llvm.memcpy
  // CHECK: call void @llvm.memcpy
  // CHECK: call void @_Z6anchori(i32 512)

  // Check for assignment op. invocation and destructor called for temporary
  S s;
  #pragma simd lastprivate(s)
  for (int i = 0; i < 10; ++i) {
    (void)s;
    anchor(521);
  }
  anchor(522);
  // CHECK: call void @_Z6anchori(i32 521)
  // CHECK: icmp ne i8 %{{.+}}, 0
  // CHECK: call void @_Z6anchori(i32 521)
  // CHECK: call void @_ZN1SaSES_
  // CHECK-NEXT: call void @_ZN1SD1Ev
  // CHECK: call void @_Z6anchori(i32 522)

  // Check that all lastprivate variables get updated
  #pragma simd lastprivate(d) lastprivate(x) lastprivate(s)
  for (int i = 0; i < 10; ++i) {
    (void)d;
    (void)x;
    (void)s;
    anchor(531);
  }
  anchor(532);
  // CHECK: call void @_Z6anchori(i32 531)
  // CHECK: icmp ne i8 %{{.+}}, 0
  // CHECK: call void @_Z6anchori(i32 531)
  // CHECK: store double
  // CHECK: call void @llvm.memcpy
  // CHECK: call void @llvm.memcpy
  // CHECK: call void @_ZN1SaSES_
  // CHECK-NEXT: call void @_ZN1SD1Ev
  // CHECK: call void @_Z6anchori(i32 532)
}

const int a = 10;

struct A {
  static int s;
};
int g;
static int G;
void test_private_static() {
  static int L;

  // Check that the outer static is still being used if not a SIMD Variable
  #pragma simd
  for (int i = 0; i < 10; ++i) {
    anchor(601);
    L = 1;
    A::s = 2;
    G = 3;
    g = 4;
  }
  anchor(602);
  // CHECK: call void @_Z6anchori(i32 601)
  // CHECK: store i32 1, i32* @_ZZ19test_private_staticvE1L
  // CHECK: store i32 2, i32* @_ZN1A1sE
  // CHECK: store i32 3, i32* @_ZL1G
  // CHECK: store i32 4, i32* @g
  // CHECK: call void @_Z6anchori(i32 602)

  // Check that outer static not used when declared a SIMD Variable
  #pragma simd private(L) private(A::s) private(G) private(g)
  for (int i = 0; i < 10; ++i) {
    anchor(611);
    L = 1;
    A::s = 2;
    G = 3;
    g = 4;
  }
  anchor(612);
  // CHECK: call void @_Z6anchori(i32 611)
  // CHECK-NOT: store i32 1, i32* @_ZZ19test_private_staticvE1L
  // CHECK-NOT: store i32 2, i32* @_ZN1A1sE
  // CHECK-NOT: store i32 3, i32* @_ZL1G
  // CHECK-NOT: store i32 4, i32* @g
  // CHECK: call void @_Z6anchori(i32 612)

  // Make sure nested lambdas are not being called with the static as capture
  #pragma simd private(L) private(A::s)
  for (int i = 0; i < 10; ++i) {
    anchor(621);
    [&] {
      [&] {
        L = 1;
      }();
      A::s = 2;
    }();
  }
  anchor(622);
  // CHECK: call void @_Z6anchori(i32 621)
  // CHECK-NOT: store i32* @_ZZ19test_private_staticvE1L
  // CHECK-NOT: store i32* @_ZN1A1sE
  // CHECK: call void @_Z6anchori(i32 622)

  // Make sure that lambdas are being called with the local variable as capture
  #pragma simd private(L) private(A::s)
  for (int i = 0; i < 10; ++i) {
    anchor(631);
    L = 0;
    A::s = 1;
    anchor(632);
    [&] {
      [&] {
        L = 2;
      }();
      A::s = 3;
    }();
  }
  anchor(633);
  // CHECK: call void @_Z6anchori(i32 631)
  // CHECK: store i32 0, i32* [[LOCAL_L:%[_A-Za-z0-9\.]]]
  // CHECK-NEXT: store i32 1, i32* [[LOCAL_S:%[_A-Za-z0-9\.]]]
  // CHECK: call void @_Z6anchori(i32 632)
  // CHECK: store i32* [[LOCAL_L]]
  // CHECK: store i32* [[LOCAL_S]]
  // CHECK: call void @_Z6anchori(i32 633)
}

void test_linear() {
  int x = 0;
  anchor(600);
  #pragma simd linear(x)
  for (int i = 0; i < 100; ++i) {
    anchor(601);
    (void)x;
  }
  // CHECK: call void @_Z6anchori(i32 600)
  // CHECK: add i32
  // CHECK-NEXT: store i32
  // CHECK-NEXT: call void @_Z6anchori(i32 601)

  anchor(610);
  #pragma simd linear(x:5)
  for (int i = 0; i < 100; ++i) {
    anchor(611);
    (void)x;
  }
  // CHECK: call void @_Z6anchori(i32 610)
  // CHECK: mul i32 %{{.+}}, 5
  // CHECK-NEXT: add i32
  // CHECK-NEXT: store i32
  // CHECK-NEXT: call void @_Z6anchori(i32 611)

  // Check that pointers are being correctly incremented
  int *p;
  anchor(620);
  #pragma simd linear(p)
  for (int i = 0; i < 100; ++i) {
    anchor(621);
    (void)p;
  }
  // CHECK: call void @_Z6anchori(i32 620)
  // CHECK: getelementptr i32*
  // CHECK-NEXT: store i32*
  // CHECK-NEXT: call void @_Z6anchori(i32 621)

  anchor(630);
  #pragma simd linear(p)
  for (int i = 0; i < 100; ++i) {
    anchor(631);
    (void)p;
  }
  // CHECK: call void @_Z6anchori(i32 630)
  // CHECK: getelementptr i32*
  // CHECK-NEXT: store i32*
  // CHECK-NEXT: call void @_Z6anchori(i32 631)
}

void use(int&);

void test_array1() {
  int array[10] = {0};
  #pragma simd private(array)
  for (int i = 0; i < 10; i++) {
    anchor(800);
    use(array[i]);
    anchor(801);
  }
  // A local array is allocated and no extra indirection will be required.
  //
  // CHECK: define void @_Z11test_array1v()
  //
  // CHECK: alloca [10 x i32]
  // CHECK: alloca [10 x i32]
  //
  // CHECK: call void @_Z6anchori(i32 800)
  // CHECK-NOT: load [10 x i32]**
  // CHECK: call void @_Z6anchori(i32 801)
}

void test_array2() {
  int array[10] = {0};
  #pragma simd
  for (int i = 0; i < 10; i++) {
    anchor(802);
    use(array[i]);
    anchor(803);
  }
  //
  // CHECK: define void @_Z11test_array2v()
  //
  // CHECK: alloca [10 x i32]
  // CHECK-NOT: alloca [10 x i32]
  // CHECK: call void @_Z6anchori(i32 802)
  // CHECK: load [10 x i32]**
  // CHECK: call void @_Z6anchori(i32 803)
}

void test_array3() {
  int array[810] = {0};
  #pragma simd lastprivate(array)
  for (int i = 0; i < 10; i++) {
    use(array[i]);
    anchor(811);
  }
  anchor(812);
  // CHECK: define void @_Z11test_array3v()
  // CHECK: call void @_Z6anchori(i32 811)
  //
  // Check if there is a loop for the update.
  //
  // CHECK: icmp ult {{.*}}, 810
  //
  // CHECK: call void @_Z6anchori(i32 812)
}

void test_array4() {
  int array[820][821][822];

  #pragma simd lastprivate(array)
  for (int i = 0; i < 10; i++) {
    use(array[i][i][i]);
    anchor(823);
  }
  anchor(824);
  // CHECK: define void @_Z11test_array4v()
  // CHECK: call void @_Z6anchori(i32 823)
  //
  // CHECK: icmp ult {{.*}}, 820
  // CHECK: icmp ult {{.*}}, 821
  // CHECK: icmp ult {{.*}}, 822
  //
  // CHECK: call void @_Z6anchori(i32 824)
}

struct Y {
  Y();
  void operator=(const Y&);
};

void use(const Y&);

void test_array5() {
  Y array[10];

  #pragma simd lastprivate(array)
  for (int i = 0; i < 10; i++) {
    use(array[i]);
    anchor(830);
  }
  anchor(831);
  // CHECK: define void @_Z11test_array5v()
  // CHECK: call void @_Z6anchori(i32 830)
  // CHECK: call void @_ZN1YaSERKS_
  // CHECK: call void @_Z6anchori(i32 831)
}

// test()
// CHECK: !0 = metadata !{metadata !0}
// test1()
// CHECK: !1 = metadata !{metadata !1}
// test_vectorlength()
// CHECK: !2 = metadata !{metadata !2, metadata !3}
// CHECK: !3 = metadata !{metadata !"llvm.vectorizer.width", i32 4}
// CHECK: !4 = metadata !{metadata !4, metadata !5}
// CHECK: !5 = metadata !{metadata !"llvm.vectorizer.width", i32 8}
// CHECK: !6 = metadata !{metadata !6, metadata !7}
// CHECK: !7 = metadata !{metadata !"llvm.vectorizer.width", i32 16}
// CHECK: !8 = metadata !{metadata !8}
