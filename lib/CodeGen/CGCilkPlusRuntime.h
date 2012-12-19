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

class CilkSpawnStmt;
class CilkSpawnCapturedStmt;
class CilkSyncStmt;

namespace CodeGen {

class CodeGenFunction;
class CodeGenModule;

/// \brief Implements Cilk Plus runtime specific code generation functions.
class CGCilkPlusRuntime {
private:
  CodeGenModule &CGM;

public:
  CGCilkPlusRuntime(CodeGenModule &CGM);

  ~CGCilkPlusRuntime();

  void EmitCilkSpawn(CodeGenFunction &CGF, const CilkSpawnStmt &E);

  void EmitCilkSpawn(CodeGenFunction &CGF, const CilkSpawnCapturedStmt &S);

  void EmitCilkSync(CodeGenFunction &CGF, const CilkSyncStmt &S);

  void EmitCilkParentStackFrame(CodeGenFunction &CGF);

  void EmitCilkHelperStackFrame(CodeGenFunction &CGF);

  void EmitCilkHelperPrologue(CodeGenFunction &CGF);
};

/// \brief Creates an instance of a Cilk Plus runtime object.
CGCilkPlusRuntime *CreateCilkPlusRuntime(CodeGenModule &CGM);

} // namespace CodeGen
} // namespace clang

#endif
