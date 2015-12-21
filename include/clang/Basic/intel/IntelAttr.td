
class SourceLocArgument<string name, bit opt = 0> : Argument<name, opt>;
class CheckedExprArgument<string name> : Argument<name, 1>;

class CilkKeyword<string name> : Spelling<name, "CilkKeyword">;
def CilkPlus : LangOpt<"CilkPlus">;

def SIMD : InheritableAttr {
  let Spellings = [];
  let Documentation = [Undocumented];
  int Requires = 2;
}

def SIMDLength : Attr {
  let Spellings = [CilkKeyword<"vectorlength">];
  let Args = [ExprArgument<"ValueExpr">];
  let Documentation = [Undocumented];
  int Requires = 2;
}

def SIMDLinear : Attr {
  let Spellings = [CilkKeyword<"linear">];
  // Array alternates between linear variable and linear step.
  let Args = [VariadicExprArgument<"Items">];
  let AdditionalMembers = [{
    class linear_iterator {
    private:
      items_iterator it;
    public:
      linear_iterator(const SIMDLinearAttr &A, bool step, bool end) {
        it = ((end) ? A.items_end() : A.items_begin()) + step;
      }
      Expr *operator*() { return *it; }
      void operator++() { it += 2; }
      bool operator!=(const linear_iterator &i) const { return (it != i.it); }
    };
    linear_iterator vars_begin() const {
      return linear_iterator(*this, false, false);
    }
    linear_iterator steps_begin() const {
      return linear_iterator(*this, true, false);
    }
    linear_iterator vars_end() const {
      return linear_iterator(*this, false, true);
    }
    linear_iterator steps_end() const {
      return linear_iterator(*this, true, true);
    }
  }];
  let Documentation = [Undocumented];
  int Requires = 2;
}

def SIMDPrivate : Attr {
  let Spellings = [CilkKeyword<"private">];
  let Args = [VariadicExprArgument<"Variables">];
  let Documentation = [Undocumented];
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
  int Requires = 2;
}

def SIMDLastPrivate : Attr {
  let Spellings = [CilkKeyword<"lastprivate">];
  let Args = [VariadicExprArgument<"Variables">];
  let Documentation = [Undocumented];
  int Requires = 2;
}

def SIMDFirstPrivate : Attr {
  let Spellings = [CilkKeyword<"firstprivate">];
  let Args = [VariadicExprArgument<"Variables">];
  let Documentation = [Undocumented];
  int Requires = 2;
}

def SIMDReduction : Attr {
  let Spellings = [CilkKeyword<"reduction">];
  let Args = [VariadicExprArgument<"Variables">];
  let AdditionalMembers = [{
    enum SIMDReductionKind {
      plus = tok::plus,
      star = tok::star,
      minus = tok::minus,
      amp = tok::amp,
      pipe = tok::pipe,
      caret = tok::caret,
      ampamp = tok::ampamp,
      pipepipe = tok::pipepipe,
      max,
      min
    };
    SIMDReductionKind Operator;
  }];
  let Documentation = [Undocumented];
  int Requires = 2;
}

