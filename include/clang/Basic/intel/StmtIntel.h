//===--- StmtIntel.h - Types for Pragma SIMD -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_INTEL_STMT_H
#define LLVM_CLANG_AST_INTEL_STMT_H

#if INTEL_SPECIFIC_CILKPLUS
#include "clang/AST/Stmt.h"
#include "clang/Basic/intel/PragmaSIMD.h"

namespace clang {

/// CilkSyncStmt - This represents a _Cilk_sync
class CilkSyncStmt : public Stmt {
  SourceLocation SyncLoc;

  friend class ASTStmtReader;

public:
  explicit CilkSyncStmt(SourceLocation SL)
      : Stmt(CilkSyncStmtClass), SyncLoc(SL) {}
  explicit CilkSyncStmt(EmptyShell E) : Stmt(CilkSyncStmtClass, E) {}

  SourceLocation getSyncLoc() const { return SyncLoc; }

  SourceLocation getLocStart() const LLVM_READONLY { return SyncLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return SyncLoc; }

  SourceRange getSourceRange() const LLVM_READONLY {
    return SourceRange(SyncLoc);
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CilkSyncStmtClass;
  }
  static bool classof(CilkSyncStmt *) { return true; }

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
};

/// \brief This represents a Cilk for grainsize statement.
/// \code
/// #pragma cilk grainsize = expr
/// _Cilk_for(...) { ... }
/// \endcode
class CilkForGrainsizeStmt : public Stmt {
private:
  enum { GRAINSIZE, CILK_FOR, LAST };
  Stmt *SubExprs[LAST];
  SourceLocation LocStart;

public:
  /// \brief Construct a Cilk for grainsize statement.
  CilkForGrainsizeStmt(Expr *Grainsize, Stmt *CilkFor, SourceLocation LocStart);

  /// \brief Construct an empty Cilk for grainsize statement.
  explicit CilkForGrainsizeStmt(EmptyShell Empty);

  SourceLocation getLocStart() const LLVM_READONLY { return LocStart; }
  SourceLocation getLocEnd() const LLVM_READONLY {
    return SubExprs[CILK_FOR]->getLocEnd();
  }

  Expr *getGrainsize() { return reinterpret_cast<Expr *>(SubExprs[GRAINSIZE]); }
  const Expr *getGrainsize() const {
    return const_cast<CilkForGrainsizeStmt *>(this)->getGrainsize();
  }

  Stmt *getCilkFor() { return SubExprs[CILK_FOR]; }
  const Stmt *getCilkFor() const { return SubExprs[CILK_FOR]; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CilkForGrainsizeStmtClass;
  }

  child_range children() { return child_range(SubExprs, SubExprs + LAST); }
};

/// \brief This represents a Cilk for statement.
/// \code
/// _Cilk_for (int i = 0; i < n; ++i) {
///   // ...
/// }
/// \endcode
class CilkForStmt : public Stmt {
private:
  /// \brief An enumeration for accessing stored statements in a Cilk for
  /// statement.
  enum { INIT, COND, INC, BODY, LOOP_COUNT, LAST };

  Stmt *SubExprs[LAST]; // SubExprs[INIT] is an expression or declstmt.
                        // SubExprs[BODY] is a CapturedStmt.

  /// \brief The source location of '_Cilk_for'.
  SourceLocation CilkForLoc;

  /// \brief The source location of opening parenthesis.
  SourceLocation LParenLoc;

  /// \brief The source location of closing parenthesis.
  SourceLocation RParenLoc;

  /// \brief The loop control variable.
  const VarDecl *LoopControlVar;

  /// \brief The local copy of the loop control variable.
  VarDecl *InnerLoopControlVar;

  /// \brief The implicit generated full expression for adjusting
  /// the inner loop control variable.
  Expr *InnerLoopVarAdjust;

public:
  /// \brief Construct a Cilk for statement.
  CilkForStmt(Stmt *Init, Expr *Cond, Expr *Inc, CapturedStmt *Body,
              Expr *LoopCount, SourceLocation FL, SourceLocation LP,
              SourceLocation RP);

