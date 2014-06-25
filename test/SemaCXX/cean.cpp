// RUN: %clang_cc1 -triple x86_64-apple-macos10.7.0 -verify -fopenmp -ferror-limit 140 -fcilkplus %s

struct S { // expected-note {{candidate constructor (the implicit copy constructor) not viable: no known conversion from 'int' to 'const S &' for 1st argument}} expected-note {{candidate function (the implicit copy assignment operator) not viable: cannot convert argument of incomplete type 'void' to 'const S'}} expected-note {{candidate function (the implicit copy assignment operator) not viable: no known conversion from 'int' to 'const S' for 1st argument}}
  int a;
};

const int csarr[20] = {2};
int sarr[20] = {csarr[:]}; // expected-error {{extended array notation is not allowed}}

int test (int x = csarr[:]) {
  return x;
}

template <class C>
C test (C x[]) {
  return x[2:5]; // expected-error {{extended array notation is not allowed}}
}

template <class C>
C test2 (C x, C y) { //expected-note {{candidate function [with C = S] not viable: no known conversion from 'S *' to 'S' for 1st argument; remove &}} expected-note {{candidate template ignored: deduced conflicting types for parameter 'C' ('S *' vs. 'S')}}
  return x;
}

template <class C>
C test3 (C *x, C y) {
  return *x;
}

int test4(S x, S y) {
  return 0;
}

int (*fn[56])();

