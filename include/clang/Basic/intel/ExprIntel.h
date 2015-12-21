//===--- ExprIntel.h - Types for Pragma SIMD -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_INTEL_EXPR_H
#define LLVM_CLANG_AST_INTEL_EXPR_H

#if INTEL_SPECIFIC_CILKPLUS

#include "clang/AST/Expr.h"

namespace clang {
/// \brief Adaptor class for mixing a CilkSpawnDecl with expressions.
class CilkSpawnExpr : public Expr {
  CilkSpawnDecl *TheSpawn;

public:
  explicit CilkSpawnExpr(CilkSpawnDecl *D, QualType Ty)
    : Expr(CilkSpawnExprClass, Ty, VK_RValue, OK_Ordinary,
           Ty->isDependentType(), Ty->isDependentType(),
           Ty->isInstantiationDependentType(), false), TheSpawn(D) { }

  /// \brief Build an empty block expression.
  explicit CilkSpawnExpr(EmptyShell Empty) : Expr(CilkSpawnExprClass, Empty) { }

  const CilkSpawnDecl *getSpawnDecl() const { return TheSpawn; }
  CilkSpawnDecl *getSpawnDecl() { return TheSpawn; }
  void setSpawnDecl(CilkSpawnDecl *D) { TheSpawn = D; }

  Stmt *getSpawnStmt() { return TheSpawn->getSpawnStmt(); }
  const Stmt *getSpawnStmt() const { return TheSpawn->getSpawnStmt(); }

  Expr *getSpawnExpr() { return dyn_cast<Expr>(getSpawnStmt()); }
  const Expr *getSpawnExpr() const { return dyn_cast<Expr>(getSpawnStmt()); }

  SourceLocation getLocStart() const LLVM_READONLY;
  SourceLocation getLocEnd() const LLVM_READONLY;

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CilkSpawnExprClass;
  }

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// CEANIndexExpr - CEAN index triplet.
class CEANIndexExpr : public Expr {
  enum { BASE, LOWER_BOUND, LENGTH, STRIDE, INDEX_EXPR, END_EXPR };
  Stmt* SubExprs[END_EXPR];
  SourceLocation ColonLoc1, ColonLoc2;
  unsigned Rank;
public:
  CEANIndexExpr(Expr *Base, Expr *LowerBound, SourceLocation ColonLoc1,
                Expr *Length, SourceLocation ColonLoc2, Expr *Stride,
                QualType QTy)
  : Expr(CEANIndexExprClass, QTy, VK_RValue, OK_Ordinary,
         (Base && Base->isTypeDependent()) ||
         (LowerBound && LowerBound->isTypeDependent()) ||
         (Length && Length->isTypeDependent()) ||
         (Stride && Stride->isTypeDependent()),
         (Base && Base->isValueDependent()) ||
         (LowerBound && LowerBound->isValueDependent()) ||
         (Length && Length->isValueDependent()) ||
         (Stride && Stride->isValueDependent()),
         ((Base && Base->isInstantiationDependent()) ||
          (LowerBound && LowerBound->isInstantiationDependent()) ||
          (Length && Length->isInstantiationDependent()) ||
          (Stride && Stride->isInstantiationDependent())),
         ((Base && Base->containsUnexpandedParameterPack()) ||
          (LowerBound && LowerBound->containsUnexpandedParameterPack()) ||
          (Length && Length->containsUnexpandedParameterPack()) ||
          (Stride && Stride->containsUnexpandedParameterPack()))),
    ColonLoc1(ColonLoc1), ColonLoc2(ColonLoc2), Rank(0) {
    SubExprs[BASE] = Base;
    SubExprs[LOWER_BOUND] = LowerBound;
    SubExprs[LENGTH] = Length;
    SubExprs[STRIDE] = Stride;
    SubExprs[INDEX_EXPR] = 0;
  }

  /// \brief Create an empty CEAN index expression.
  explicit CEANIndexExpr(EmptyShell Shell)
    : Expr(CEANIndexExprClass, Shell), ColonLoc1(), ColonLoc2(), Rank(0) { }

  Expr *getBase() { return dyn_cast_or_null<Expr>(SubExprs[BASE]); }
  const Expr *getBase() const { return dyn_cast_or_null<Expr>(SubExprs[BASE]); }
  void setBase(Expr *E) { SubExprs[BASE] = E; }

