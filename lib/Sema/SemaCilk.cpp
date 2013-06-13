//===--- SemaCilk.cpp - Semantic Analysis for Cilk Plus -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief This file implements Cilk Plus related semantic analysis.
///
//===----------------------------------------------------------------------===//
#include "clang/Sema/Lookup.h"
#include "clang/Sema/SemaInternal.h"

using namespace clang;
using namespace sema;

AttrResult Sema::ActOnPragmaSIMDLength(SourceLocation VectorLengthLoc,
                                       Expr *VectorLengthExpr) {
  /* Perform checks that Expr is a constant, power of 2 */
  if (VectorLengthExpr->getType().isNull())
    return AttrError();

  llvm::APSInt Constant;
  Constant.setIsUnsigned(true);

  if (!VectorLengthExpr->EvaluateAsInt(Constant, Context)) {
    Diag(VectorLengthExpr->getLocStart(),
         diag::err_pragma_simd_invalid_vectorlength_expr) << 0;
    return AttrError();
  }
  if (!Constant.isPowerOf2()) {
    Diag(VectorLengthExpr->getLocStart(),
         diag::err_pragma_simd_invalid_vectorlength_expr) << 1;
    return AttrError();
  }

  ExprResult E = Owned(IntegerLiteral::Create(
      Context, Constant, Context.getCanonicalType(VectorLengthExpr->getType()),
      VectorLengthExpr->getLocStart()));
  if (E.isInvalid())
    return AttrError();

  return AttrResult(::new (Context)
                    SIMDLengthAttr(VectorLengthLoc, Context, E.get()));
}

AttrResult Sema::ActOnPragmaSIMDLengthFor(SourceLocation VectorLengthForLoc,
                                          SourceLocation TypeLoc,
                                          QualType &VectorLengthForType) {
  if (VectorLengthForType.isNull())
    return AttrError();

  QualType Ty = VectorLengthForType.getCanonicalType();
  if (Ty->isVoidType()) {
    Diag(TypeLoc, diag::err_simd_for_length_for_void);
    return AttrError();
  }
  if (Ty->isIncompleteType()) {
    Diag(TypeLoc, diag::err_simd_for_length_for_incomplete) << Ty;
    return AttrError();
  }
  return AttrResult(::new (Context) SIMDLengthForAttr(
      VectorLengthForLoc, Context, VectorLengthForType));
}

ExprResult Sema::ActOnPragmaSIMDLinearVariable(CXXScopeSpec SS,
                                               DeclarationNameInfo Name) {
  // linear-variable must be a variable with scalar type
  LookupResult Lookup(*this, Name, LookupOrdinaryName);
  LookupParsedName(Lookup, CurScope, &SS, /*AllowBuiltinCreation*/false);
  if (Lookup.isAmbiguous())
    return ExprError();

  if (Lookup.empty()) {
    Diag(Name.getLoc(), diag::err_undeclared_var_use) << Name.getName();
    return ExprError();
  }

  VarDecl *VD = 0;
  if (!(VD = Lookup.getAsSingle<VarDecl>())) {
    Diag(Name.getLoc(), diag::err_pragma_simd_invalid_linear_var);
    return ExprError();
  }

  QualType VarType = VD->getType();
  if (!VarType->isScalarType()) {
    Diag(Name.getLoc(), diag::err_pragma_simd_invalid_linear_var);
    return ExprError();
  }

  bool RefersToEnclosingScope = (CurContext != VD->getDeclContext() &&
                                 VD->getDeclContext()->isFunctionOrMethod());
  DeclRefExpr *E =
      DeclRefExpr::Create(Context, NestedNameSpecifierLoc(), SourceLocation(),
                          VD, RefersToEnclosingScope, Name,
                          VD->getType().getNonReferenceType(), VK_LValue);

  return Owned(E);
}

