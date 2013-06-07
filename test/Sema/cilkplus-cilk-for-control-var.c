// RUN: %clang_cc1 -fcilkplus -fsyntax-only -Wall -verify %s

void func1(const int);
void func2(int *i);
void func3(const int *i);
void func4(const int * const i);
void func5(int, int, int *i);
void func6(int, int, int, int, int, int, int *i);
int func7(int i);

void test_control_var_modification_in_body() {
  _Cilk_for (int i = 0; i < 10; ++i) {
    int j = i; // OK
    int *p = &i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}} \
                 //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    const int *cp = &i; // OK
    const int *cp2;
    cp2 = &i; // OK

    i = 19; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
            //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    i++; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
         //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    ++i; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
         //expected-note@12{{'_Cilk_for' loop control variable declared here}}


    i--; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
         //expected-note@12{{'_Cilk_for' loop control variable declared here}}


    --i; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
         //expected-note@12{{'_Cilk_for' loop control variable declared here}}


    i += 2; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
            //expected-note@12{{'_Cilk_for' loop control variable declared here}}


    i -= 3; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
            //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    i *= 3; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
            //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    i /= 2; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
            //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    {
      i = 2; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
             //expected-note@12{{'_Cilk_for' loop control variable declared here}}

      i++; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
           //expected-note@12{{'_Cilk_for' loop control variable declared here}}
    }

    int *p2 = 0;
    p2 = &i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}}\
             //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    void *p3 = 0;
    p3 = (void*)&i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}}\
                    //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    int *p4 = (func1(0), &i); //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}}\
                              //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    p4 = (func1(0), &i); //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}}\
                       //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    p4 = &i, func1(0); //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}}\
                       //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    func1(i); // OK

    func2(&i); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
               //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    func2(&j); // OK

    func2((int*)&i); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
                     //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    func3(&i); // OK

    func4(&i); // OK

    func5(0, 0, &i); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
                     //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    func6(0, 0, 0, 0, 0, 0, &i); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
                                 //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    i = func7(i); //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                  //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    _Cilk_for (int x = 0; x < 20; ++x) {
      _Cilk_for (int y = 0; y < 10; ++y) {
        _Cilk_for (int z = 0; z < 10; ++z) {
          z = y; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                 //expected-note@-2{{'_Cilk_for' loop control variable declared here}}
          z = x; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                 //expected-note@-4{{'_Cilk_for' loop control variable declared here}}
          x = z; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                 //expected-note@-7{{'_Cilk_for' loop control variable declared here}}
          x = y = z; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                     //expected-note@-9{{'_Cilk_for' loop control variable declared here}} \
                     //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                     //expected-note@-8{{'_Cilk_for' loop control variable declared here}}
          int k;
          int l;
          k = y = z; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                     //expected-note@-15{{'_Cilk_for' loop control variable declared here}}
          k = l = y = z; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
                         //expected-note@-16{{'_Cilk_for' loop control variable declared here}}
        }
        y = x; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
               //expected-note@-18{{'_Cilk_for' loop control variable declared here}}
        x = y; //expected-error{{Modifying the loop control variable inside a '_Cilk_for' has undefined behavior}} \
               //expected-note@-20{{'_Cilk_for' loop control variable declared here}}
      }
    }

    _Cilk_for (int x = 0; x < 10; ++x) {
      _Cilk_for (int y = 0; y < 10; ++y) {
        _Cilk_for (int z = 0; z < x + 10; ++z) { // OK
        }
        _Cilk_for (int z = 0; z < x + 10; z += 2) { // OK
        }
      }
    }

    typedef void (*FuncPtrIntPtr)(int*);
    FuncPtrIntPtr f = &func2;
    f(&i); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
           //expected-note@12{{'_Cilk_for' loop control variable declared here}}

    void (*fp)(int*);
    (*fp)(&i); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
               //expected-note@12{{'_Cilk_for' loop control variable declared here}}
  }
}