// Cilk Plus elemental function vector() attribute arguments are defined
// as attributes themselves in the dummy _Cilk_elemental scope.
def CilkProcessor : InheritableAttr {
  let Spellings = [CXX11<"_Cilk_elemental", "processor">];
  let Args = [EnumArgument<"Processor", "CilkProcessor",
                           ["<unspecified>",
                            "pentium_4",
                            "pentium_4_sse3",
                            "core_2_duo_ssse3",
                            "core_2_duo_sse4_1",
                            "core_i7_sse4_2",
                            "core_2nd_gen_avx",
                            "core_3rd_gen_avx",
                            "core_4th_gen_avx"],
                           ["Unspecified",
                            "Pentium_4",
                            "Pentium_4_sse3",
                            "Core_2_duo_ssse3",
                            "Core_2_duo_sse4_1",
                            "Core_i7_sse4_2",
                            "Core_2nd_gen_avx",
                            "Core_3rd_gen_avx",
                            "Core_4th_gen_avx"]>,
              SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let AdditionalMembers =
    [{
      static CilkProcessor getValue(llvm::StringRef Processor) {
        return llvm::StringSwitch<CilkProcessor>(Processor)
          .Case("pentium_4",         Pentium_4)
          .Case("pentium_4_sse3",    Pentium_4_sse3)
          .Case("core_2_duo_ssse3",  Core_2_duo_ssse3)
          .Case("core_2_duo_sse4_1", Core_2_duo_sse4_1)
          .Case("core_i7_sse4_2",    Core_i7_sse4_2)
          .Case("core_2nd_gen_avx",  Core_2nd_gen_avx)
          .Case("core_3rd_gen_avx",  Core_3rd_gen_avx)
          .Case("core_4th_gen_avx",  Core_4th_gen_avx)
          .Default(Unspecified);
      }
      static StringRef getProcessorString(CilkProcessor Processor) {
        switch (Processor) {
        case Pentium_4:         return "pentium_4";
        case Pentium_4_sse3:    return "pentium_4_sse3";
        case Core_2_duo_ssse3:  return "core_2_duo_ssse3";
        case Core_2_duo_sse4_1: return "core_2_duo_sse4_1";
        case Core_i7_sse4_2:    return "core_i7_sse4_2";
        case Core_2nd_gen_avx:  return "core_2nd_gen_avx";
        case Core_3rd_gen_avx:  return "core_3rd_gen_avx";
        case Core_4th_gen_avx:  return "core_4th_gen_avx";
        default:
          return "";
        }
      }
      StringRef getName() const {
        return getProcessorString(processor);
      }
      static unsigned getProcessorVectorBytes(CilkProcessor Proc) {
        switch (Proc) {
        case Pentium_4:
        case Pentium_4_sse3:
        case Core_2_duo_ssse3:
        case Core_2_duo_sse4_1:
        case Core_i7_sse4_2:
          return 16;
        case Core_2nd_gen_avx:
        case Core_3rd_gen_avx:
        case Core_4th_gen_avx:
          return 32;
        default:
          return 1;
        }
      }
      unsigned getVectorBytes() const {
        return getProcessorVectorBytes(processor);
      }
    }];
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 2;
}

def CilkVecLength : InheritableAttr {
  let Spellings = [CXX11<"_Cilk_elemental", "vectorlength">];
  let Args = [CheckedExprArgument<"Length">, SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let AdditionalMembers =
    [{
      template <typename SemaMemberPtr>
      Expr *CheckArgument(Sema &S, Expr *E, SemaMemberPtr Fn) const {
        return (S.*Fn)(E);
      }
    }];
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 2;
}

def CilkLinear : InheritableAttr {
  let Spellings = [CXX11<"_Cilk_elemental", "linear">];
  let Args = [IdentifierArgument<"Parameter">,
              SourceLocArgument<"ParameterLoc">,
              CheckedExprArgument<"StepExpr">,
              SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let AdditionalMembers =
    [{
      IdentifierInfo *getStepParameter() const {
        DeclRefExpr *E = dyn_cast_or_null<DeclRefExpr>(getStepExpr());
        return E ? E->getDecl()->getIdentifier() : 0;
      }
      SourceLocation getStepLoc() const {
        Expr *E = getStepExpr();
        return E ? E->getExprLoc() : SourceLocation();
      }
      template <typename SemaMemberPtr>
        Expr *CheckArgument(Sema &S, Expr *E, SemaMemberPtr Fn) const {
        return (S.*Fn)(E);
      }
    }];
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 2;
}

def CilkAligned : InheritableAttr {
  let Spellings = [CXX11<"_Cilk_elemental", "aligned1">];
//  let Accessors = Accessor<"CilkPlus", [Keyword<"_Alignas">]>,
//  let LangOpts = [CilkPlus];
  let Args = [IdentifierArgument<"Parameter">,
              SourceLocArgument<"ParameterLoc">,
              CheckedExprArgument<"AlignedExpr">,
              SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let AdditionalMembers =
    [{
      IdentifierInfo *getAlignedParameter() const {
        DeclRefExpr *E = dyn_cast_or_null<DeclRefExpr>(getAlignedExpr());
        return E ? E->getDecl()->getIdentifier() : 0;
      }
      SourceLocation getAlignedLoc() const {
        Expr *E = getAlignedExpr();
        return E ? E->getExprLoc() : SourceLocation();
      }
      template <typename SemaMemberPtr>
        Expr *CheckArgument(Sema &S, Expr *E, SemaMemberPtr Fn) const {
        return (S.*Fn)(E);
      }
    }];
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 2;
}

def CilkUniform : InheritableAttr {
  let Spellings = [CXX11<"_Cilk_elemental", "uniform">];
  let Args = [IdentifierArgument<"Parameter">,
              SourceLocArgument<"ParameterLoc">,
              SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 2;
}

def CilkMask : InheritableAttr {
  let Spellings = [CXX11<"_Cilk_elemental", "mask">];
  let Args = [BoolArgument<"Mask">, SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 2;
}

def CilkNoMask : InheritableAttr {
  let Spellings = [CXX11<"_Cilk_elemental", "nomask">];
  let Args = [SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
  int Requires = 2;
}

def CilkElemental : InheritableAttr {
  let Spellings = [Declspec<"vector">,
                   GNU<"vector">,
                   CXX11<"", "vector", 200809>];
  let Args = [SourceLocArgument<"Group">];
  let Subjects = SubjectList<[Function]>;
  let TemplateDependent = 1;
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 2;
}

def AvoidFalseShare : InheritableAttr {
  let Spellings = [Declspec<"avoid_false_share">];
  let Args = [StringArgument<"ExceptionId">];
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 4;
}

def Allocate : InheritableAttr {
  let Spellings = [GNU<"allocate">];
  let Args = [StringArgument<"Name">];
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 4;
}

def GCCStruct : InheritableAttr {
  let Spellings = [GNU<"gcc_struct">];
  let Subjects = SubjectList<[Record]>;
  let HasCustomParsing = 1;
  let Documentation = [Undocumented];
  int Requires = 4;
}

def BNDLegacy : InheritableAttr {
  let Spellings = [GNU<"bnd_legacy">];
  let Subjects = SubjectList<[Function]>;
  let Documentation = [Undocumented];
  int Requires = 4;
}

def BNDVarSize : InheritableAttr {
  let Spellings = [GNU<"bnd_variable_size">];
  let Subjects = SubjectList<[Field], WarnDiag,
                             "ExpectedFieldOrGlobalVar">;
  let Documentation = [Undocumented];
  int Requires = 4;
}

def SIMDAssert : Attr {
  let Spellings = [CilkKeyword<"assert">];
  let Documentation = [Undocumented];
  int Requires = 6;
}
