//=== CilkStackFrameInit.cpp - Optimization for stack frame initialization ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief This file implements an optimization to move the initialization of
/// Cilk stack frames to the branches that actually spawn.  This should save
/// time in the base case of recursive calls by avoiding calls into the runtime
/// but may pessimize unconditional spawns slightly, since it requires the
/// parent prologue, epilogue, and sync to handle the case where the stack
/// frame is uninitialized by checking for a null worker.
///
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "cilk-sf-late-init"

#include "CilkPlus.h"
#include "llvm/Pass.h"
#include "llvm/PassSupport.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Scalar.h"
using namespace llvm;
using namespace llvm::cilkplus;

STATISTIC(NumStackInitsDelayed, "Number of stack-frame initializations delayed");
STATISTIC(NumStackInitsInserted, "Number of new late stack-frame initializations");
STATISTIC(NumCallsConditionalized, "Number of sync and parent epilogue calls made conditional");

namespace {

// These are from CGCilkPlusRuntime.cpp in Clang, but it's not worth adding a
// header just to share these.
enum {
  __CILKRTS_ABI_VERSION = 1
};
#define CILK_FRAME_VERSION (__CILKRTS_ABI_VERSION << 24)

enum StackFrameMembers {
  flags,
  size,
  call_parent,
  worker,
  except_data,
  ctx,
  mxcsr,
  fpcsr,
  reserved,
  parent_pedigree
};

class CilkStackFrameLateInit : public CilkFunctionPass {
public:
  static char ID;
  CilkStackFrameLateInit() : CilkFunctionPass(ID) {
    initializeCilkStackFrameLateInitPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F);

  /// \brief This pass preserves the CFG.
  virtual void getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequired<DominatorTree>();
    AU.setPreservesCFG();
  }

private:
  typedef SmallVector<Instruction *, 4> InstListTy;
  typedef SmallPtrSet<const BasicBlock *, 4> VisitedSetTy;
  typedef SmallVector<const BasicBlock *, 4> VisitedStackTy;
  typedef SmallPtrSet<const Function *, 4> FuncSetTy;
  typedef SmallVector<const BasicBlock *, 2> ExitBlocksTy;

  /// \brief A list of exit blocks from the current function.
  ExitBlocksTy ExitBlocks;

  const DominatorTree *DT;

private:
  CallInst *findInit(const Function &F) const;
  InstListTy findDestroy(const Function &F) const;
  bool hasMultipleInits(const Function &F, const Instruction *Init) const;
  void computeExitBlocks(const Function &F);
  bool isUnconditional(const Instruction *I) const;
  void findSpawnsAndSyncs(bool &UnconditionalSpawn, InstListTy &Spawns,
                          InstListTy &Syncs, const BasicBlock *BB) const;
  void findSpawnsAndSyncs(bool &UnconditionalSpawn,
                          InstListTy &Spawns,
                          InstListTy &Syncs,
                          VisitedSetTy &Visited,
                          VisitedStackTy &PrevSpawnBBs,
                          const BasicBlock *BB) const;
  static void addSyncMD(Function *Sync);
};

} // anonymous namespace

/// \brief Introduces a conditional version of Orig, by checking that sf->worker
/// is not null before calling Orig.
static Function *createConditionalFunc(Function *Orig, bool &New,
                                       bool OnNull=false) {
  New = false;
  assert(Orig->getName().startswith("__cilk_") &&
         "createConditionalFunc is only for cilk functions");
  Module *M = Orig->getParent();
  std::string Name(("__cilk_conditional" + Orig->getName().drop_front(6)).str());

  Function *F = M->getFunction(Name);
  if (F)
    return F;

  New = true;
  F = Function::Create(Orig->getFunctionType(),
                       Orig->getLinkage(),
                       Name, M);

  F->setAttributes(Orig->getAttributes());

  Value *SF = F->arg_begin();

  BasicBlock *Entry = BasicBlock::Create(M->getContext(), "entry", F);
  BasicBlock *Call = BasicBlock::Create(M->getContext(), "call", F);
  BasicBlock *Exit = BasicBlock::Create(M->getContext(), "exit", F);

  // Entry
  {
    IRBuilder<> B(Entry);
    Value *W = B.CreateLoad(B.CreateStructGEP(SF, worker));
    Value *C = 0;
    if (OnNull)
      C = B.CreateICmpEQ(W, Constant::getNullValue(W->getType()));
    else
      C = B.CreateICmpNE(W, Constant::getNullValue(W->getType()));
    B.CreateCondBr(C, Call, Exit);
  }

  // Call
  {
    IRBuilder<> B(Call);
    B.CreateCall(Orig, SF);
    B.CreateBr(Exit);
  }

  // Exit
  {
    IRBuilder<> B(Exit);
    B.CreateRetVoid();
  }

  return F;
}