AttrResult Sema::ActOnPragmaSIMDLinear(SourceLocation LinearLoc,
                                       ArrayRef<Expr *> Exprs) {
  for (unsigned i = 1, e = Exprs.size(); i < e; i += 2) {
    // linear-step must be either an integer constant expression,
    // or be a reference to a variable with integral type.
    if (Exprs[i]) {
      Expr *Step = Exprs[i];
      llvm::APSInt Constant;
      if (!Step->EvaluateAsInt(Constant, Context)) {
        if (!isa<DeclRefExpr>(Step)) {
          Diag(Step->getLocStart(), diag::err_pragma_simd_invalid_linear_step);
          return AttrError();
        }
        QualType SQT = Step->getType();
        if (!SQT->isIntegralType(Context)) {
          Diag(Step->getLocStart(), diag::err_pragma_simd_invalid_linear_step);
          return AttrError();
        }
      }
      if (!Constant.getBoolValue())
        Diag(Step->getLocStart(), diag::warn_pragma_simd_linear_expr_zero);
    }
  }
  return AttrResult(::new (Context) SIMDLinearAttr(
      LinearLoc, Context, const_cast<Expr **>(Exprs.data()), Exprs.size()));
}

ExprResult Sema::ActOnPragmaSIMDPrivateVariable(CXXScopeSpec SS,
                                                DeclarationNameInfo Name,
                                                SIMDPrivateKind Kind) {
  SourceLocation NameLoc = Name.getLoc();
  StringRef KindName = (Kind == SIMD_Private) ? "private" :
                   (Kind == SIMD_LastPrivate) ? "lastprivate" : "firstprivate";

  LookupResult Lookup(*this, Name, LookupOrdinaryName);
  LookupParsedName(Lookup, CurScope, &SS, /*AllowBuiltinCreation*/false);
  if (Lookup.isAmbiguous())
    return ExprError();

  if (Lookup.empty()) {
    Diag(NameLoc, diag::err_undeclared_var_use) << Name.getName();
    return ExprError();
  }

  VarDecl *VD = 0;
  if (!(VD = Lookup.getAsSingle<VarDecl>())) {
    Diag(NameLoc, diag::err_pragma_simd_invalid_private_var) << KindName;
    return ExprError();
  }

  QualType VarType = VD->getType().getCanonicalType();

  // A variable that appears in a {first, last} private clause must not have an
  // incomplete type or a reference type.
  if (VarType->isIncompleteType()) {
    Diag(NameLoc, diag::err_pragma_simd_var_incomplete)
      << KindName << VarType;
    return ExprError();
  }

  if (VarType->isReferenceType()) {
    Diag(NameLoc, diag::err_pragma_simd_var_reference)
      << KindName << VarType;
    return ExprError();
  }

  // A variable that appears in a private or lastprivate clause must not have
  // a const-qualified type unless it is of class type with a mutable member.
  // This restriction does not apply to the firstprivate clause.
  if (VarType.isConstQualified() && (Kind != SIMD_FirstPrivate)) {
    bool HasMutableMember = false;

    if (getLangOpts().CPlusPlus && VarType->isRecordType()) {
      const RecordDecl *RD = VarType->getAs<RecordType>()->getDecl();
      assert(RD && "null RecordDecl unexpected");
      if (const CXXRecordDecl *Class = dyn_cast<CXXRecordDecl>(RD))
        HasMutableMember = Class->hasMutableFields();
    }

    if (!HasMutableMember) {
      Diag(NameLoc, diag::err_pragma_simd_var_const) << KindName;
      Diag(VD->getLocation(), diag::note_pragma_simd_var);
      return ExprError();
    }
  }

  QualType ElementTy = VarType;
  if (VarType->isArrayType())
    ElementTy = VarType->getAsArrayTypeUnsafe()->getElementType();

  CXXRecordDecl *Class = 0;
  if (getLangOpts().CPlusPlus && ElementTy->isRecordType()) {
    RecordDecl *RD = ElementTy->getAs<RecordType>()->getDecl();
    assert(RD && "null RecordDecl unexpected");
    if (isa<CXXRecordDecl>(RD))
      Class = cast<CXXRecordDecl>(RD)->getDefinition();
  }

  enum {
    DefaultConstructor = 0,
    CopyConstructor,
    CopyAssignment
  };

  // A variable of class type (or array thereof) that appears in a private
  // clause requires an accessible, unambiguous default constructor for the
  // class type.
  //
  // A variable of class type (or array thereof) that appears in a lastprivate
  // clause requires an accessible, unambiguous default constructor for the
  // class type, unless the list item is also specified in a firstprivate
  // clause.
  if (Class && (Kind == SIMD_Private || Kind == SIMD_LastPrivate)) {
    SpecialMemberOverloadResult *Result =
      LookupSpecialMember(Class, CXXDefaultConstructor, /*ConstArg=*/false,
                          /*VolatileThis=*/false, /*RValueThis*/false,
                          /*ConstThis*/false, /*VolatileThis*/false);

    switch (Result->getKind()) {
    case SpecialMemberOverloadResult::NoMemberOrDeleted:
      Diag(NameLoc, diag::err_pragma_simd_var_no_member) << KindName
        << DefaultConstructor << ElementTy;
      return ExprError();
    case SpecialMemberOverloadResult::Ambiguous:
      Diag(NameLoc, diag::err_pragma_simd_var_ambiguous_member)
        << KindName << DefaultConstructor << ElementTy;
      return ExprError();
    default:
      // Access check will be deferred until its first use.
      ;
    }
  }

  // A variable of class type (or array thereof) that appears in a firstprivate
  // clause requires an accessible, unambiguous copy constructor for the
  // class type.
  if (Class && (Kind == SIMD_FirstPrivate)) {
    SpecialMemberOverloadResult *Result =
      LookupSpecialMember(Class, CXXCopyConstructor, /*ConstArg=*/false,
                          /*VolatileThis=*/false, /*RValueThis*/false,
                          /*ConstThis*/false, /*VolatileThis*/false);

    switch (Result->getKind()) {
    case SpecialMemberOverloadResult::NoMemberOrDeleted:
      Diag(NameLoc, diag::err_pragma_simd_var_no_member) << KindName
        << CopyConstructor << ElementTy;
      return ExprError();
    case SpecialMemberOverloadResult::Ambiguous:
      Diag(NameLoc, diag::err_pragma_simd_var_ambiguous_member)
        << KindName << CopyConstructor << ElementTy;
      return ExprError();
    default:
      // Access check will be deferred until its first use.
      ;
    }
  }

  // A variable of class type (or array thereof) that appears in a lastprivate
  // clause requires an accessible, unambiguous copy assignment operator for the
  // class type.
  if (Class && (Kind == SIMD_LastPrivate)) {
    SpecialMemberOverloadResult *Result =
      LookupSpecialMember(Class, CXXCopyAssignment, /*ConstArg=*/false,
                          /*VolatileThis=*/false, /*RValueThis*/false,
                          /*ConstThis*/false, /*VolatileThis*/false);

    switch (Result->getKind()) {
    case SpecialMemberOverloadResult::NoMemberOrDeleted:
      Diag(NameLoc, diag::err_pragma_simd_var_no_member) << KindName
        << CopyAssignment << ElementTy;
      return ExprError();
    case SpecialMemberOverloadResult::Ambiguous:
      Diag(NameLoc, diag::err_pragma_simd_var_ambiguous_member)
        << KindName << CopyAssignment << ElementTy;
      return ExprError();
    default:
      // Access check will be deferred until its first use.
      ;
    }
  }

  // FIXME: Should just return this VarDecl rather than a DeclRefExpr, which
  // is a ODR-use of this declaration.
  return BuildDeclRefExpr(VD, VarType, VK_LValue, NameLoc);
}