  /// \brief Construct an empty Cilk for statement.
  explicit CilkForStmt(EmptyShell Empty);

  /// \brief Retrieve the initialization expression or declaration statement.
  Stmt *getInit() { return SubExprs[INIT]; }
  const Stmt *getInit() const { return SubExprs[INIT]; }

  /// \brief Retrieve the loop condition expression.
  Expr *getCond() { return reinterpret_cast<Expr *>(SubExprs[COND]); }
  const Expr *getCond() const {
    return reinterpret_cast<Expr *>(SubExprs[COND]);
  }

  /// \brief Retrieve the loop increment expression.
  Expr *getInc() { return reinterpret_cast<Expr *>(SubExprs[INC]); }
  const Expr *getInc() const { return reinterpret_cast<Expr *>(SubExprs[INC]); }

  /// \brief Retrieve the loop body.
  CapturedStmt *getBody() {
    return reinterpret_cast<CapturedStmt *>(SubExprs[BODY]);
  }
  const CapturedStmt *getBody() const {
    return reinterpret_cast<CapturedStmt *>(SubExprs[BODY]);
  }

  /// \brief Retrieve the loop count expression.
  Expr *getLoopCount() {
    return reinterpret_cast<Expr *>(SubExprs[LOOP_COUNT]);
  }
  const Expr *getLoopCount() const {
    return reinterpret_cast<Expr *>(SubExprs[LOOP_COUNT]);
  }

  const VarDecl *getLoopControlVar() const { return LoopControlVar; }
  void setLoopControlVar(const VarDecl *V) { LoopControlVar = V; }

  VarDecl *getInnerLoopControlVar() const { return InnerLoopControlVar; }
  void setInnerLoopControlVar(VarDecl *V) { InnerLoopControlVar = V; }

  Expr *getInnerLoopVarAdjust() const { return InnerLoopVarAdjust; }
  void setInnerLoopVarAdjust(Expr *E) { InnerLoopVarAdjust = E; }

  SourceLocation getCilkForLoc() const LLVM_READONLY { return CilkForLoc; }
  SourceLocation getLParenLoc() const LLVM_READONLY { return LParenLoc; }
  SourceLocation getRParenLoc() const LLVM_READONLY { return RParenLoc; }

  void setCilkForLoc(SourceLocation L) { CilkForLoc = L; }
  void setLParenLoc(SourceLocation L) { LParenLoc = L; }
  void setRParenLoc(SourceLocation L) { RParenLoc = L; }

  SourceLocation getLocStart() const LLVM_READONLY { return CilkForLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY {
    return SubExprs[BODY]->getLocEnd();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CilkForStmtClass;
  }

  child_range children() { return child_range(SubExprs, SubExprs + LAST); }
};

/// \brief This represents a pragma SIMD for statement.
/// \code
/// #pragma simd ...
/// for (int i = 0; i < n; ++i) {
///   // ...
/// }
/// \endcode
class SIMDForStmt : public Stmt {
public:
  /// \brief Represent SIMD data-privatization variables associated with
  /// necessary expressions for code generation.
  ///
  /// If a variable is private, then the local copy is default-initialized.
  ///
  /// If a variable is firstprivate, then the local copy is copy-initialized.
  ///
  /// If a variable is lastprivate, then the local copy is default-initialized
  /// and an update expression is needed for the final iteration.
  ///
  /// If a variable is linear, then the local copy is initialized as
  /// \code
  ///  __local_x = x + __loop_index * step
  /// \endcode
  /// and an update expression is need for the final iteration.
  ///
  /// If a variable is reduction, then the local copy is initialized as
  /// specified by the reduction identifier. An update expression is need for
  /// the final iteration.
  class SIMDVariable {
    /// \brief The variable kind.
    unsigned Kind;

    /// \brief The variable in the clause.
    VarDecl *SIMDVar;

    /// \brief The local copy of this variable.
    VarDecl *LocalVar;