/// \brief Minimally initializes the stack frame before the given instruction.
static void insertZeroInit(Instruction *IP, Value *SF) {
  IRBuilder<> B(IP);
  // sf->flags = CILK_FRAME_VERSION
  Value *Flags = B.CreateStructGEP(SF, flags);
  B.CreateStore(ConstantInt::get(Flags->getType()->getContainedType(0),
                                 CILK_FRAME_VERSION), Flags);

  // sf->worker = 0
  Value *W = B.CreateStructGEP(SF, worker);
  B.CreateStore(Constant::getNullValue(W->getType()->getContainedType(0)), W);
}

/// \brief Perform late-initialization of Cilk stack frames.
/// This pass moves calls to __cilk_parent_prologue to the location of the first
/// conditional spawn(s), and makes calls to __cilk_sync and
/// __cilk_parent_epilogue conditional by creating variants of those functions
/// that check whether sf->worker is initialized.
bool CilkStackFrameLateInit::runOnFunction(Function &F) {
  bool Changed = false;
  if (!ParentPrologue || !ParentEpilogue || SpawnFuncs.empty())
    return Changed;

  // Precondition: the parent_prologue is called in the entry block, and the
  // parent_epilogue is called before the end of the function.
  CallInst *Init = findInit(F);
  InstListTy Epilogues = findDestroy(F);
  if (!Init || Epilogues.empty()) return Changed;

  // Precondition: the parent_prologue is called exactly once.
  if (hasMultipleInits(F, Init)) return Changed;

  DT = &getAnalysis<DominatorTree>();
  computeExitBlocks(F);

  bool UnconditionalSpawn = false;
  InstListTy Spawns;
  InstListTy Syncs;
  findSpawnsAndSyncs(UnconditionalSpawn, Spawns, Syncs, &F.getEntryBlock());

  // If there is an unconditional spawn, then the stack frame must always be
  // initialized anyway, so quit.
  if (UnconditionalSpawn)
    return Changed;

  Changed = true;
  Value *SF = Init->getArgOperand(0);

  // Remove original initialization.
  ++NumStackInitsDelayed;
  assert(Init->hasNUses(0) && "Initialization should be void function call");
  insertZeroInit(Init, SF);
  Init->eraseFromParent();

  // Insert conditional initalizations.
  bool New;
  Function *CondPrologue = createConditionalFunc(ParentPrologue,
                                                 New, /*OnNull*/true);
  for (InstListTy::iterator I = Spawns.begin(), E = Spawns.end(); I != E; ++I) {
    ++NumStackInitsInserted;
    IRBuilder<> B(*I);
    B.CreateCall(CondPrologue, SF);
  }

  // Convert to conditional syncs.
  for (InstListTy::iterator I = Syncs.begin(), E = Syncs.end(); I != E; ++I) {
    ++NumCallsConditionalized;
    IRBuilder<> B(*I);
    Function *Callee = const_cast<Function*>(getCalledFunction(*I));
    Function *CondSync = createConditionalFunc(Callee, New);
    if (New)
      addSyncMD(CondSync);
    B.CreateCall(CondSync, SF);
    (*I)->eraseFromParent();
  }

  // Convert to conditional epilgoue.
  Function *CondEpilogue = createConditionalFunc(ParentEpilogue, New);
  for (InstListTy::iterator I = Epilogues.begin(), E = Epilogues.end(); I != E; ++I) {
    IRBuilder<> B(*I);
    B.CreateCall(CondEpilogue, SF);
    (*I)->eraseFromParent();
  }

  return Changed;
}

/// \brief Determines if I is unconditionally executed.
bool CilkStackFrameLateInit::isUnconditional(const Instruction *I) const {
  // A BB is unconditional if it dominates all exits.
  for (ExitBlocksTy::const_iterator BB = ExitBlocks.begin(),
       E = ExitBlocks.end(); BB != E; ++BB) {
    if (!DT->dominates(I->getParent(), *BB))
      return false;
  }
  return true;
}

/// \brief Finds the (first) call to the parent prologue, which initializes the
/// Cilk stack frame for this function.
CallInst *CilkStackFrameLateInit::findInit(const Function &F) const {
  const BasicBlock &BB = F.getEntryBlock();
  for (BasicBlock::const_iterator I = BB.begin(), E = BB.end(); I != E; ++I)
    if (isParentPrologue(I))
      return cast<CallInst>(const_cast<Instruction*>(&*I));
  return 0;
}

