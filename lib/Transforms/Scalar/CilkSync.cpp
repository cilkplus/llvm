//===------------- CilkSync.cpp - Optimizations for Cilk sync -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief This file implements an optimization to elide unnecessary calls to
/// __cilk_sync. The most common case is an explicit sync that occurs
/// immediately before an implicit sync.
///
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "elide-cilk-sync"

#include "CilkPlus.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/PassSupport.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"

#include <algorithm>
#include <deque>

using namespace llvm;
using namespace llvm::cilkplus;

STATISTIC(NumSyncsRemoved, "Number of Cilk syncs removed");

namespace {
  struct StackFrameInfo {
    const Value *Gen;
    const Value *Kill;
    const Value *In;
    const Value *Out;

    StackFrameInfo() : Gen(0), Kill(0), In(0), Out(0) {}
  };

  class CilkCFGNode {
  public:
    CilkCFGNode(Value *V, CilkCFGNode *Parent, const Value *SF) : V(V) {
      Dummy = isa<BasicBlock>(V);
      if (Parent) {
        addPred(Parent);
        Parent->addSucc(this);
      }

      // In(B) = all stack frames
      SFInfo.In = SF;
      SFInfo.Out = SF;
    }

    typedef SmallPtrSet<CilkCFGNode*, 2>::iterator pred_iterator;
    pred_iterator pred_begin() { return Preds.begin(); }
    pred_iterator pred_end()   { return Preds.end(); }

    unsigned getNumPreds() const { return Preds.size(); }

    typedef SmallPtrSet<CilkCFGNode*, 2>::iterator succ_iterator;
    succ_iterator succ_begin() { return Succs.begin(); }
    succ_iterator succ_end()   { return Succs.end(); }

    Instruction *getInst() {
      assert(!Dummy && "Dummy can't have instruction");
      return cast<Instruction>(V);
    }

    void addPred(CilkCFGNode *Parent) { Preds.insert(Parent); }

    void addSucc(CilkCFGNode *Succ) {
      Succs.insert(Succ);
    }

  private:
    /// \brief The LLVM Value that this node represents, either a BasicBlock
    /// or a CallInst/InvokeInst of a Cilk function.
    Value *V;

    /// \brief This node is a dummy if it represents a basic block
    /// that has no Cilk calls.
    bool Dummy;
    SmallPtrSet<CilkCFGNode*, 2> Succs;
    SmallPtrSet<CilkCFGNode*, 2> Preds;

  public:
    StackFrameInfo SFInfo;
  };

  class ElideCilkSync : public CilkFunctionPass {
  public:
    static char ID;
    ElideCilkSync() : CilkFunctionPass(ID) {
      initializeFindUsedTypesPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F);

    /// \brief We preserve CFG.
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesCFG();
    }

  private:
    DenseMap<Value*, CilkCFGNode*> InstrToCilkCFG;
    SmallVector<CilkCFGNode*, 4> CilkCalls;

    /// \brief Cache of previously visited CilkCFG nodes.
    SmallPtrSet<const void*, 8> Visited;

    /// \brief The Cilk stack frame in the function.
    const Value *StackFrame;

  private:
    CilkCFGNode *analyzeSyncs(BasicBlock *BB, CilkCFGNode *CurNode);
    bool propogateInOut(CilkCFGNode *CurNode);
    void updateOut(StackFrameInfo &Info);
    bool updateInOut(CilkCFGNode *Cur);

    Instruction *getNextInstr(BasicBlock::iterator &BBI, bool &IsKill);
    bool insertOrUpdateCilkCFG(Value *V, const Value *SF,
                               CilkCFGNode *Parent, CilkCFGNode *&NewNode);

    const Value *getStackFrame(const CallInst *CilkFn) const;
    bool setStackFrame(const Function *F);
  };
} // namespace

char ElideCilkSync::ID = 0;
INITIALIZE_PASS(ElideCilkSync, "elide-cilk-sync", "Elide Cilk Sync", false, false)

FunctionPass *llvm::createElideCilkSyncPass() {
  return new ElideCilkSync();
}