int main(int argc, char **argv) { // expected-note {{declared here}}
  int *arr;
  int carr[56], carrarr[56][28], carr1[28];
  int varr[argc] = {__sec_implicit_index(0)}; // expected-error {{extended array notation is not allowed}}
  S sarr[20], res;

  carr[:]=fn[:](); // expected-error {{rank of the expression must be zero}}
  test(carr); // expected-note {{in instantiation of function template specialization 'test<int>' requested here}}
  arr[:]=5; // expected-error {{cannot define default length for non-array type 'int *'}}
  arr[::]=5; // expected-error {{expected unqualified-id}}
  arr[:::]=5; // expected-error {{expected unqualified-id}}
  arr[::::]=5; // expected-error {{expected unqualified-id}} expected-error {{expected ']'}} expected-note {{to match this '['}}
  arr[:::::]=5; // expected-error {{expected unqualified-id}} expected-error {{expected ']'}} expected-note {{to match this '['}}
  arr[0:]=5; // expected-error {{incorrect form of extended array notation}}
  arr[0::]=5; // expected-error {{expected ']'}} expected-note {{to match this '['}}
  arr[0:5]=5;
  arr[0:5:]=5; // expected-error {{expected expression}}
  arr[0:5:2]=5;
  carr[:]=5;
  carrarr[:][:]=carrarr[:][carr1[:]];
  carrarr[:][:]=carrarr[carr[:]][:];
  carr[::]=5; // expected-error {{expected unqualified-id}}
  carr[:::]=5; // expected-error {{expected unqualified-id}}
  carr[::::]=5; // expected-error {{expected unqualified-id}} expected-error {{expected ']'}} expected-note {{to match this '['}}
  carr[:::::]=5; // expected-error {{expected unqualified-id}} expected-error {{expected ']'}} expected-note {{to match this '['}}
  carr[0:]=5; // expected-error {{incorrect form of extended array notation}}
  carr[0::]=5; // expected-error {{expected ']'}} expected-note {{to match this '['}}
  carr[0:5]=5;
  carr[0:5:]=5; // expected-error {{expected expression}}
  carr[0:5:2]=5;
  varr[:]=5;
  varr[::]=5; // expected-error {{expected unqualified-id}}
  varr[:::]=5; // expected-error {{expected unqualified-id}}
  varr[::::]=5; // expected-error {{expected unqualified-id}} expected-error {{expected ']'}} expected-note {{to match this '['}}
  varr[:::::]=5; // expected-error {{expected unqualified-id}} expected-error {{expected ']'}} expected-note {{to match this '['}}
  varr[0:]=5; // expected-error {{incorrect form of extended array notation}}
  varr[0::]=5; // expected-error {{expected ']'}} expected-note {{to match this '['}}
  varr[0:5]=5;
  varr[0:5l:]=5; // expected-error {{expected expression}}
  varr[true:5:2]=5;
  arr[arr:0:0] = 5; // expected-error {{lower bound expression has non-integer type 'int *'}}
  arr[:carr:0] = 5; // expected-error {{expected ']'}} expected-note {{to match this '['}} expected-error {{incorrect form of extended array notation}}
  arr[0:0:varr] = 5; // expected-error {{stride expression has non-integer type 'int [argc]'}}
  carr[:] = test(carr);
  carrarr[:][:] = test(); // expected-error {{rank mismatch in array section expression}}
  carrarr[:][:] = carrarr[0:5:1][:]; // expected-error {{different length value in extended array notation expression}} expected-note {{length specified here}}
  for (carr[:] = 0; carr[:] > 10; ++carr[:]) // expected-error 3 {{extended array notation is not allowed}}
    ;
  if (carr[:] == 5) // expected-note {{rank is defined by condition}}
    carrarr[:][:] = 15; // expected-error {{rank mismatch in array section expression}}
  if (carrarr[:][:] == 5) // expected-note {{rank is defined by condition}}
    carr[:] = 15; // expected-error {{rank mismatch in array section expression}}
  if (carrarr[:][:] == 5)
    carrarr[:][:] = 15;
  if (carr[:] == 5)
    carr[:] = 15;
  if (carrarr[:][:] == 5) { // expected-note {{rank is defined by condition}}
    carrarr[:][:] = 15;
    if (carr[:] == 5) // expected-error {{rank mismatch in array section expression}}
      carr[:] = 15;
  }
  if (carrarr[:][:] == 5) { // expected-note {{rank is defined by condition}}
    carrarr[:][:] = 15;
    if (carrarr[:][:] == 5)
      carr[:] = 15; // expected-error {{rank mismatch in array section expression}}
  }
  if (carrarr[:][:] == 5) { // expected-note {{length specified here}}
    carrarr[:][:] = 15;
    if (carrarr[:][:] == 5)
      carrarr[:][0:5:1] = 15; // expected-error {{different length value in extended array notation expression}}
  }
  if (carrarr[:][:] == 5) {
    carrarr[:][:] = 15;
    if (carrarr[:][:] == 5)
      carrarr[:][:] = 15;
  }

  res = __sec_reduce_add(sarr[:]); // expected-error {{type of argument is not scalar or complex}}
  res = __sec_reduce_add(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_add(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_add(carr[:]);
  *arr = __sec_reduce_add(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce_mul(sarr[:]); // expected-error {{type of argument is not scalar or complex}}
  res = __sec_reduce_mul(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_mul(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_mul(carr[:]);
  *arr = __sec_reduce_mul(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce_max(sarr[:]); // expected-error {{type of argument is not scalar}}
  res = __sec_reduce_max(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_max(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_max(carr[:]);
  *arr = __sec_reduce_max(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce_min(sarr[:]); // expected-error {{type of argument is not scalar}}
  res = __sec_reduce_min(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_min(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_min(carr[:]);
  *arr = __sec_reduce_min(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce_max_ind(sarr[:]); // expected-error {{type of argument is not scalar}}
  res = __sec_reduce_max_ind(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_max_ind(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_max_ind(carr[:]);
  *arr = __sec_reduce_max_ind(carr[0]); // expected-error {{argument of the function is not an array section}}
  *arr = __sec_reduce_max_ind(carrarr[:][:]); // expected-error {{rank of the argument must be exactly one}}

  res = __sec_reduce_min_ind(sarr[:]); // expected-error {{type of argument is not scalar}}
  res = __sec_reduce_min_ind(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_min_ind(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_min_ind(carr[:]);
  *arr = __sec_reduce_min_ind(carr[0]); // expected-error {{argument of the function is not an array section}}
  *arr = __sec_reduce_min_ind(carrarr[:][:]); // expected-error {{rank of the argument must be exactly one}}

  res = __sec_reduce_all_zero(sarr[:]); // expected-error {{type of argument is not scalar or complex}}
  res = __sec_reduce_all_zero(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_all_zero(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_all_zero(carr[:]);
  *arr = __sec_reduce_all_zero(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce_all_nonzero(sarr[:]); // expected-error {{type of argument is not scalar or complex}}
  res = __sec_reduce_all_nonzero(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_all_nonzero(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_all_nonzero(carr[:]);
  *arr = __sec_reduce_all_nonzero(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce_any_zero(sarr[:]); // expected-error {{type of argument is not scalar or complex}}
  res = __sec_reduce_any_zero(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_any_zero(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_any_zero(carr[:]);
  *arr = __sec_reduce_any_zero(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce_any_nonzero(sarr[:]); // expected-error {{type of argument is not scalar or complex}}
  res = __sec_reduce_any_nonzero(); // expected-error {{too few arguments to function call, expected 1, have 0}}
  res = __sec_reduce_any_nonzero(0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
  *arr = __sec_reduce_any_nonzero(carr[:]);
  *arr = __sec_reduce_any_nonzero(carr[0]); // expected-error {{argument of the function is not an array section}}

  res = __sec_reduce(sarr[:], sarr[:]); // expected-error {{too few arguments to function call, expected 3, have 2}}
  res = __sec_reduce(sarr[:], sarr[:], test2, 1); // expected-error {{too many arguments to function call, expected 3, have 4}}
  res = __sec_reduce(sarr[:], sarr[:], test2<S>); // expected-error {{extended array notation is not allowed}}
  res = __sec_reduce(S(), sarr[:], sarr[:]); // expected-error {{extended array notation is not allowed}}
  res = __sec_reduce(S(), sarr[:], test2<S>);
  res = __sec_reduce(0, sarr[:], test2<S>); // expected-error {{no viable conversion from 'int' to 'S'}}
  res = __sec_reduce(S(), sarr[:], test2);
  res = __sec_reduce(S(), sarr[:], 0); // expected-error {{expected unqualified-id}}
  res = __sec_reduce(S(), sarr[0], test2<S>); // expected-error {{argument of the function is not an array section}}
  res = __sec_reduce(S(), sarr[:], test4); // expected-error {{no viable overloaded '='}}

  res = __sec_reduce_mutating(sarr[:], sarr[:]); // expected-error {{too few arguments to function call, expected 3, have 2}}
  res = __sec_reduce_mutating(sarr[:], sarr[:], 0, 1); // expected-error {{expected unqualified-id}}
  res = __sec_reduce_mutating(sarr[:], sarr[:], test2<S>); // expected-error {{extended array notation is not allowed}}
  res = __sec_reduce_mutating(S(), sarr[:], sarr[:]); // expected-error {{extended array notation is not allowed}}
  res = __sec_reduce_mutating(S(), sarr[:], test2<S>); // expected-error {{argument expression must be an l-value}}
  __sec_reduce_mutating(res, sarr[:], test2<S>); // expected-error {{no matching function for call to 'test2'}}
  __sec_reduce_mutating(res, sarr[:], test2); // expected-error {{no matching function for call to 'test2'}}
  __sec_reduce_mutating(res, sarr[:], 0); // expected-error {{expected unqualified-id}}
  __sec_reduce_mutating(res, sarr[0], test2<S>); // expected-error {{argument of the function is not an array section}}
  __sec_reduce_mutating(res, sarr[:], test3);
  res = __sec_reduce_mutating(res, sarr[:], test3); // expected-error {{no viable overloaded '='}}

  carr[:] = __sec_implicit_index(); // expected-error  {{too few arguments to function call, expected 1, have 0}}
  carr[:] = __sec_implicit_index(0, 0); // expected-error {{too many arguments to function call, expected 1, have 2}}
  carr[:] = __sec_implicit_index(0.0); // expected-error {{type of argument is not integer}}
  carr[:] = __sec_implicit_index(argc); // expected-error {{expression is not an integral constant expression}} expected-note {{read of non-const variable 'argc' is not allowed in a constant expression}}
  carr[0] = __sec_implicit_index(0); // expected-error {{rank mismatch in array section expression}}
  carr[:] = __sec_implicit_index(-1); // expected-error  {{value of argument is negative integer}}
  carr[:] = __sec_implicit_index(1); // expected-error  {{rank mismatch in array section expression}}
  carrarr[:][:] = __sec_implicit_index(1) + __sec_implicit_index(0);
  for (unsigned i = 0; i > __sec_implicit_index(0); ++i); // expected-error {{extended array notation is not allowed}}
  *arr = _Cilk_spawn test(); // expected-error {{extended array notation is not allowed}}
  return test(); // expected-error {{extended array notation is not allowed}}
}