AttrResult Sema::ActOnPragmaSIMDPrivate(SourceLocation ClauseLoc,
                                        llvm::MutableArrayRef<Expr *> Exprs,
                                        SIMDPrivateKind Kind) {
  Attr *Clause = 0;
  switch (Kind) {
  case SIMD_Private:
    Clause = ::new (Context) SIMDPrivateAttr(ClauseLoc, Context,
                                             Exprs.data(), Exprs.size());
    break;
  case SIMD_LastPrivate:
    Clause = ::new (Context) SIMDLastPrivateAttr(ClauseLoc, Context,
                                                 Exprs.data(), Exprs.size());
    break;
  case SIMD_FirstPrivate:
    Clause = ::new (Context) SIMDFirstPrivateAttr(ClauseLoc, Context,
                                                  Exprs.data(), Exprs.size());
    break;
  }

  return AttrResult(Clause);
}

AttrResult
Sema::ActOnPragmaSIMDReduction(SourceLocation ReductionLoc,
                               SIMDReductionAttr::SIMDReductionKind Operator,
                               llvm::MutableArrayRef<Expr *> VarList) {
  for (unsigned i = 0, e = VarList.size(); i < e; ++i) {
    Expr *E = VarList[i];
    const ValueDecl *VD = 0;
    if (const DeclRefExpr *D = dyn_cast<DeclRefExpr>(E)) {
      VD = D->getDecl();
    } else if (const MemberExpr *M = dyn_cast<MemberExpr>(E)) {
      VD = M->getMemberDecl();
    }

    if (!VD || VD->isFunctionOrFunctionTemplate()) {
      Diag(E->getLocStart(), diag::err_pragma_simd_reduction_invalid_var);
      return AttrError();
    }

    QualType QT = VD->getType().getCanonicalType();

    // Arrays may not appear in a reduction clause
    if (QT->isArrayType()) {
      Diag(E->getLocStart(), diag::err_pragma_simd_var_array) << "reduction";
      Diag(VD->getLocation(), diag::note_declared_at);
      return AttrError();
    }

    // An item that appears in a reduction clause must not be const-qualified.
    if (QT.isConstQualified()) {
      Diag(E->getLocStart(), diag::err_pragma_simd_var_const) << "reduction";
      Diag(VD->getLocation(), diag::note_declared_at);
      return AttrError();
    }

    BinaryOperatorKind ReductionOp;
    switch (Operator) {
    case SIMDReductionAttr::max:
    case SIMDReductionAttr::min:
      // For a max or min reduction in C/C++, the type of the list item must
      // be an allowed arithmetic data type
      if (!QT->isArithmeticType()) {
        Diag(E->getLocStart(), diag::err_pragma_simd_reduction_maxmin)
            << (Operator == SIMDReductionAttr::max);
        return AttrError();
      }
      break;
    case SIMDReductionAttr::plus:
      ReductionOp = BO_AddAssign;
      break;
    case SIMDReductionAttr::star:
      ReductionOp = BO_MulAssign;
      break;
    case SIMDReductionAttr::minus:
      ReductionOp = BO_SubAssign;
      break;
    case SIMDReductionAttr::amp:
      ReductionOp = BO_AndAssign;
      break;
    case SIMDReductionAttr::pipe:
      ReductionOp = BO_OrAssign;
      break;
    case SIMDReductionAttr::caret:
      ReductionOp = BO_XorAssign;
      break;
    case SIMDReductionAttr::ampamp:
      ReductionOp = BO_LAnd;
      break;
    case SIMDReductionAttr::pipepipe:
      ReductionOp = BO_LAnd;
      break;
    }
    if (Operator != SIMDReductionAttr::max &&
        Operator != SIMDReductionAttr::min && getLangOpts().CPlusPlus &&
        QT->isRecordType()) {
      // Test that the given operator is overloaded for this type
      if (BuildBinOp(getCurScope(), E->getLocStart(), ReductionOp, E, E)
              .isInvalid()) {
        return AttrError();
      }
    }
  }

  SIMDReductionAttr *ReductionAttr = ::new SIMDReductionAttr(
      ReductionLoc, Context, VarList.data(), VarList.size());
  ReductionAttr->Operator = Operator;

  return AttrResult(ReductionAttr);
}

