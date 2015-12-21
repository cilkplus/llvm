//===--- Decl.cpp - Declaration AST Node Implementation -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Decl subclasses.
//
//===----------------------------------------------------------------------===//

#if INTEL_SPECIFIC_CILKPLUS
#include "clang/AST/Decl.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/ASTMutationListener.h"
#include "clang/AST/Attr.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/TypeLoc.h"
#include "clang/Basic/Builtins.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/Module.h"
#include "clang/Basic/Specifiers.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "llvm/Support/ErrorHandling.h"
#include "clang/Basic/intel/DeclIntel.h"
#include <algorithm>

using namespace clang;


CilkSpawnDecl::CilkSpawnDecl(DeclContext *DC, CapturedStmt *Spawn) :
  Decl(CilkSpawn, DC, Spawn->getLocStart()), CapturedSpawn(Spawn) {
}

CilkSpawnDecl *CilkSpawnDecl::Create(ASTContext &C, DeclContext *DC,
                                     CapturedStmt *Spawn) {
  return new (C, DC) CilkSpawnDecl(DC, Spawn);
}

CilkSpawnDecl *CilkSpawnDecl::CreateDeserialized(ASTContext &C, unsigned ID) {
  return new (C, ID) CilkSpawnDecl(nullptr, nullptr);
}

Stmt *CilkSpawnDecl::getSpawnStmt() {
  return getCapturedStmt()->getCapturedStmt();
}

bool CilkSpawnDecl::hasReceiver() const {
  const Stmt *S = getSpawnStmt();
  assert(S && "null spawn statement");
  return isa<DeclStmt>(S);
}

VarDecl *CilkSpawnDecl::getReceiverDecl() const {
  Stmt *S = const_cast<Stmt *>(getSpawnStmt());
  assert(S && "null spawn statement");
  if (DeclStmt *DS = dyn_cast<DeclStmt>(S)) {
    assert(DS->isSingleDecl() && "single declaration expected");
    return cast<VarDecl>(DS->getSingleDecl());
  }

  return 0;
}

bool Decl::isSpawning() const {
  if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(this))
    return FD->isSpawning();
  else if (const CapturedDecl *CD = dyn_cast<CapturedDecl>(this))
    return CD->isSpawning();

  return false;
}

#endif // INTEL_SPECIFIC_CILKPLUS
