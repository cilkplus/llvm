// RUN: %clang_cc1 -fcilkplus -emit-llvm -O2 -o - %s | FileCheck %s
// REQUIRES: cilkplus

#define N SMALL_INT_ARRAY_SIZE
#define TEST_CONFORM 0
#define TEST_NOT_CONFORM 1
#define SMALL_INT_ARRAY_SIZE 155

int a[N][N];
int b[N];

int add (int x, int y){
  return x + y;
}

int check (){
  int i;
  _Cilk_for (i = 0; i < N; i++)
    {
      b[i] = __sec_reduce (0, a[i][:], add);
    }
  for (i = 0; i < N; i++)
    {
      if (b[i] != ((N * N - N) / 2 + N * i))
        {
          return TEST_NOT_CONFORM;
        }
    }
  return TEST_CONFORM;
}

//CHECK: define {{.+}}@check()
//CHECK: call {{.+}}__cilkrts_cilk_for{{.+}}__cilk_for_helper
//CHECH: define {{.+}}__cilk_for_helper
