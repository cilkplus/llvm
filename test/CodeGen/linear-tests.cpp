// RUN: %clang -O0 -fcilkplus -std=c++11 -S -emit-llvm %s -o - | FileCheck %s

void anchor(int);

signed int total_size;
unsigned int utotal_size;

int linear_test1(int array_foo[100]) {
  int address = 0;
  signed long step = 4;
  unsigned char ustep = 8;
  
  total_size = 0;
  
  #pragma simd linear(utotal_size:ustep)
  for (int idx = 0; idx < 100; ++idx) {
    address += array_foo[idx];
    total_size += step;
  }
  #pragma simd linear(utotal_size:step)
  for (unsigned idx = 0; idx < 100; ++idx) {
    address += array_foo[idx];
    total_size += step;
  }
  #pragma simd linear(total_size:ustep)
  for (unsigned idx = 0; idx < 100; ++idx) {
    address += array_foo[idx];
    total_size += step;
  }

  return address;
}

int linear_test2(int array_foo[100], double offset) {
  int address = 0;
  int step = 4;

  total_size = 0;

  #pragma simd linear(offset:step)
  for (int idx = 0; idx < 100; ++idx) {
    address += array_foo[idx];
    offset += step;
  }
  // CHECK: call void @llvm.intel.pragma(metadata !{metadata !"SIMD_LOOP", metadata !"LINEAR"{{.+}}
  // CHECK: [[STEPDBL:%[0-9]+]] = sitofp i32 %{{[0-9]+}} to double
  // CHECK-NEXT: [[ADDR:%[0-9]+]] = fadd double %{{[0-9]+}}, [[STEPDBL]]
  // CHECK-NEXT: store double [[ADDR]], double*
  return address + offset;
}

int linear_test3(int xxarray_foo[100]) {
  int xxaddress = 0;
  long long xxstep = 4;

  #pragma simd linear(total_size:xxstep)
  for (int xxidx = 0; xxidx < 100; ++xxidx) {
    xxaddress += xxarray_foo[xxidx];
    total_size += xxstep;
  // CHECK: call void @llvm.intel.pragma(metadata !{metadata !"SIMD_LOOP", metadata !"LINEAR"{{.+}}
  // CHECK: [[STEP64:%[0-9]+]] = load i64*
  // CHECK-NEXT: [[STEP32:%[0-9]+]] = trunc i64 [[STEP64]] to i32
  // CHECK-NEXT: %{{[0-9]+}} = mul i32 %{{[0-9]+}}, [[STEP32]]
  }
  return xxaddress;
}