  Expr *getLowerBound() { return dyn_cast_or_null<Expr>(SubExprs[LOWER_BOUND]); }
  const Expr *getLowerBound() const { return dyn_cast_or_null<Expr>(SubExprs[LOWER_BOUND]); }
  void setLowerBound(Expr *E) { SubExprs[LOWER_BOUND] = E; }

  Expr *getLength() { return dyn_cast_or_null<Expr>(SubExprs[LENGTH]); }
  const Expr *getLength() const { return dyn_cast_or_null<Expr>(SubExprs[LENGTH]); }
  void setLength(Expr *E) { SubExprs[LENGTH] = E; }

  Expr *getStride() { return dyn_cast_or_null<Expr>(SubExprs[STRIDE]); }
  const Expr *getStride() const { return dyn_cast_or_null<Expr>(SubExprs[STRIDE]); }
  void setStride(Expr *E) { SubExprs[STRIDE] = E; }

  SourceLocation getLocStart() const LLVM_READONLY {
    return getLowerBound()->getLocStart();
  }
  SourceLocation getLocEnd() const LLVM_READONLY { return getStride()->getLocEnd(); }

  SourceLocation getColonLoc1() const { return ColonLoc1; }
  void setColonLoc1(SourceLocation L) { ColonLoc1 = L; }
  SourceLocation getColonLoc2() const { return ColonLoc2; }
  void setColonLoc2(SourceLocation L) { ColonLoc2 = L; }

  SourceLocation getExprLoc() const LLVM_READONLY {
    return getLocStart();
  }

  unsigned getRank() const { return Rank; }
  void setRank(unsigned R) { Rank = R; }

  Expr *getIndexExpr() { return (!SubExprs[INDEX_EXPR] || SubExprs[INDEX_EXPR] == SubExprs[STRIDE]) ? 0 : cast<Expr>(SubExprs[INDEX_EXPR]); }
  Expr *getIndexExpr() const { return (!SubExprs[INDEX_EXPR] || SubExprs[INDEX_EXPR] == SubExprs[STRIDE]) ? 0 : cast<Expr>(SubExprs[INDEX_EXPR]); }
  void setIndexExpr(Expr *E) { SubExprs[INDEX_EXPR] = E; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CEANIndexExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(&SubExprs[LOWER_BOUND], &SubExprs[END_EXPR]);
  }
};

/// CEANBuiltinExpr - CEAN builtin expressions.
class CEANBuiltinExpr : public Expr {
public:
  enum CEANKindType {ReduceAdd, ReduceMul, ReduceMax, ReduceMin, ReduceMaxIndex,
                     ReduceMinIndex, ReduceAllZero, ReduceAllNonZero, ReduceAnyZero,
                     ReduceAnyNonZero, Reduce, ReduceMutating, ImplicitIndex, Unknown};
private:
  unsigned Rank, ArgsSize;
  CEANKindType CEANKind;
  SourceLocation StartLoc, RParenLoc;

  CEANBuiltinExpr(SourceLocation StartLoc,
                  unsigned Rank, unsigned ArgsSize,
                  CEANKindType Kind,
                  QualType QTy, SourceLocation RParenLoc)
  : Expr(CEANBuiltinExprClass, QTy, VK_RValue, OK_Ordinary,
         false, false, false, false),
    Rank(Rank), ArgsSize(ArgsSize), CEANKind(Kind), StartLoc(StartLoc), RParenLoc(RParenLoc) { }

  /// \brief Create an empty CEAN builtin expression.
  explicit CEANBuiltinExpr(unsigned Rank, unsigned ArgsSize)
    : Expr(CEANBuiltinExprClass, EmptyShell()), Rank(Rank), ArgsSize(ArgsSize), CEANKind(Unknown),
      StartLoc(), RParenLoc() { }

public:
  /// \brief Creates expression.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Rank Rank of expression.
  /// \param Kind Kind of builtin expression.
  /// \param Args List of arguments.
  /// \param Lengths List of lengths.
  /// \param Vars List of vars.
  /// \param Increments List of increments.
  /// \param Body Statement, associated with the statement.
  /// \param Return Return expression.
  /// \param QTy Type of expression.
  ///
  static CEANBuiltinExpr *Create(ASTContext &C,
                                 SourceLocation StartLoc,
                                 SourceLocation EndLoc,
                                 unsigned Rank,
                                 CEANKindType Kind,
                                 ArrayRef<Expr *> Args,
                                 ArrayRef<Expr *> Lengths,
                                 ArrayRef<Stmt *> Vars,
                                 ArrayRef<Stmt *> Increments,
                                 Stmt *Init, Stmt *Body, Expr *Return,
                                 QualType QTy);

