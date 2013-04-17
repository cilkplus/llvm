//===-- CGCilkPlusRuntime.h - Interface to CilkPlus Runtimes ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file implements Cilk Plus runtime specific code generation.
///
//===----------------------------------------------------------------------===//

#ifndef CLANG_CODEGEN_CILKPLUSRUNTIME_H
#define CLANG_CODEGEN_CILKPLUSRUNTIME_H

#include "CGBuilder.h"
#include "CGCall.h"
#include "CGValue.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace llvm {
  class Constant;
  class Function;
  class Module;
  class StructLayout;
  class StructType;
  class Type;
  class Value;
}

namespace clang {

class CilkSpawnDeprecatedCapturedStmt;
class CilkSyncStmt;
class CXXThrowExpr;
class CXXTryStmt;

namespace CodeGen {

class CodeGenFunction;
class CodeGenModule;

/// \brief Implements Cilk Plus runtime specific code generation functions.
class CGCilkPlusRuntime {
public:
  enum CilkCleanupKind {
    ImplicitSyncCleanup = 0x01,
    ReleaseFrameCleanup = 0x02,
    ImpSyncAndRelFrameCleanup = ImplicitSyncCleanup | ReleaseFrameCleanup
  };

  void EmitCilkSpawn(CodeGenFunction &CGF, const CilkSpawnDeprecatedCapturedStmt &S);

  void EmitCilkSync(CodeGenFunction &CGF);

  void EmitCilkExceptingSync(CodeGenFunction &CGF);

  void EmitCilkParentStackFrame(CodeGenFunction &CGF,
                                CilkCleanupKind K = ImpSyncAndRelFrameCleanup);

  void EmitCilkHelperStackFrame(CodeGenFunction &CGF);

  void EmitCilkHelperCatch(llvm::BasicBlock *Catch, CodeGenFunction &CGF);

  void EmitCilkHelperPrologue(CodeGenFunction &CGF);

  void pushCilkImplicitSyncCleanup(CodeGenFunction &CGF);
};

/// \brief API to query if an implicit sync is necessary during code generation.
class CGCilkImplicitSyncInfo {
public:
  typedef llvm::SmallPtrSet<const Stmt *, 4> SyncStmtSetTy;

private:
  CodeGenFunction &CGF;

  /// \brief True if an implicit sync is needed for this spawning function.
  bool NeedsImplicitSync;

  /// \brief Store CXXTryStmt or CXXThrowExpr which needs an implicit sync.
  SyncStmtSetTy SyncSet;

public:

  explicit CGCilkImplicitSyncInfo(CodeGenFunction &CGF)
    : CGF(CGF), NeedsImplicitSync(false) {
    analyze();
  }

  /// \brief Checks if an implicit sync is needed for a spawning function.
  bool needsImplicitSync() const { return NeedsImplicitSync; }

  /// \brief Checks if an implicit sync is needed before throwing
  bool needsImplicitSync(const CXXThrowExpr *E) const {
    return SyncSet.count(reinterpret_cast<const Stmt *>(E));
  }

  /// \brief Checks if an implicit sync is needed for a try block.
  bool needsImplicitSync(const CXXTryStmt *S) const {
    return SyncSet.count(reinterpret_cast<const Stmt *>(S));
  }

private:
  void analyze();
};

/// \brief Creates an instance of an implicit sync info for a spawning function.
CGCilkImplicitSyncInfo *CreateCilkImplicitSyncInfo(CodeGenFunction &CGF);

} // namespace CodeGen
} // namespace clang

#endif
