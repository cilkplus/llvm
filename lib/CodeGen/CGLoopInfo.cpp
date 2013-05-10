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

LoopAttributes::LoopAttributes() : IsParallel(false) {
}

void LoopAttributes::Clear() {
  IsParallel = false;
}

LoopInfo::LoopInfo(llvm::BasicBlock *Header, const LoopAttributes &Attrs)
  : LoopID(0), Header(Header), Attrs(Attrs) {
}

llvm::MDNode *LoopInfo::GetLoopID() const {
  if (LoopID) return LoopID;
  // Create a loop id metadata of the form '!n = metadata !{metadata !n}'
  LoopID = llvm::MDNode::get(Header->getContext(), 0);
  LoopID->replaceOperandWith(0, LoopID);
  return LoopID;
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

const LoopInfo &LoopInfoStack::GetInfo() const {
  assert(HasInfo());
  return Active.back();
}

