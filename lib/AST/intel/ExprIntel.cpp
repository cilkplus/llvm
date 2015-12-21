//===--- Expr.cpp - Expression AST Node Implementation --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Expr class and subclasses.
//
//===----------------------------------------------------------------------===//

#if INTEL_SPECIFIC_CILKPLUS

#include "clang/AST/APValue.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/LiteralSupport.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <cstring>

using namespace clang;

SourceLocation CilkSpawnExpr::getLocStart() const {
  return TheSpawn->getSpawnStmt()->getLocStart();
}

SourceLocation CilkSpawnExpr::getLocEnd() const {
  return TheSpawn->getSpawnStmt()->getLocEnd();
}

Expr *Expr::IgnoreImpCastsAsWritten() {
  Expr *E = this;
  while (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(E))
    E = ICE->getSubExprAsWritten();
  return E;
}

Expr *Expr::getSubExprAsWritten() {
  Expr *E = this;
  while (true) {
    if (ImplicitCastExpr *ICE = dyn_cast<ImplicitCastExpr>(E))
      E = ICE->getSubExprAsWritten();
    else if (MaterializeTemporaryExpr *MTE
                                        = dyn_cast<MaterializeTemporaryExpr>(E))
      E = MTE->GetTemporaryExpr();
    else if (CXXBindTemporaryExpr *BTE = dyn_cast<CXXBindTemporaryExpr>(E))
      E = BTE->getSubExpr();
    else if (ExprWithCleanups *EWC = dyn_cast<ExprWithCleanups>(E))
      E = EWC->getSubExpr();
    else
      break;
  }

  return E;
}

Expr *Expr::IgnoreImplicitForCilkSpawn() {
  Expr *E = this;
  while (E) {
    E = E->IgnoreParens();
    if (ImplicitCastExpr *CE = dyn_cast<ImplicitCastExpr>(E))
      E = CE->getSubExprAsWritten();
    else if (ExprWithCleanups *EWC = dyn_cast<ExprWithCleanups>(E))
      E = EWC->getSubExpr();
    else if (MaterializeTemporaryExpr *MTE
        = dyn_cast<MaterializeTemporaryExpr>(E))
      E = MTE->GetTemporaryExpr();
    else if (CXXBindTemporaryExpr *BTE = dyn_cast<CXXBindTemporaryExpr>(E))
      E = BTE->getSubExpr();
    else if (CXXConstructExpr *CE = dyn_cast<CXXConstructExpr>(E)) {
      // CXXTempoaryObjectExpr represents a functional cast with != 1 arguments
      // so handle it the same way as CXXFunctionalCastExpr
      if (isa<CXXTemporaryObjectExpr>(CE))
        break;
      if (CE->getNumArgs() >= 1)
        E = CE->getArg(0);
      else
        break;
    } else
      break;
  }

  return E->IgnoreParens();
}

bool Expr::isCilkSpawn() const {
  const Expr *E = IgnoreImplicitForCilkSpawn();
  if (const CallExpr *CE = dyn_cast_or_null<CallExpr>(E))
    return CE->isCilkSpawnCall();

  return false;
}

CEANBuiltinExpr *CEANBuiltinExpr::Create(ASTContext &C,
                                         SourceLocation StartLoc,
                                         SourceLocation EndLoc,
                                         unsigned Rank,
                                         CEANKindType Kind,
                                         ArrayRef<Expr *> Args,
                                         ArrayRef<Expr *> Lengths,
                                         ArrayRef<Stmt *> Vars,
                                         ArrayRef<Stmt *> Increments,
                                         Stmt *Init, Stmt *Body, Expr *Return,
                                         QualType QTy) {
  void *Mem = C.Allocate(sizeof(CEANBuiltinExpr) + 2 * sizeof(Stmt *) +
                         sizeof(Expr *) +
                         sizeof(Expr *) * Args.size() +
                         sizeof(Expr *) * Lengths.size() +
                         sizeof(Stmt *) * Vars.size() +
                         sizeof(Stmt *) * Increments.size(),
                         llvm::alignOf<CEANBuiltinExpr>());
  CEANBuiltinExpr *E = new (Mem) CEANBuiltinExpr(StartLoc, Rank, Args.size(),
                                                 Kind, QTy, EndLoc);
  E->setInit(Init);
  E->setBody(Body);
  E->setReturnExpr(Return);
  E->setArgs(Args);
  E->setLengths(Lengths);
  E->setVars(Vars);
  E->setIncrements(Increments);
  for (ArrayRef<Expr *>::iterator I = Args.begin(), End = Args.end();
       I != End; ++I) {
    if ((*I)->isValueDependent())
      E->setValueDependent(true);
    if ((*I)->isTypeDependent())
      E->setTypeDependent(true);
    if ((*I)->isInstantiationDependent())
      E->setInstantiationDependent(true);
    if ((*I)->containsUnexpandedParameterPack())
      E->setContainsUnexpandedParameterPack();
  }
  return E;
}

CEANBuiltinExpr *CEANBuiltinExpr::CreateEmpty(ASTContext &C, unsigned Rank,
                                              unsigned ArgsSize) {
  void *Mem = C.Allocate(sizeof(CEANBuiltinExpr) + 2 * sizeof(Stmt *) +
                         sizeof(Expr *) +
                         sizeof(Expr *) * ArgsSize +
                         sizeof(Expr *) * Rank +
                         sizeof(Stmt *) * 2 * Rank,
                         llvm::alignOf<CEANBuiltinExpr>());
  return new (Mem) CEANBuiltinExpr(Rank, ArgsSize);
}

void CEANBuiltinExpr::setArgs(ArrayRef<Expr *> Args) {
  assert(Args.size() == ArgsSize &&
         "Number of args is not the same as the preallocated buffer");
  std::copy(Args.begin(), Args.end(), &reinterpret_cast<Expr **>(this + 1)[3]);
}

void CEANBuiltinExpr::setLengths(ArrayRef<Expr *> Lengths) {
  assert(Lengths.size() == Rank &&
         "Number of lengths is not the same as the preallocated buffer");
  std::copy(Lengths.begin(), Lengths.end(),
            &reinterpret_cast<Expr **>(this + 1)[3 + ArgsSize]);
}

void CEANBuiltinExpr::setVars(ArrayRef<Stmt *> Vars) {
  assert(Vars.size() == Rank &&
         "Number of vars is not the same as the preallocated buffer");
  std::copy(Vars.begin(), Vars.end(),
            &reinterpret_cast<Stmt **>(this + 1)[3 + Rank + ArgsSize]);
}

void CEANBuiltinExpr::setIncrements(ArrayRef<Stmt *> Increments) {
  assert(Increments.size() == Rank &&
         "Number of increments is not the same as the preallocated buffer");
  std::copy(Increments.begin(), Increments.end(),
            &reinterpret_cast<Stmt **>(this + 1)[3 + 2 * Rank + ArgsSize]);
}

#endif // INTEL_SPECIFIC_CILKPLUS
