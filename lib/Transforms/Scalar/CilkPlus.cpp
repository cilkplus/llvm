//===--------- CilkPlus.cpp - Common code for Cilk optimizations ----------===//
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

#include "CilkPlus.h"
using namespace llvm;
using namespace llvm::cilkplus;

/// \brief Collect and cache spawn, sync and parent prologue/epilogue functions
/// in this module.
bool CilkFunctionPass::doInitialization(Module &M) {
  ParentPrologue = M.getFunction("__cilk_parent_prologue");
  ParentEpilogue = M.getFunction("__cilk_parent_epilogue");
  SyncFuncs.clear();
  SpawnFuncs.clear();

  if (NamedMDNode *Syncs = getSyncMetadata(M)) {
    for (unsigned I = 0, N = Syncs->getNumOperands(); I < N; ++I) {
      MDNode *Node = Syncs->getOperand(I);
      if (Node->getNumOperands() == 0)
        continue;

      if (Function *Sync = dyn_cast_or_null<Function>(Node->getOperand(0)))
        SyncFuncs.insert(Sync);
    }
  }

  if (NamedMDNode *Spawns = getSpawnMetadata(M)) {
    for (unsigned I = 0, N = Spawns->getNumOperands(); I < N; ++I) {
      MDNode *Node = Spawns->getOperand(I);
      if (Node->getNumOperands() == 0)
        continue;

      if (Function *Spawn = dyn_cast_or_null<Function>(Node->getOperand(0)))
        SpawnFuncs.insert(Spawn);
    }
  }

  return true;
}

