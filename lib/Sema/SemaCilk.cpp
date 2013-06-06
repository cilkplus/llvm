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
    case SIMDReductionAttr::star:
    case SIMDReductionAttr::minus:
    case SIMDReductionAttr::amp:
    case SIMDReductionAttr::pipe:
    case SIMDReductionAttr::caret:
    case SIMDReductionAttr::ampamp:
    case SIMDReductionAttr::pipepipe:
      break;
    }
  }

  SIMDReductionAttr *ReductionAttr = ::new SIMDReductionAttr(
      ReductionLoc, Context, VarList.data(), VarList.size());
  ReductionAttr->Operator = Operator;

  return AttrResult(ReductionAttr);
}
