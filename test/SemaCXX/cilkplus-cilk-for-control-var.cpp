// RUN: %clang_cc1 -std=c++11 -fcilkplus -fsyntax-only -Wall -Wno-unused-variable -verify %s

void func1(const int &i);
void func2(int &i);
void func3(void *i);

template <typename T>
struct Foo {
  Foo(T);
  void operator++(int);
  void operator++();
  bool operator<(const T&);
  void operator+=(const T&);
  void operator-(const T&);
  friend T operator-(const T&, const Foo&);

  void Ptr(Foo *);
  void ConstPtr(const Foo*);
  void Ref(Foo &);
  void ConstRef(const Foo&);
};

template <typename T>
struct P {
  static T *RetPtr();
  static const T *RetConstPtr();
  static T &RetRef();
  static const T &RetConstRef();

  typedef decltype(RetPtr()) Ptr;
  typedef decltype(RetConstPtr()) ConstPtr;
  typedef decltype(RetRef()) Ref;
  typedef decltype(RetConstRef()) ConstRef;
};

void test_control_var_modification_in_body() {
  _Cilk_for (int i = 0; i < 10; ++i) {
    int &r1 = i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}} \
                //expected-note@37{{'_Cilk_for' loop control variable declared here}}

    const int &r2 = i; // OK

    func1(i); // OK

    func2(i); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
              //expected-note@37{{'_Cilk_for' loop control variable declared here}}

    func3(reinterpret_cast<void*>(&i)); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
                                        //expected-note@37{{'_Cilk_for' loop control variable declared here}}

    typedef int *IntPtr;
    typedef const int *ConstIntPtr;
    typedef int &IntRef;
    typedef const int &ConstIntRef;

    IntPtr p1 = &i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}} \
                    //expected-note@37{{'_Cilk_for' loop control variable declared here}}

    ConstIntPtr p2 = &i; // OK

    IntRef r3 = i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}} \
                   //expected-note@37{{'_Cilk_for' loop control variable declared here}}

    ConstIntRef r4 = i; // OK

    _Cilk_for (Foo<int> i = 0; i < 10; ++i) {
      Foo<int> *p1 = &i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}} \
                         //expected-note@66{{'_Cilk_for' loop control variable declared here}}
      const Foo<int> *p2 = &i; // OK

      Foo<int> *p3;
      p3 = &i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}} \
               //expected-note@66{{'_Cilk_for' loop control variable declared here}}

      Foo<int> &r1 = i; //expected-warning{{Modifying the loop control variable 'i' through an alias in '_Cilk_for' has undefined behavior}} \
                        //expected-note@66{{'_Cilk_for' loop control variable declared here}}
      const Foo<int> &r2 = i; // OK
    }

    _Cilk_for (int j = 0; j < 10; ++j) {
      P<int>::Ptr p1 = &j; //expected-warning{{Modifying the loop control variable 'j' through an alias in '_Cilk_for' has undefined behavior}} \
                           //expected-note@80{{'_Cilk_for' loop control variable declared here}}
      P<int>::ConstPtr p2 = &j; // OK
      P<int>::Ptr p3;
      p3 = &j; //expected-warning{{Modifying the loop control variable 'j' through an alias in '_Cilk_for' has undefined behavior}} \
               //expected-note@80{{'_Cilk_for' loop control variable declared here}}

      P<int>::Ref r1 = j; //expected-warning{{Modifying the loop control variable 'j' through an alias in '_Cilk_for' has undefined behavior}} \
                          //expected-note@80{{'_Cilk_for' loop control variable declared here}}
      P<int>::ConstRef r2 = j; // OK
    }
  }

  _Cilk_for (Foo<int> j = 0; j < 10; ++j) {
    j.Ptr(&j); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
               //expected-note@94{{'_Cilk_for' loop control variable declared here}}
    j.ConstPtr(&j); // OK
    j.Ref(j); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
              //expected-note@94{{'_Cilk_for' loop control variable declared here}}
    j.ConstRef(j); // OK

    typedef void (Foo<int>::*FnPtr)(Foo<int>*);
    FnPtr fp = &Foo<int>::Ptr;
    Foo<int> f = 0;
    (f.*fp)(&j); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
                 //expected-note@94{{'_Cilk_for' loop control variable declared here}}

    void (Foo<int>::*FnRef)(Foo<int>&);
    FnRef = &Foo<int>::Ref;
    (f.*FnRef)(j); //expected-warning{{Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior}} \
                   //expected-note@94{{'_Cilk_for' loop control variable declared here}}
  }
}
