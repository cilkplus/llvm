// RUN: %clang -O3 -std=c++11 -S -emit-llvm %s -o - | FileCheck %s

void anchor(int);

int max_reduction(int arr[100]) {
  int x = 0;
  anchor(1);
  #pragma simd reduction(max:x)
  for (int i = 0; i < 100; ++i)
    if (arr[i] > x) x = arr[i];
  // CHECK: call void @_Z6anchori(i32 1)
  // CHECK: phi  <[[WIDTH:[0-9]+]] x i32>
  // CHECK: icmp sgt <[[WIDTH]] x i32>
  // CHECK: select <[[WIDTH]] x i1>
  // CHECK: extractelement <[[WIDTH]] x i32>
  return x;
}

int plus_reduction(int arr[100]) {
  int sum = 0;
  anchor(2);
  #pragma simd reduction(+:sum)
  for (int i = 0; i < 100; ++i)
    sum += arr[i];
  // CHECK: call void @_Z6anchori(i32 2)
  // CHECK: phi  <[[WIDTH:[0-9]+]] x i32>
  // CHECK: add <[[WIDTH]] x i32>
  // CHECK: extractelement <[[WIDTH]] x i32>
  return sum;
}

int prod_reduction(int arr[100]) {
  int prod = 1;
  anchor(3);
  #pragma simd reduction(*:prod)
  for (int i = 0; i < 100; ++i)
    prod *= arr[i];
  // CHECK: call void @_Z6anchori(i32 3)
  // CHECK: phi  <[[WIDTH:[0-9]+]] x i32>
  // CHECK: mul <[[WIDTH]] x i32>
  // CHECK: extractelement <[[WIDTH]] x i32>
  return prod;
}

int xor_reduction(int arr[100]) {
  int x = 0;
  anchor(4);
  #pragma simd reduction(^:x)
  for (int i = 0; i < 100; ++i)
    x ^= arr[i];
  // CHECK: call void @_Z6anchori(i32 4)
  // CHECK: phi  <[[WIDTH:[0-9]+]] x i32>
  // CHECK: xor <[[WIDTH]] x i32>
  // CHECK: extractelement <[[WIDTH]] x i32>
  return x;
}

// Boolean reductions are already optimized such that if-conversion will fail.
bool land_reduction(bool arr[100]) {
    bool land = 1;
  anchor(5);
  #pragma simd reduction(&& : land)
  for (int i = 0; i < 100; ++i)
    land = land && arr[i];
  return land;
}

// The vectorizer is currently unable to vectorize multple reductions.
int multiple1(int arr[100], int b[100]) {
  int add = 0;
  bool x = 0;
  #pragma simd reduction(+:add) reduction(^:x)
  for (int i = 0; i < 100; ++i) {
    add += arr[i];
    x ^= b[i];
  }
  return add + x;
}