/// \brief Determine which syncs are redundant and remove
/// them from the function.
bool ElideCilkSync::runOnFunction(Function &F) {
  // Skip functions that do not have a sync
  if (!setStackFrame(&F))
    return false;

  Visited.clear();
  CilkCalls.clear();
  InstrToCilkCFG.clear();

  // Walk the CFG and build another graph containing only Cilk calls
  // (spawns and syncs). During the walk, initialize gen and kill sets for
  // each Cilk call seen. This is basically available expression analysis
  // where the expressions are the Cilk stack frames, syncs generate them,
  // and spawns kill them. Since there is only one stack frame per function,
  // the sets can be reduced to a single pointer to the stack frame, or null
  // for the empty set.
  BasicBlock *BB = &F.getEntryBlock();
  CilkCFGNode *CilkCFGRoot = analyzeSyncs(BB, 0);

  // Now propogate the in/out sets forward until there are no more changes.
  while (propogateInOut(CilkCFGRoot));

  bool MadeChange = false;
  // Remove candidates from the function
  for (SmallVector<CilkCFGNode*, 4>::iterator SI = CilkCalls.begin();
       SI != CilkCalls.end(); ++SI) {
    Instruction *I = (*SI)->getInst();
    if (isSync(I) && (*SI)->SFInfo.In == StackFrame) {
      NumSyncsRemoved++;
      MadeChange = true;
      I->eraseFromParent();
    }
    // Destroy Cilk-only CFG
    delete *SI;
  }

  return MadeChange;
}

/// \brief Build a CFG containing only Cilk calls, and initialize gen and
/// kill sets for each Cilk call. Returns a pointer to the first node
/// in the Cilk-only CFG.
CilkCFGNode *ElideCilkSync::analyzeSyncs(BasicBlock *BB, CilkCFGNode *CurNode) {
  bool VisitSuccessors = Visited.insert(BB);

  BasicBlock::iterator BBI = BB->begin();
  CilkCFGNode *RootNode = CurNode;

  bool IsKill;
  Instruction *I = getNextInstr(BBI, IsKill);
  bool FoundCilkCall = I != 0;

  while (I) {
    // Build Cilk-only CFG
    CilkCFGNode *Parent = CurNode;

    // Already visited this basic block so we can skip the remaining
    // Cilk calls, since they are already successors
    if (!insertOrUpdateCilkCFG(I, StackFrame, Parent, CurNode))
      break;

    // Add a new CFG node for this cilk call
    CilkCalls.push_back(CurNode);
    if (!RootNode)
      RootNode = CurNode;

    // Compute gen/kill sets for each sync and spawn. The Cilk stack frame
    // is the "expression"
    if (IsKill)
      CurNode->SFInfo.Kill = StackFrame;
    else
      CurNode->SFInfo.Gen = StackFrame;

    updateOut(CurNode->SFInfo);

    I = getNextInstr(BBI, IsKill);
  }

  if (!FoundCilkCall) {
    // If there were no Cilk calls in this BB, create an empty (dummy) CFG
    // node in the Cilk-only CFG to represent the control flow through this BB.
    CilkCFGNode *Parent = CurNode;
    insertOrUpdateCilkCFG(BB, StackFrame, Parent, CurNode);
    if (!RootNode)
      RootNode = CurNode;
  }

  if (!VisitSuccessors)
    return RootNode;

  for (succ_iterator NextBB = succ_begin(BB); NextBB != succ_end(BB); ++NextBB)
    analyzeSyncs(*NextBB, CurNode);

  return RootNode;
}

/// \brief Update the in/out sets for each node in the CFG.
/// Returns true if any in/out set was changed, false otherwise.
bool ElideCilkSync::propogateInOut(CilkCFGNode *Root) {
  std::deque<CilkCFGNode*> Next;
  CilkCFGNode *Cur = Root;
  bool MadeChange = false;

  Visited.clear();

  // Do a level-first traversal of the Cilk-only CFG and update in/out
  Next.push_back(Root);
  while (!Next.empty()) {
    Cur = Next.front();
    Next.pop_front();

    for (CilkCFGNode::succ_iterator I = Cur->succ_begin();
        I != Cur->succ_end(); ++I) {
      if (Visited.insert(*I))
        Next.push_back(*I);
    }

    if (Cur != Root)
      MadeChange |= updateInOut(Cur);
  }

  return MadeChange;
}