namespace {
struct UsedDecl {
  // If this is allowed to conflict (eg. a linear step)
  bool CanConflict;
  // Attribute using this variable
  const Attr *Attribute;
  // Specific usage within that attribute
  const Expr *Usage;

  UsedDecl(bool CanConflict, const Attr *Attribute, const Expr *Usage)
      : CanConflict(CanConflict), Attribute(Attribute), Usage(Usage) {}
};
typedef llvm::SmallDenseMap<const ValueDecl *,
                            llvm::SmallVector<UsedDecl, 4> > DeclMapTy;

void HandleSIMDLinearAttr(const Attr *A, DeclMapTy &UsedDecls) {
  const SIMDLinearAttr *LA = static_cast<const SIMDLinearAttr *>(A);
  for (Expr **i = LA->steps_begin(), **e = LA->steps_end(); i < e; i += 2) {
    const Expr *LE = *i;
    if (const DeclRefExpr *D = dyn_cast_or_null<DeclRefExpr>(LE)) {
      const ValueDecl *VD = D->getDecl();
      UsedDecls[VD].push_back(UsedDecl(false, A, D));
    }
    const Expr *SE = *(i+1);
    if (const DeclRefExpr *D = dyn_cast_or_null<DeclRefExpr>(SE)) {
      const ValueDecl *VD = D->getDecl();
      UsedDecls[VD].push_back(UsedDecl(true, LA, D));
    }
  }
}

void HandleGenericSIMDAttr(const Attr *A, DeclMapTy &UsedDecls,
                           bool CanConflict = false) {
  const SIMDReductionAttr *RA = static_cast<const SIMDReductionAttr *>(A);
  for (Expr **i = RA->variables_begin(), **e = RA->variables_end(); i < e; ++i) {
    const Expr *RE = *i;
    if (const DeclRefExpr *D = dyn_cast_or_null<DeclRefExpr>(RE)) {
      const ValueDecl *VD = D->getDecl();
      UsedDecls[VD].push_back(UsedDecl(CanConflict, A, D));
    } else if (const MemberExpr *M = dyn_cast_or_null<MemberExpr>(RE)) {
      const ValueDecl *VD = M->getMemberDecl();
      UsedDecls[VD].push_back(UsedDecl(CanConflict, A, M));
    }
  }
}

void EnforcePragmaSIMDConstraints(DeclMapTy &UsedDecls, Sema &S) {
  for (DeclMapTy::iterator it = UsedDecls.begin(), end = UsedDecls.end();
       it != end; it++) {
    llvm::SmallVectorImpl<UsedDecl> &Usages = it->second;
    if (Usages.size() <= 1)
      continue;

    UsedDecl &First = Usages[0]; // Legitimate usage of this variable
    for (unsigned i = 1, e = Usages.size(); i < e; ++i) {
      UsedDecl &Use = Usages[i];
      if (First.CanConflict && Use.CanConflict)
        continue;
      // One is in conflict. Issue error on i'th usage.
      switch (Use.Attribute->getKind()) {
      case attr::SIMDLinear:
        if (Use.CanConflict || (First.CanConflict &&
                                First.Attribute->getKind() == attr::SIMDLinear))
          // This is a linear step, or in conflict with a linear step
          S.Diag(Use.Usage->getLocStart(), diag::err_pragma_simd_conflict_step)
            << !Use.CanConflict << Use.Usage->getSourceRange();
        else
          // Re-using linear variable in another simd clause
          S.Diag(Use.Usage->getLocStart(), diag::err_pragma_simd_reuse_var)
            << 0 << Use.Usage->getSourceRange();
        break;
      case attr::SIMDPrivate:
      case attr::SIMDFirstPrivate:
      case attr::SIMDLastPrivate:
        S.Diag(Use.Usage->getLocStart(), diag::err_pragma_simd_reuse_var)
          << (First.Attribute->getKind() == attr::SIMDReduction)
          << Use.Usage->getSourceRange();
        break;
      case attr::SIMDReduction:
        // Any number of reduction clauses can be specified on the directive,
        // but a list item can appear only once in the reduction clauses for
        // that directive.
        S.Diag(Use.Usage->getLocStart(), diag::err_pragma_simd_reuse_var)
          << 1 << Use.Usage->getSourceRange();
        break;
      default:
        ;
      }
      S.Diag(First.Usage->getLocStart(), diag::note_pragma_simd_used_here)
        << First.Attribute->getRange() << First.Usage->getSourceRange();
    }
  }
}
} // namespace

