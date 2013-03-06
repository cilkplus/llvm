//===---------- CilkPlus.h - Common code for Cilk optimizations -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief This file contains some code that is common to Cilk optimization
/// passes, such as finding spawns and syncs.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_CILKPLUS_H
#define LLVM_TRANSFORMS_CILKPLUS_H

#include "llvm/Pass.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CallSite.h"

namespace llvm {
class Module;
class Function;

namespace cilkplus {

class CilkFunctionPass : public FunctionPass {
public:
  explicit CilkFunctionPass(char &ID) : FunctionPass(ID) { }

  virtual bool doInitialization(Module &M);

  static const Function *getCalledFunction(const Instruction *I) {
    ImmutableCallSite CI(I);
    if (CI)
      return CI.getCalledFunction();
    return 0;
  }

  static llvm::NamedMDNode *getSpawnMetadata(const llvm::Module &M) {
    return M.getNamedMetadata("cilk.spawn");
  }

  static llvm::NamedMDNode *getSyncMetadata(const llvm::Module &M) {
    return M.getNamedMetadata("cilk.sync");
  }

  /// \brief Determines if I is a call to a spawn helper.
  bool isSpawn(const Instruction *I) const {
    return SpawnFuncs.count(getCalledFunction(I));
  }

  /// \brief Determines if I is a call to a __cilk_sync.
  bool isSync(const Instruction *I) const {
    return SyncFuncs.count(getCalledFunction(I));
  }

  /// \brief Determines if I is a call to the parent prologue.
  bool isParentPrologue(const Instruction *I) const {
    return ParentPrologue == getCalledFunction(I);
  }

  /// \brief Determines if I is a call to the parent epilogue.
  bool isParentEpilogue(const Instruction *I) const {
    return ParentEpilogue == getCalledFunction(I);
  }

protected:
  /// \brief The parent prologue function for this module.
  Function *ParentPrologue;

  /// \brief The parent epilogue function for this module.
  Function *ParentEpilogue;

  /// \brief Cache of spawn functions.
  SmallPtrSet<const Function *, 4> SpawnFuncs;

  /// \brief Cache of sync functions.
  SmallPtrSet<const Function *, 4> SyncFuncs;
};

} // namespace cilkplus
} // namespace llvm

#endif // LLVM_TRANSFORMS_CILKPLUS_H
