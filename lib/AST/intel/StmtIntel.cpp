//===--- Stmt.cpp - Statement AST Node Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Stmt class and statement subclasses.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTDiagnostic.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprObjC.h"
#include "clang/AST/ExprOpenMP.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/intel/StmtIntel.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtObjC.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/AST/Type.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Lex/Token.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

#if INTEL_SPECIFIC_CILKPLUS
CilkForGrainsizeStmt::CilkForGrainsizeStmt(Expr *Grainsize, Stmt *CilkFor,
                                           SourceLocation LocStart)
    : Stmt(CilkForGrainsizeStmtClass), LocStart(LocStart) {
  SubExprs[GRAINSIZE] = Grainsize;
  SubExprs[CILK_FOR] = CilkFor;
}

CilkForGrainsizeStmt::CilkForGrainsizeStmt(EmptyShell Empty)
    : Stmt(CilkForGrainsizeStmtClass), LocStart() {
  SubExprs[GRAINSIZE] = 0;
  SubExprs[CILK_FOR] = 0;
}

/// \brief Construct an empty Cilk for statement.
CilkForStmt::CilkForStmt(EmptyShell Empty)
    : Stmt(CilkForStmtClass, Empty), LoopControlVar(0), InnerLoopControlVar(0),
      InnerLoopVarAdjust(0) {
  SubExprs[INIT] = 0;
  SubExprs[COND] = 0;
  SubExprs[INC] = 0;
  SubExprs[BODY] = 0;
  SubExprs[LOOP_COUNT] = 0;
}

/// \brief Construct a Cilk for statement.
CilkForStmt::CilkForStmt(Stmt *Init, Expr *Cond, Expr *Inc, CapturedStmt *Body,
                         Expr *LoopCount, SourceLocation FL, SourceLocation LP,
                         SourceLocation RP)
    : Stmt(CilkForStmtClass), CilkForLoc(FL), LParenLoc(LP), RParenLoc(RP),
      LoopControlVar(0), InnerLoopControlVar(0), InnerLoopVarAdjust(0) {
  assert(Init && Cond && Inc && Body && "null argument unexpected");

  SubExprs[INIT] = Init;
  SubExprs[COND] = Cond;
  SubExprs[INC] = Inc;
  SubExprs[BODY] = Body;
  SubExprs[LOOP_COUNT] = LoopCount;
}

SIMDForStmt::SIMDVariable::SIMDVariable(unsigned Kind, VarDecl *SIMDVar,
                                        VarDecl *LocalVar, Expr *UpdateExpr,
                                        ArrayRef<VarDecl *> IndexVars)
    : Kind(Kind), SIMDVar(SIMDVar), LocalVar(LocalVar), UpdateExpr(UpdateExpr),
      ArrayIndexNum(IndexVars.size()), ArrayIndexVars(0) {
  // Allocate an array of index variables if necessary.
  if (ArrayIndexNum) {
    ASTContext &C = SIMDVar->getASTContext();
    unsigned Size = ArrayIndexNum * sizeof(VarDecl *);
    void *Mem = C.Allocate(Size);
    std::memcpy(Mem, IndexVars.data(), Size);
    ArrayIndexVars = reinterpret_cast<VarDecl **>(Mem);
  }
}

SIMDForStmt::SIMDVariable *SIMDForStmt::getStoredSIMDVars() const {
  unsigned Size = sizeof(SIMDForStmt) + sizeof(Attr *) * NumSIMDAttrs;

  // Offset of the first SIMDVariable object.
  unsigned FirstSIMDVariableOffset =
      llvm::RoundUpToAlignment(Size, llvm::alignOf<SIMDVariable>());

  return reinterpret_cast<SIMDVariable *>(
      reinterpret_cast<char *>(const_cast<SIMDForStmt *>(this)) +
      FirstSIMDVariableOffset);
}

SIMDForStmt::SIMDForStmt(SourceLocation PragmaLoc, ArrayRef<Attr *> SIMDAttrs,
                         ArrayRef<SIMDVariable> SIMDVars, Stmt *Init,
                         Expr *Cond, Expr *Inc, CapturedStmt *Body,
                         Expr *LoopCount, Expr *LoopStride,
                         VarDecl *LoopControlVar, SourceLocation FL,
                         SourceLocation LP, SourceLocation RP)
    : Stmt(SIMDForStmtClass), LoopControlVar(LoopControlVar),
      PragmaLoc(PragmaLoc), ForLoc(FL), LParenLoc(LP), RParenLoc(RP),
      NumSIMDAttrs(SIMDAttrs.size()), NumSIMDVars(SIMDVars.size()) {

  assert(Init && Cond && Inc && Body && "null argument unexpected");
  SubExprs[INIT] = Init;
  SubExprs[COND] = Cond;
  SubExprs[INC] = Inc;
  SubExprs[BODY] = Body;
  SubExprs[LOOP_COUNT] = LoopCount;
  SubExprs[LOOP_STRIDE] = LoopStride;

  // Initialize the SIMD clauses.
  std::copy(SIMDAttrs.begin(), SIMDAttrs.end(), getStoredSIMDAttrs());

  // Copy all SIMDVariable objects.
  std::copy(SIMDVars.begin(), SIMDVars.end(), getStoredSIMDVars());
}

