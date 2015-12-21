//===----- CilkJumpBuffer.h - CilkPlus jmp_buf implementation -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// \brief This files hides all details of jmp_buf implementation on different
/// platforms.
///
//===----------------------------------------------------------------------===//
#include "CodeGenFunction.h"

/* Non-Windows - only need 5 registers for the jump buffer */
enum class gcc_cilk_jmpbuf {
  JMPBUF_FP,
  JMPBUF_PC,
  JMPBUF_SP,
};

/* Windows - things are a little more complicated */
enum class win64_cilk_jmpbuf {
  Frame,
  Rbx,
  JMPBUF_SP, // Rsp
  JMPBUF_FP, // Rbp
  Rsi,
  Rdi,
  R12,
  R13,
  R14,
  R15,
  JMPBUF_PC, // Rip
  MxCsr,
  FpCsr,
  Spare,
};

enum class win32_cilk_jmpbuf {
  JMPBUF_FP, // Ebp
  Ebx,
  Edi,
  Esi,
  JMPBUF_SP, // Ebp
  JMPBUF_PC, // Eip
  Registration,
  TryLevel
};

enum TargetPlatform {
  TP_GCC,   // it's a default platform:
  TP_WIN32, //    if !TP_WIN32 && !TP_WIN64
  TP_WIN64  //    then TP_GCC
};

static inline TargetPlatform
GetTargetPlatform(clang::CodeGen::CodeGenFunction &CGF) {
  if (CGF.getTarget().getTriple().isKnownWindowsMSVCEnvironment()) {
    if (CGF.getTarget().getTriple().getArch() == llvm::Triple::x86_64)
      return TP_WIN64;
    else
      return TP_WIN32;
  } else
    return TP_GCC;
}

static inline int GetJmpBuf_SP(clang::CodeGen::CodeGenFunction &CGF) {
  switch (GetTargetPlatform(CGF)) {
  case TP_GCC:
    return (int)gcc_cilk_jmpbuf::JMPBUF_SP;
    break;
  case TP_WIN32:
    return (int)win32_cilk_jmpbuf::JMPBUF_SP;
    break;
  case TP_WIN64:
    return (int)win64_cilk_jmpbuf::JMPBUF_SP;
    break;
  }
  return -1;
}

static inline int GetJmpBuf_FP(clang::CodeGen::CodeGenFunction &CGF) {
  switch (GetTargetPlatform(CGF)) {
  case TP_GCC:
    return (int)gcc_cilk_jmpbuf::JMPBUF_FP;
    break;
  case TP_WIN32:
    return (int)win32_cilk_jmpbuf::JMPBUF_FP;
  case TP_WIN64:
    return (int)win64_cilk_jmpbuf::JMPBUF_FP;
  }
  return -1;
}