/// \brief Finds the call to the parent epilogue, which destroys the
/// Cilk stack frame for this function.
CilkStackFrameLateInit::InstListTy
CilkStackFrameLateInit::findDestroy(const Function &F) const {
  InstListTy Epilogues;
  for (Function::use_iterator U = ParentEpilogue->use_begin(),
       E = ParentEpilogue->use_end(); U != E; ++U) {
    Instruction *I = dyn_cast<Instruction>(*U);
    if (I && I->getParent()->getParent() == &F)
      Epilogues.push_back(I);
  }
  return Epilogues;
}

/// \brief Determines if F has multiple calls to the parent prologue.
bool CilkStackFrameLateInit::hasMultipleInits(const Function &F,
                                              const Instruction *Init) const {
  for (Function::use_iterator U = ParentPrologue->use_begin(),
       E = ParentPrologue->use_end(); U != E; ++U) {
    const Instruction *I = dyn_cast<Instruction>(*U);
    if (I && I != Init && I->getParent()->getParent() == &F)
      return true;
  }
  return false;
}

/// \brief Determines the set of exit blocks from F.
void CilkStackFrameLateInit::computeExitBlocks(const Function &F) {
  ExitBlocks.clear();
  for (Function::const_iterator BB = F.begin(), E = F.end(); BB != E; ++BB)
    if (std::distance(succ_begin(BB), succ_end(BB)) == 0)
      ExitBlocks.push_back(BB);
}

/// \brief Finds all spawns that should have a delayed initialization inserted
/// before them, and all syncs that need to become conditional syncs, since the
/// stack frame will not necessarily be initialized.
void CilkStackFrameLateInit::findSpawnsAndSyncs(bool &UnconditionalSpawn,
                                                InstListTy &Spawns,
                                                InstListTy &Syncs,
                                                const BasicBlock *BB) const {

  VisitedSetTy Visited;
  VisitedStackTy PrevSpawnBBs;
  findSpawnsAndSyncs(UnconditionalSpawn, Spawns, Syncs,
                     Visited, PrevSpawnBBs, BB);
}

void CilkStackFrameLateInit::findSpawnsAndSyncs(bool &UnconditionalSpawn,
                                                InstListTy &Spawns,
                                                InstListTy &Syncs,
                                                VisitedSetTy &Visited,
                                                VisitedStackTy &PrevSpawnBBs,
                                                const BasicBlock *BB) const {
  if (!Visited.insert(BB)) return;

  bool RequiresInit = true;
  for (VisitedStackTy::iterator PS = PrevSpawnBBs.begin(),
       PSE = PrevSpawnBBs.end(); PS != PSE; ++PS)
    if (DT->dominates(*PS, BB))
      RequiresInit = false;

  // If this block is dominated by a previously seen spawn, then the stack frame
  // is already initialized and we don't need to change anything.
  const Instruction *Spawn = 0;
  if (RequiresInit) {
    // Find the first (if any) spawn in this block, as well as any syncs.
    for (BasicBlock::const_iterator I = BB->begin(), E = BB->end(); I != E; ++I) {
      // Since we only care about Spawns that are not dominated by other spawns,
      // we only need to consider the first spawn in a basic block.
      if (!Spawn && isSpawn(I))
        Spawn = I;
      else if (!Spawn && isSync(I))
        Syncs.push_back(const_cast<Instruction *>(&*I));
    }
  }

  // If there is a spawn that requires init, add it to Spawns, and push it onto
  // the stack of potentially dominating spawns.
  if (Spawn) {
    UnconditionalSpawn = UnconditionalSpawn || isUnconditional(Spawn);
    Spawns.push_back(const_cast<Instruction *>(Spawn));
    PrevSpawnBBs.push_back(Spawn->getParent());
  }

  // Depth-first search of the remaining basic blocks.
  for (succ_const_iterator Next = succ_begin(BB); Next != succ_end(BB); ++Next)
    findSpawnsAndSyncs(UnconditionalSpawn, Spawns, Syncs,
                       Visited, PrevSpawnBBs, *Next);

  if (Spawn)
    PrevSpawnBBs.pop_back();
}

/// \brief Add a new sync function to the cilk.sync named metadata.
void CilkStackFrameLateInit::addSyncMD(Function *Sync) {
  Module *M = Sync->getParent();
  getSyncMetadata(*M)->addOperand(MDNode::get(M->getContext(), Sync));
}

char CilkStackFrameLateInit::ID = 0;
INITIALIZE_PASS_BEGIN(CilkStackFrameLateInit, "cilk-sf-late-init",
                      "Do late stack-frame initialization", false, false)
INITIALIZE_PASS_DEPENDENCY(DominatorTree)
INITIALIZE_PASS_END(CilkStackFrameLateInit, "cilk-sf-late-init",
                    "Do late stack-frame initialization", false, false)

FunctionPass *llvm::createCilkStackFrameLateInitPass() {
  return new CilkStackFrameLateInit();
}