/// \brief Update the in and out sets of Cur based on its predecessors.
/// Returns true if either of the in/out sets were changed, false otherwise.
bool ElideCilkSync::updateInOut(CilkCFGNode *Cur) {
  StackFrameInfo &SFInfo = Cur->SFInfo;

  const Value *PrevIn = SFInfo.In;
  const Value *PrevOut = SFInfo.Out;

  // In(B) = n Out(Pred(B))
  if (Cur->getNumPreds() > 0) {
    CilkCFGNode::pred_iterator PI = Cur->pred_begin();

    // Out(B) = Out(Pred(B)_0)
    SFInfo.In = (*PI)->SFInfo.Out;
    ++PI;

    // Out(B) = Out(B) n Out(Pred(B)_i)
    for (; PI != Cur->pred_end(); ++PI) {
      if (SFInfo.In != (*PI)->SFInfo.Out) {
        SFInfo.In = 0;
        break;
      }
    }
  }

  updateOut(Cur->SFInfo);

  return PrevIn != SFInfo.In || PrevOut != SFInfo.Out;
}

/// \brief Updates the out set based on the gen and kill sets.
void ElideCilkSync::updateOut(StackFrameInfo &Info) {
  // Out(B) = (In(B) U Gen(B)) \ Kill(B)
  Info.Out = Info.In;
  if (Info.Gen)
    Info.Out = Info.Gen;

  if (Info.Kill == Info.Out)
    Info.Out = 0;
}

/// \brief Returns the call to the sync/spawn in the basic block after the
/// instruction pointed to by BBI, or null if there is none.
Instruction *ElideCilkSync::getNextInstr(BasicBlock::iterator &BBI,
                                         bool &IsKill) {
  Instruction *I = &(*BBI);
  BasicBlock *BB = I->getParent();

  for (; BBI != BB->end(); ++BBI) {
    I = &(*BBI);

    IsKill = isSpawn(I);

    if (isSpawn(I) || isSync(I)) {
      ++BBI;
      return I;
    }
  }

  return 0;
}

/// \brief Inserts a node pointing to V into the Cilk-specific CFG. If V
/// is already in the CFG, update its predecessors to include Parent.
/// NewNode will be set to the CFG node. Returns true if a node was inserted,
/// false if the node existed already.
bool ElideCilkSync::insertOrUpdateCilkCFG(Value *V,
                                          const Value *SF,
                                          CilkCFGNode *Parent,
                                          CilkCFGNode *&NewNode) {
  DenseMap<Value*, CilkCFGNode*>::iterator It = InstrToCilkCFG.find(V);
  if (It != InstrToCilkCFG.end()) {
    // Already exists, update the node's predecessors
    NewNode = It->second;
    NewNode->addPred(Parent);
    return false;
  }

  // Never saw this instruction/BB before, create a new node
  NewNode = new CilkCFGNode(V, Parent, SF);
  InstrToCilkCFG[V] = NewNode;
  return true;
}

/// \brief Sets StackFrame to the unique Cilk stack frame in the given
/// function. Returns true if there is at least one stack frame,
/// false otherwise.
bool ElideCilkSync::setStackFrame(const Function *F) {
  typedef SmallPtrSet<const Function *, 2>::iterator IterTy;
  for (IterTy I = SyncFuncs.begin(), E = SyncFuncs.end(); I != E; ++I) {
    StackFrame = 0;
    for (Value::const_use_iterator UI = (*I)->use_begin(), UE = (*I)->use_end();
                                   UI != UE; ++UI) {
      assert(isa<CallInst>(*UI) && "sync use not a call instruction");
      const CallInst *CI = cast<CallInst>(*UI);
      if (CI->getParent()->getParent() == F) {
        const Value *CurStackFrame = getStackFrame(CI);
        assert((StackFrame == 0 || StackFrame == CurStackFrame) &&
               "More than one stack frame in the function");
        // Do not return immediately, continue to check if there are multiple
        // stack frames in this function.
        StackFrame = CurStackFrame;
      }
    }

    if (StackFrame)
      return true;
  }

  return false;
}

/// \brief Returns the stack frame argument passed to the call to CilkFn.
const Value *ElideCilkSync::getStackFrame(const CallInst *CilkFn) const {
  assert(CilkFn->getNumArgOperands() > 0 &&
         "Cilk function must at least 1 argument");
  return CilkFn->getArgOperand(0);
}