    /// \brief The update expression. This expression is only created for
    /// lastprivate and linear variables.
    Expr *UpdateExpr;

    /// \brief The number of array index variables.
    unsigned ArrayIndexNum;

    /// \brief The array index variables for the update expression.
    VarDecl **ArrayIndexVars;

  public:
    SIMDVariable(unsigned Kind, VarDecl *SIMDVar, VarDecl *LocalVar,
                 Expr *UpdateExpr, ArrayRef<VarDecl *> IndexVars);

    bool isPrivate() const { return Kind & SIMD_VK_Private; }
    bool isLastPrivate() const { return Kind & SIMD_VK_LastPrivate; }
    bool isFirstPrivate() const { return Kind & SIMD_VK_FirstPrivate; }
    bool isLinear() const { return Kind & SIMD_VK_Linear; }
    bool isReduction() const { return Kind & SIMD_VK_Reduction; }

    VarDecl *getSIMDVar() const { return SIMDVar; }
    VarDecl *getLocalVar() const { return LocalVar; }

    Expr *getUpdateExpr() const {
      assert((isLastPrivate() || isLinear()) &&
             "only for lastprivate and linear variables");
      return UpdateExpr;
    }

    unsigned getArrayIndexNum() const { return ArrayIndexNum; }
    ArrayRef<VarDecl *> getArrayIndexVars() const {
      return ArrayRef<VarDecl *>(ArrayIndexVars, ArrayIndexNum);
    }
  };

private:
  /// \brief An enumeration for accessing stored statements in a SIMD for
  /// statement.
  enum { INIT, COND, INC, BODY, LOOP_COUNT, LOOP_STRIDE, LAST };

  Stmt *SubExprs[LAST]; // SubExprs[INIT] is an expression or declstmt.
                        // SubExprs[BODY] is a CapturedStmt.

  /// \brief The control variable of the loop.
  VarDecl *LoopControlVar;

  /// \brief The source location of '#pragma'.
  SourceLocation PragmaLoc;

  /// \brief The source location of 'for'.
  SourceLocation ForLoc;

  /// \brief The source location of opening parenthesis.
  SourceLocation LParenLoc;

  /// \brief The source location of closing parenthesis.
  SourceLocation RParenLoc;

  /// \brief The number of SIMD clauses.
  unsigned NumSIMDAttrs;

  /// \brief The number of data-privatization variables effectively used
  /// in the SIMD for loop.
  unsigned NumSIMDVars;

  SIMDForStmt(EmptyShell Empty, unsigned NumSIMDAttrs, unsigned NumSIMDVars);

  SIMDForStmt(SourceLocation PragmaLoc, ArrayRef<Attr *> SIMDAttrs,
              ArrayRef<SIMDVariable> SIMDVars, Stmt *Init, Expr *Cond,
              Expr *Inc, CapturedStmt *Body, Expr *LoopCount, Expr *LoopStride,
              VarDecl *LoopControlVar, SourceLocation FL, SourceLocation LP,
              SourceLocation RP);

  Attr **getStoredSIMDAttrs() const {
    return reinterpret_cast<Attr **>(const_cast<SIMDForStmt *>(this) + 1);
  }

  SIMDVariable *getStoredSIMDVars() const;

public:
  /// \brief Construct a SIMD for statement.
  static SIMDForStmt *Create(const ASTContext &C, SourceLocation PragmaLoc,
                             ArrayRef<Attr *> SIMDAttrs,
                             ArrayRef<SIMDVariable> SIMDVars, Stmt *Init,
                             Expr *Cond, Expr *Inc, CapturedStmt *Body,
                             Expr *LoopCount, Expr *LoopStride,
                             VarDecl *LoopControlVar, SourceLocation FL,
                             SourceLocation LP, SourceLocation RP);

  /// \brief Construct an empty SIMD for statement.
  static SIMDForStmt *CreateEmpty(const ASTContext &C, unsigned NumSIMDAttrs,
                                  unsigned NumSIMDVars);