  /// \brief Creates an empty expression.
  ///
  /// \param C AST context.
  /// \param Rank Rank of the arguments.
  /// \param ArgsSize Number of arguments.
  ///
  static CEANBuiltinExpr *CreateEmpty(ASTContext &C, unsigned Rank, unsigned ArgsSize);

  CEANKindType getBuiltinKind() const { return CEANKind; }
  void setBuiltinKind(CEANKindType Kind) { CEANKind = Kind; }

  unsigned getRank() const { return Rank; }
  unsigned getArgsSize() const { return ArgsSize; }

  Stmt *getInit() { return reinterpret_cast<Stmt **>(this + 1)[0]; }
  const Stmt *getInit() const { return reinterpret_cast<const Stmt * const*>(this + 1)[0]; }
  void setInit(Stmt *S) { reinterpret_cast<Stmt **>(this + 1)[0] = S; }

  Stmt *getBody() { return reinterpret_cast<Stmt **>(this + 1)[1]; }
  const Stmt *getBody() const { return reinterpret_cast<const Stmt * const*>(this + 1)[1]; }
  void setBody(Stmt *S) { reinterpret_cast<Stmt **>(this + 1)[1] = S; }

  Expr *getReturnExpr() { return reinterpret_cast<Expr **>(this + 1)[2]; }
  const Expr *getReturnExpr() const { return reinterpret_cast<const Expr * const*>(this + 1)[2]; }
  void setReturnExpr(Expr *E) { reinterpret_cast<Expr **>(this + 1)[2] = E; }

  ArrayRef<Expr *> getArgs() { return llvm::makeArrayRef(&reinterpret_cast<Expr **>(this + 1)[3], ArgsSize); }
  ArrayRef<const Expr *> getArgs() const { return llvm::makeArrayRef(&reinterpret_cast<const Expr * const*>(this + 1)[3], ArgsSize); }
  void setArgs(ArrayRef<Expr *> Args);

  ArrayRef<Expr *> getLengths() { return llvm::makeArrayRef(&reinterpret_cast<Expr **>(this + 1)[3 + ArgsSize], Rank); }
  ArrayRef<const Expr *> getLengths() const { return llvm::makeArrayRef(&reinterpret_cast<const Expr * const*>(this + 1)[3 + ArgsSize], Rank); }
  void setLengths(ArrayRef<Expr *> Lengths);

  ArrayRef<Stmt *> getVars() { return llvm::makeArrayRef(&reinterpret_cast<Stmt **>(this + 1)[3 + Rank + ArgsSize], Rank); }
  ArrayRef<const Stmt *> getVars() const { return llvm::makeArrayRef(&reinterpret_cast<const Stmt * const*>(this + 1)[3 + Rank + ArgsSize], Rank); }
  void setVars(ArrayRef<Stmt *> Vars);

  ArrayRef<Stmt *> getIncrements() { return llvm::makeArrayRef(&reinterpret_cast<Stmt **>(this + 1)[3 + 2 * Rank + ArgsSize], Rank); }
  ArrayRef<const Stmt *> getIncrements() const { return llvm::makeArrayRef(&reinterpret_cast<const Stmt * const*>(this + 1)[3 + 2 * Rank + ArgsSize], Rank); }
  void setIncrements(ArrayRef<Stmt *> Increments);

  SourceLocation getLocStart() const LLVM_READONLY { return StartLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return RParenLoc; }
  void setStartLoc(SourceLocation L) { StartLoc = L; }
  void setRParenLoc(SourceLocation L) { RParenLoc = L; }

  SourceLocation getExprLoc() const LLVM_READONLY {
    return getLocStart();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CEANBuiltinExprClass;
  }

  // Iterators
  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(this + 1),
                       &reinterpret_cast<Stmt **>(this + 1)[3 + 3 * Rank + ArgsSize]);
  }
};
} // end namespace clang

#endif // INTEL_SPECIFIC_CILKPLUS
#endif // LLVM_CLANG_AST_INTEL_EXPR_H