void Sema::CheckSIMDPragmaClauses(SourceLocation PragmaLoc,
                                  ArrayRef<Attr *> Attrs) {
  // Collect all used decls in order of usage
  DeclMapTy UsedDecls;
  const Attr *VectorLengthAttr = 0;
  for (unsigned i = 0, e = Attrs.size(); i < e; ++i) {
    if (!Attrs[i])
      continue;
    const Attr *A = Attrs[i];

    switch(A->getKind()) {
    case attr::SIMDLinear:
      HandleSIMDLinearAttr(A, UsedDecls);
      break;
    case attr::SIMDLength:
    case attr::SIMDLengthFor:
      if (VectorLengthAttr) {
        attr::Kind ThisKind = A->getKind(),
                   PrevKind = VectorLengthAttr->getKind();

        if (ThisKind == PrevKind)
          Diag(A->getLocation(), diag::err_pragma_simd_vectorlength_multiple)
            << (ThisKind == attr::SIMDLength) << A->getRange();
        else
          Diag(A->getLocation(), diag::err_pragma_simd_vectorlength_conflict)
              << (ThisKind == attr::SIMDLength) << A->getRange();
        Diag(VectorLengthAttr->getLocation(),
             diag::note_pragma_simd_specified_here)
            << (PrevKind == attr::SIMDLength) << VectorLengthAttr->getRange();
      } else
        VectorLengthAttr = A;
      break;
    case attr::SIMDFirstPrivate:
    case attr::SIMDLastPrivate:
    case attr::SIMDPrivate:
      HandleGenericSIMDAttr(A, UsedDecls, true);
      break;
    case attr::SIMDReduction:
      HandleGenericSIMDAttr(A, UsedDecls);
      break;
    default:
      ;
    }
  }
  EnforcePragmaSIMDConstraints(UsedDecls, *this);
}