  ArrayRef<Attr *> getSIMDAttrs() const {
    return ArrayRef<Attr *>(getStoredSIMDAttrs(), NumSIMDAttrs);
  }

  /// \brief An iterator that walks over the SIMD data-privatization variables.
  typedef SIMDVariable *simd_var_iterator;

  simd_var_iterator simd_var_begin() const { return getStoredSIMDVars(); }
  simd_var_iterator simd_var_end() const {
    return getStoredSIMDVars() + NumSIMDVars;
  }

  /// \brief Retrieve the initialization expression or declaration statement.
  Stmt *getInit() { return SubExprs[INIT]; }
  const Stmt *getInit() const { return SubExprs[INIT]; }

  /// \brief Retrieve the loop condition expression.
  Expr *getCond() { return reinterpret_cast<Expr *>(SubExprs[COND]); }
  const Expr *getCond() const {
    return reinterpret_cast<Expr *>(SubExprs[COND]);
  }

  /// \brief Retrieve the loop increment expression.
  Expr *getInc() { return reinterpret_cast<Expr *>(SubExprs[INC]); }
  const Expr *getInc() const { return reinterpret_cast<Expr *>(SubExprs[INC]); }

  /// \brief Retrieve the loop body.
  CapturedStmt *getBody() {
    return reinterpret_cast<CapturedStmt *>(SubExprs[BODY]);
  }
  const CapturedStmt *getBody() const {
    return reinterpret_cast<CapturedStmt *>(SubExprs[BODY]);
  }

  /// \brief Retrieve the loop count expression.
  Expr *getLoopCount() {
    return reinterpret_cast<Expr *>(SubExprs[LOOP_COUNT]);
  }
  const Expr *getLoopCount() const {
    return reinterpret_cast<Expr *>(SubExprs[LOOP_COUNT]);
  }

  /// \brief Retrieve the loop control variable stride expression.
  Expr *getLoopStride() {
    return reinterpret_cast<Expr *>(SubExprs[LOOP_STRIDE]);
  }
  const Expr *getLoopStride() const {
    return reinterpret_cast<Expr *>(SubExprs[LOOP_STRIDE]);
  }

  /// \brief Retrieve the loop control variable.
  VarDecl *getLoopControlVar() { return LoopControlVar; }
  const VarDecl *getLoopControlVar() const {
    return const_cast<SIMDForStmt *>(this)->getLoopControlVar();
  }

  SourceLocation getPragmaLoc() const LLVM_READONLY { return PragmaLoc; }
  SourceLocation getForLoc() const LLVM_READONLY { return ForLoc; }
  SourceLocation getLParenLoc() const LLVM_READONLY { return LParenLoc; }
  SourceLocation getRParenLoc() const LLVM_READONLY { return RParenLoc; }

  void setPragmaLoc(SourceLocation L) { PragmaLoc = L; }
  void setForLoc(SourceLocation L) { ForLoc = L; }
  void setLParenLoc(SourceLocation L) { LParenLoc = L; }
  void setRParenLoc(SourceLocation L) { RParenLoc = L; }

  SourceLocation getLocStart() const LLVM_READONLY { return PragmaLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY {
    return SubExprs[BODY]->getLocEnd();
  }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == SIMDForStmtClass;
  }

  child_range children() { return child_range(SubExprs, SubExprs + LAST); }
};

/// \brief This represents special ranked statements.
///
class CilkRankedStmt : public Stmt {
  friend class ASTStmtReader;
  SourceLocation StartLoc, EndLoc;
  unsigned Rank;

  /// \brief Build statement with the given start and end location.
  ///
  /// \param StartLoc Starting location.
  /// \param EndLoc Ending Location.
  /// \param Rank Rank of the statement.
  ///
  CilkRankedStmt(SourceLocation StartLoc, SourceLocation EndLoc, unsigned Rank)
      : Stmt(CilkRankedStmtClass), StartLoc(StartLoc), EndLoc(EndLoc),
        Rank(Rank) {}

