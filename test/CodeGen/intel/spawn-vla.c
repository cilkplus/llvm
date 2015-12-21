// RUN: %clang_cc1 -fcilkplus -triple i386-unknown-unknown %s -emit-llvm -o - | FileCheck %s
// REQUIRES: cilkplus

void reverse( int n, int a[n] ) {
// CHECK: define {{.*}}reverse
    if( n<=1 ) {
        return;
    } else {
        int m = n/2;
        int b_[m];
// CHECK: [[sizeof_val:%.+]] = mul
// CHECK: {{.*}}call {{.*}}malloc{{.*}}[[sizeof_val]]
// CHECK: %{{.*}} = bitcast{{.*}}to i
        int* b = b_;
        b[0:m] = a[0:m];
        _Cilk_spawn reverse(m,b);
        double d[n-m];
// CHECK: {{.*}}call {{.*}}malloc{{.*}}%
// CHECK: %{{.*}} = bitcast{{.*}}to double
        d[0:n-m] = a[m:n-m];
        int c[n-m];
// CHECK: {{.*}}call {{.*}}malloc{{.*}}%
// CHECK: %{{.*}} = bitcast{{.*}}to i
        c[0:n-m] = d[0:n-m];
        reverse(n-m, c);
        _Cilk_sync;
        a[0:n-m] = c[0:n-m];
        a[n-m:m] = b[0:m];
// CHECK: bitcast
// CHECK: call{{.*}}free
// CHECK: bitcast
// CHECK: call{{.*}}free
// CHECK: bitcast
// CHECK: call{{.*}}free
    }
}