#define ADD_SIMD_VAR_LIST(ATTR, FUNC)                                          \
do {                                                                           \
  const ATTR *PA = static_cast<const ATTR *>(A);                               \
  for (Expr **I = PA->variables_begin(), **E = PA->variables_end();            \
    I != E; ++I) {                                                             \
    Expr *DE = *I;                                                             \
    assert (DE && isa<DeclRefExpr>(DE) && "reference to a variable expected"); \
    VarDecl *VD = cast<VarDecl>(cast<DeclRefExpr>(DE)->getDecl());             \
    getCurSIMDFor()->FUNC(VD);                                                 \
  }                                                                            \
} while (0)

void Sema::ActOnStartOfSIMDForStmt(SourceLocation PragmaLoc, Scope *CurScope,
                                   ArrayRef<Attr *> SIMDAttrList) {
  CheckSIMDPragmaClauses(PragmaLoc, SIMDAttrList);

  CapturedDecl *CD = 0;
  RecordDecl *RD = CreateCapturedStmtRecordDecl(CD, PragmaLoc, /*NumArgs*/1);

  PushSIMDForScope(CurScope, CD, RD, PragmaLoc);

  // Process all variables in the clauses.
  for (unsigned I = 0, N = SIMDAttrList.size(); I < N; ++I) {
    const Attr *A = SIMDAttrList[I];
    if (!A)
      continue;
    switch(A->getKind()) {
    case attr::SIMDPrivate:
      ADD_SIMD_VAR_LIST(SIMDPrivateAttr, addPrivateVar);
      break;
    case attr::SIMDLastPrivate:
      ADD_SIMD_VAR_LIST(SIMDLastPrivateAttr, addLastPrivateVar);
      break;
    case attr::SIMDFirstPrivate:
      ADD_SIMD_VAR_LIST(SIMDFirstPrivateAttr, addFirstPrivateVar);
      break;
    case attr::SIMDReduction:
#if 0
      // TODO: Need to properly handle Reductions, in case of MemberExpr
      // rather than a DeclRefExpr
      ADD_SIMD_VAR_LIST(SIMDReductionAttr, addReductionVar);
#endif
      break;
    case attr::SIMDLinear: {
      const SIMDLinearAttr *LA = static_cast<const SIMDLinearAttr *>(A);
      for (Expr **I = LA->steps_begin(), **E = LA->steps_end();
           I != E; I += 2) {
        Expr *DE = *I;
        assert (DE && isa<DeclRefExpr>(DE) && "reference to a variable expected");
        VarDecl *VD = cast<VarDecl>(cast<DeclRefExpr>(DE)->getDecl());
        getCurSIMDFor()->addLinearVar(VD);
      }
      break;
    }
    default:
      ;
    }
  }

  // Push a compound scope for the body.
  PushCompoundScope();

  if (CurScope)
    PushDeclContext(CurScope, CD);
  else
    CurContext = CD;

  PushExpressionEvaluationContext(PotentiallyEvaluated);
}

void Sema::ActOnSIMDForStmtError() {
  DiscardCleanupsInEvaluationContext();
  PopExpressionEvaluationContext();

  PopDeclContext();

  SIMDForScopeInfo *FSI = getCurSIMDFor();
  RecordDecl *Record = FSI->TheRecordDecl;
  Record->setInvalidDecl();

  SmallVector<Decl*, 4> Fields;
  for (RecordDecl::field_iterator I = Record->field_begin(),
                                  E = Record->field_end(); I != E; ++I)
    Fields.push_back(*I);
  ActOnFields(/*Scope=*/0, Record->getLocation(), Record, Fields,
              SourceLocation(), SourceLocation(), /*AttributeList=*/0);

  // Pop the compound scope we inserted implicitly.
  PopCompoundScope();
  PopFunctionScopeInfo();
}
