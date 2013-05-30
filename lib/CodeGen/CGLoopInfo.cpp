//===---- CGLoopInfo.cpp - LLVM CodeGen for loop metadata -*- C++ -*-------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "CGLoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"

using namespace clang;
using namespace CodeGen;

static llvm::MDNode *MakeMetadata(llvm::LLVMContext &Ctx, 
                                  llvm::StringRef Name, unsigned V) {
  using namespace llvm;
  Value *Args[] = { MDString::get(Ctx, Name),
                    ConstantInt::get(Type::getInt32Ty(Ctx), V) };
  return MDNode::get(Ctx, Args);
}

static llvm::MDNode *CreateMetadata(llvm::LLVMContext &Ctx,
                                    const LoopAttributes &Attrs) {
  using namespace llvm;

  SmallVector<Value*, 4> Args;
  // Reserve operand 0 for loop id self reference.
  Args.push_back(0);

  if (Attrs.VectorizerWidth > 0)
    Args.push_back(MakeMetadata(Ctx, "llvm.vectorizer.width",
                                Attrs.VectorizerWidth));

  if (!Attrs.IsParallel &&
      Args.size() < 2)
    return 0;

  assert(Args.size() > 0);
  assert(Args[0] == 0);
  MDNode *LoopID = MDNode::get(Ctx, Args);
  LoopID->replaceOperandWith(0, LoopID);
  return LoopID;
}

LoopAttributes::LoopAttributes() : IsParallel(false), VectorizerWidth(0) {
}

void LoopAttributes::Clear() {
  IsParallel = false;
  VectorizerWidth = 0;
}

LoopInfo::LoopInfo(llvm::BasicBlock *Header, const LoopAttributes &Attrs)
  : LoopID(0), Header(Header), Attrs(Attrs) {
  LoopID = CreateMetadata(Header->getContext(), Attrs);
}

void LoopInfoStack::Push(llvm::BasicBlock *Header) {
  Active.push_back(LoopInfo(Header, StagedAttrs));
  // Clear the attributes so nested loops do not inherit them.
  StagedAttrs.Clear();
}

void LoopInfoStack::Pop() {
  assert(!Active.empty());
  Active.pop_back();
}

void LoopInfoStack::InsertHelper(llvm::Instruction *I) const {
  if (!HasInfo())
    return;

  const LoopInfo &L = GetInfo();

  if (!L.GetLoopID())
    return;

  if (llvm::TerminatorInst *TI = llvm::dyn_cast<llvm::TerminatorInst>(I)) {
    for (unsigned i = 0, ie = TI->getNumSuccessors(); i < ie; ++i)
      if (TI->getSuccessor(i) == L.GetHeader()) {
        TI->setMetadata("llvm.loop", L.GetLoopID());
        break;
      }
    return;
  }

  if (L.GetAttributes().IsParallel) {
    if (llvm::StoreInst *SI = llvm::dyn_cast<llvm::StoreInst>(I))
      SI->setMetadata("llvm.mem.parallel_loop_access", L.GetLoopID());
    else if (llvm::LoadInst *LI = llvm::dyn_cast<llvm::LoadInst>(I))
      LI->setMetadata("llvm.mem.parallel_loop_access", L.GetLoopID());
  }
}