  /// \brief Build an empty statement.
  ///
  /// \param Rank Number of clause.
  ///
  explicit CilkRankedStmt(unsigned Rank)
      : Stmt(CilkRankedStmtClass), StartLoc(), EndLoc(), Rank(Rank) {}

  /// \brief Sets lengths.
  ///
  void setLengths(ArrayRef<Expr *> L);

  /// \brief Sets pseudo vars.
  ///
  void setVars(ArrayRef<Stmt *> L);

  /// \brief Sets increments for vars.
  ///
  void setIncrements(ArrayRef<Stmt *> L);

  /// \brief Set the associated statement for the directive.
  ///
  void setAssociatedStmt(Stmt *S) {
    reinterpret_cast<Stmt **>(this + 1)[0] = S;
  }

  /// \brief Set the init for ranked statement.
  ///
  void setInits(Stmt *S) { reinterpret_cast<Stmt **>(this + 1)[1] = S; }

public:
  /// \brief Creates statement.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Lengths List of lengths.
  /// \param AssociatedStmt Statement, associated with the statement.
  ///
  static CilkRankedStmt *Create(const ASTContext &C, SourceLocation StartLoc,
                                SourceLocation EndLoc, ArrayRef<Expr *> Lengths,
                                ArrayRef<Stmt *> Vars,
                                ArrayRef<Stmt *> Increments,
                                Stmt *AssociatedStmt, Stmt *Inits);

  /// \brief Creates an empty statement.
  ///
  /// \param C AST context.
  /// \param N Rank of the statement.
  ///
  static CilkRankedStmt *CreateEmpty(const ASTContext &C, unsigned N,
                                     EmptyShell);

  unsigned getRank() const { return Rank; }

  /// \brief Fetches the list of lengths.
  ArrayRef<const Expr *> getLengths() const {
    return ArrayRef<const Expr *>(
        reinterpret_cast<const Expr *const *>(this + 1) + 2, Rank);
  }
  ArrayRef<Expr *> getLengths() {
    return ArrayRef<Expr *>(reinterpret_cast<Expr **>(this + 1) + 2, Rank);
  }

  /// \brief Fetches the list of vars.
  ArrayRef<const Stmt *> getVars() const {
    return ArrayRef<const Stmt *>(
        reinterpret_cast<const Stmt *const *>(this + 1) + 2 + Rank, Rank);
  }
  ArrayRef<Stmt *> getVars() {
    return ArrayRef<Stmt *>(reinterpret_cast<Stmt **>(this + 1) + 2 + Rank,
                            Rank);
  }

  /// \brief Fetches the list of vars.
  ArrayRef<const Stmt *> getIncrements() const {
    return ArrayRef<const Stmt *>(
        reinterpret_cast<const Stmt *const *>(this + 1) + 2 + 2 * Rank, Rank);
  }
  ArrayRef<Stmt *> getIncrements() {
    return ArrayRef<Stmt *>(reinterpret_cast<Stmt **>(this + 1) + 2 + 2 * Rank,
                            Rank);
  }

  const Stmt *getAssociatedStmt() const {
    return reinterpret_cast<const Stmt *const *>(this + 1)[0];
  }

  Stmt *getAssociatedStmt() { return reinterpret_cast<Stmt **>(this + 1)[0]; }

  const Stmt *getInits() const {
    return reinterpret_cast<const Stmt *const *>(this + 1)[1];
  }

  Stmt *getInits() { return reinterpret_cast<Stmt **>(this + 1)[1]; }

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == CilkRankedStmtClass;
  }

  SourceLocation getLocStart() const LLVM_READONLY { return StartLoc; }
  SourceLocation getLocEnd() const LLVM_READONLY { return EndLoc; }

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(this + 1),
                       reinterpret_cast<Stmt **>(this + 1) + 2 + 2 * Rank);
  }
};
} // end namespace clang
#endif // INTEL_SPECIFIC_CILKPLUS
#endif // LLVM_CLANG_AST_INTEL_STMT_H
