//===----- CGCilkPlusRuntime.h - Interface to CilkPlus Runtimes ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This provides Cilk Plus code generation. 
//
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
class CilkSyncStmt;
class CilkSpawnCapturedStmt;

namespace CodeGen {

class CodeGenFunction;
class CodeGenModule;

/// Implements runtime-specific code generation functions.
class CGCilkPlusRuntime {
private:
  CodeGenModule &CGM;

public:
  CGCilkPlusRuntime(CodeGenModule &CGM);
  virtual ~CGCilkPlusRuntime();

  virtual void EmitCilkSpawn(CodeGenFunction &CGF, const CilkSpawnStmt &E);

  virtual void EmitCilkSpawn(CodeGenFunction &CGF,
                             const CilkSpawnCapturedStmt &S);

  virtual void EmitCilkSync(CodeGenFunction &CGF, const CilkSyncStmt &S);
  //virtual void EmitCilkFor(CodeGenFunction &CGF, const CilkForStmt &S);
};

/// Creates an instance of a CilkPlus runtime class.
CGCilkPlusRuntime *CreateCilkPlusRuntime(CodeGenModule &CGM);
/// Creates an instance of a fake CilkPlus runtime class
/// which serializes spawn and sync.
CGCilkPlusRuntime *CreateCilkPlusFakeRuntime(CodeGenModule &CGM);

} // namespace CodeGen
} // namespace clang

#endif
