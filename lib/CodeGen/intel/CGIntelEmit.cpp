//===--- CGDecl.cpp - Emit LLVM Code for declarations ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This contains code to emit Decl nodes as LLVM code.
//
//===----------------------------------------------------------------------===//

#include "CodeGenFunction.h"
#include "CGDebugInfo.h"
#include "CGOpenCLRuntime.h"
#include "CodeGenModule.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/CharUnits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclObjC.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/CGFunctionInfo.h"
#include "clang/Frontend/CodeGenOptions.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Type.h"

#if INTEL_SPECIFIC_CILKPLUS

using namespace clang;
using namespace CodeGen;

/// EmitCaptureReceiverDecl - Emit allocation and cleanup code for
/// a receiver declaration in a captured statement. The initialization
/// is emitted in the helper function.
void CodeGenFunction::EmitCaptureReceiverDecl(const VarDecl &D) {
#ifndef NDEBUG
  const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(CurFuncDecl);
  assert(FD && FD->isSpawning() && "unexpected function declaration");
#endif
  AutoVarEmission Emission = EmitAutoVarAlloca(D);
  EmitAutoVarCleanups(Emission);
}
#endif // INTEL_SPECIFIC_CILKPLUS