SIMDForStmt *SIMDForStmt::Create(const ASTContext &C, SourceLocation PragmaLoc,
                                 ArrayRef<Attr *> SIMDAttrs,
                                 ArrayRef<SIMDVariable> SIMDVars, Stmt *Init,
                                 Expr *Cond, Expr *Inc, CapturedStmt *Body,
                                 Expr *LoopCount, Expr *LoopStride,
                                 VarDecl *LoopControlVar, SourceLocation FL,
                                 SourceLocation LP, SourceLocation RP) {
  unsigned Size = sizeof(SIMDForStmt) + sizeof(Attr *) * SIMDAttrs.size();
  if (!SIMDVars.empty()) {
    // Realign for the following SIMDVariable array.
    Size = llvm::RoundUpToAlignment(Size, llvm::alignOf<SIMDVariable>());
    Size += sizeof(SIMDVariable) * SIMDVars.size();
  }

  void *Mem = C.Allocate(Size);
  return new (Mem)
      SIMDForStmt(PragmaLoc, SIMDAttrs, SIMDVars, Init, Cond, Inc, Body,
                  LoopCount, LoopStride, LoopControlVar, FL, LP, RP);
}

SIMDForStmt::SIMDForStmt(EmptyShell Empty, unsigned NumSIMDAttrs,
                         unsigned NumSIMDVars)
    : Stmt(SIMDForStmtClass, Empty), LoopControlVar(0),
      NumSIMDAttrs(NumSIMDAttrs), NumSIMDVars(NumSIMDVars) {
  SubExprs[INIT] = 0;
  SubExprs[COND] = 0;
  SubExprs[INC] = 0;
  SubExprs[BODY] = 0;
  SubExprs[LOOP_COUNT] = 0;
  SubExprs[LOOP_STRIDE] = 0;
}

SIMDForStmt *SIMDForStmt::CreateEmpty(const ASTContext &C,
                                      unsigned NumSIMDAttrs,
                                      unsigned NumSIMDVars) {
  assert(NumSIMDAttrs > 0 && "NumSIMDAttrs should be greater than zero");
  unsigned Size = sizeof(SIMDForStmt) + sizeof(Attr *) * NumSIMDAttrs;
  if (NumSIMDVars > 0) {
    // Realign for the following SIMDVariable array.
    Size = llvm::RoundUpToAlignment(Size, llvm::alignOf<SIMDVariable>());
    Size += sizeof(SIMDVariable) * NumSIMDVars;
  }

  void *Mem = C.Allocate(Size);
  return new (Mem) SIMDForStmt(EmptyShell(), NumSIMDAttrs, NumSIMDVars);
}

void CilkRankedStmt::setLengths(ArrayRef<Expr *> L) {
  assert(L.size() == Rank &&
         "Number of lengths is not the same as the preallocated buffer");
  std::copy(L.begin(), L.end(), reinterpret_cast<Expr **>(this + 1) + 2);
}

void CilkRankedStmt::setVars(ArrayRef<Stmt *> L) {
  assert(L.size() == Rank &&
         "Number of lengths is not the same as the preallocated buffer");
  std::copy(L.begin(), L.end(), reinterpret_cast<Stmt **>(this + 1) + 2 + Rank);
}

void CilkRankedStmt::setIncrements(ArrayRef<Stmt *> L) {
  assert(L.size() == Rank &&
         "Number of lengths is not the same as the preallocated buffer");
  std::copy(L.begin(), L.end(),
            reinterpret_cast<Stmt **>(this + 1) + 2 + 2 * Rank);
}

CilkRankedStmt *
CilkRankedStmt::Create(const ASTContext &C, SourceLocation StartLoc,
                       SourceLocation EndLoc, ArrayRef<Expr *> Lengths,
                       ArrayRef<Stmt *> Vars, ArrayRef<Stmt *> Increments,
                       Stmt *AssociatedStmt, Stmt *Inits) {
  void *Mem = C.Allocate(sizeof(CilkRankedStmt) + 2 * sizeof(Stmt *) +
                             sizeof(Expr *) * Lengths.size() +
                             sizeof(Stmt *) * 2 * Vars.size(),
                         llvm::alignOf<CilkRankedStmt>());
  CilkRankedStmt *S =
      new (Mem) CilkRankedStmt(StartLoc, EndLoc, Lengths.size());
  S->setLengths(Lengths);
  S->setVars(Vars);
  S->setIncrements(Increments);
  S->setAssociatedStmt(AssociatedStmt);
  S->setInits(Inits);
  return S;
}

CilkRankedStmt *CilkRankedStmt::CreateEmpty(const ASTContext &C, unsigned N,
                                            EmptyShell) {
  void *Mem = C.Allocate(sizeof(CilkRankedStmt) + 2 * sizeof(Stmt *) +
                             sizeof(Expr *) * N + sizeof(Stmt *) * 2 * N,
                         llvm::alignOf<CilkRankedStmt>());
  return new (Mem) CilkRankedStmt(N);
}

#endif // INTEL_SPECIFIC_CILKPLUS
