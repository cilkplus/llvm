// RUN: %clang_cc1 -std=c++11 -fcilkplus -fsyntax-only -verify %s
//
// XFAIL: *
//
void f1() {
  int j = 0;
  const int k = 10;

  _Cilk_for (j = 0; j < 10; ++j); // expected-error {{loop initialization must be a declaration in '_Cilk_for'}}

  _Cilk_for (decltype(k) i = 0; i < 10; ++i); // expected-error {{loop control variable cannot be 'const' in '_Cilk_for'}}

  // The following two tests could be valid.
  _Cilk_for (int &i = j; i < 10; ++i); // expected-error {{loop control variable may not have a reference type in '_Cilk_for'}}

  _Cilk_for (int &&i = j; i < 10; ++i); // expected-error {{loop control variable may not have a reference type in '_Cilk_for'}}
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

int operator-(int, const Base &);
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

struct NoCopyCtor : public Base {
  NoCopyCtor();
  NoCopyCtor(int);
  NoCopyCtor(const NoCopyCtor &) = delete; // expected-note {{function has been explicitly marked deleted here}}
};

// This list is not complete. Some extra operators like
//
// int operator+(int, ...)
// int operator-(int, ...)
//
// may be needed for computing the loop count.
//
void f2() {
  _Cilk_for (NoCopyCtor i; i < 10; ++i); // expected-error {{call to deleted constructor of 'NoCopyCtor'}}
}
