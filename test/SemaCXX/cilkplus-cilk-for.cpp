// RUN: %clang_cc1 -std=c++11 -fcilkplus -fsyntax-only -verify %s

void f1() {
  int j = 0;
  const int k = 10;

  _Cilk_for (auto i = 0; i < 10; ++i); // OK

  _Cilk_for (decltype(j) i = 0; i < 10; ++i); // OK
}

struct Base {
  Base();
  Base(int);
  bool operator<(int) const;
  bool operator>(int) const;
  bool operator<=(int) const;
  bool operator>=(int) const;
  bool operator!=(int) const;
  bool operator==(int) const;
  int operator-(int) const;
  int operator+(int) const;
  Base& operator++();   // prefix
  Base operator++(int); // postfix
};

int operator-(int, const Base &); // expected-note {{candidate function not viable}}
int operator+(int, const Base &);

struct DC : public Base {
  DC();
  DC(int);
};

struct NDC : public Base {
  NDC() = delete; // expected-note {{function has been explicitly marked deleted}}
  NDC(int);
};

struct NoLessThan : public Base {
  NoLessThan();
  NoLessThan(int);
  bool operator<(int) const = delete; // expected-note {{candidate function has been explicitly deleted}}
};

struct NoPreIncrement : public Base {
  NoPreIncrement();
  NoPreIncrement(int);
  NoPreIncrement& operator++() = delete; // expected-note {{candidate function has been explicitly deleted}}
};

// This list is not complete. Some extra operators like
//
// int operator+(int, ...)
// int operator-(int, ...)
//
// may be needed for computing the loop count.
//
void f2() {
  _Cilk_for (DC i; i < 10; ++i); // OK

  _Cilk_for (NDC i; i < 10; ++i); // expected-error {{call to deleted constructor of 'NDC'}}

  _Cilk_for (NoLessThan i; i < 10; ++i); // expected-error {{overload resolution selected deleted operator '<'}}

  _Cilk_for (NoPreIncrement i; i < 10; ++i); // expected-error {{overload resolution selected deleted operator '++'}}
}

struct NoOps : public Base {
  bool operator<(int) const = delete; // expected-note {{candidate function has been explicitly deleted}}
  bool operator>(int) const = delete; // expected-note {{candidate function has been explicitly deleted}}
  bool operator<=(int) const = delete; // expected-note {{candidate function has been explicitly deleted}}
  bool operator>=(int) const = delete; // expected-note {{candidate function has been explicitly deleted}}
  bool operator!=(int) const = delete; // expected-note {{candidate function has been explicitly deleted}}
};

struct BadTy {
  BadTy();
  bool operator<(int) const;
  float operator-(int) const;
  BadTy& operator++();
};

void ops() {
  // Need to check these here, since they create a CXXOperatorCallExpr, rather
  // than a BinaryOperator.
  _Cilk_for (DC i; i < 10; ++i); // OK
  _Cilk_for (NoOps i; i < 10; ++i); // expected-error {{overload resolution selected deleted operator '<'}}

  _Cilk_for (DC i; i <= 10; ++i); // OK
  _Cilk_for (NoOps i; i <= 10; ++i); // expected-error {{overload resolution selected deleted operator '<='}}

  _Cilk_for (DC i; i > 10; ++i); // OK
  _Cilk_for (NoOps i; i > 10; ++i); // expected-error {{overload resolution selected deleted operator '>'}}

  _Cilk_for (DC i; i >= 10; ++i); // OK
  _Cilk_for (NoOps i; i >= 10; ++i); // expected-error {{overload resolution selected deleted operator '>='}}

  _Cilk_for (DC i; i != 10; ++i); // OK
  _Cilk_for (NoOps i; i != 10; ++i);  // expected-error {{overload resolution selected deleted operator '!='}}

  _Cilk_for (DC i; i == 10; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}
  _Cilk_for (NoOps i; i == 10; ++i); // expected-error {{loop condition operator must be one of '<', '<=', '>', '>=', or '!=' in '_Cilk_for'}}

  _Cilk_for (BadTy i; i < 10; ++i); // expected-error {{end - begin must be well-formed in '_Cilk_for'}} \
                                    // expected-error {{invalid operands to binary expression ('int' and 'BadTy')}} \
                                    // expected-note {{loop begin expression here}} \
                                    // expected-note {{loop end expression here}}
}

struct Bool {
  operator bool() const;
};
struct C : public Base { };
Bool operator<(const C&, int);
int operator-(int, const C&);

struct From {
  bool operator++(int);
};
int operator-(int, const From&);
struct To : public Base {
  To();
  To(const From&);
};
bool operator<(const To&, int);

struct ToInt : public Base {
  int operator<(int);
};

struct ToPtr: public Base {
  void* operator<(int);
};

struct ToRef: public Base {
  bool& operator<(int);
};

struct ToCRef: public Base {
  const bool& operator<(int);
};

void conversions() {
  _Cilk_for (C c; c < 5; c++); // expected-warning {{'_Cilk_for' loop count does not respect constructor conversion in loop condition}}
  _Cilk_for (From c; c < 5; c++); // expected-warning {{'_Cilk_for' loop count does not respect user-defined conversion in loop condition}}
  _Cilk_for (ToInt c; c < 5; c++); // OK
  _Cilk_for (ToPtr c; c < 5; c++); // OK
  _Cilk_for (ToRef c; c < 5; c++); // OK
  _Cilk_for (ToCRef c; c < 5; c++); // OK
}

int jump() {
  _Cilk_for (int i = 0; i < 10; ++i) {
    return 0; // expected-error {{cannot return from within a '_Cilk_for' loop}}
  }

  _Cilk_for (int i = 0; i < 10; ++i) {
    []() { return 0; }(); // OK
  }
}
