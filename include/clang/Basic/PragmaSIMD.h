//===--- PragmaSIMD.h - Types for Pragma SIMD -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_CLANG_BASIC_PRAGMA_SIMD_H
#define LLVM_CLANG_BASIC_PRAGMA_SIMD_H

namespace clang {

/// \brief SIMD data-privatization variable kinds.
enum SIMDVariableKind {
  SIMD_VK_Unknown      = 0x00,
  SIMD_VK_Private      = 0x01,
  SIMD_VK_LastPrivate  = 0x02,
  SIMD_VK_FirstPrivate = 0x04,
  SIMD_VK_Linear       = 0x08,
  SIMD_VK_Reduction    = 0x10
};

} // end namespace clang

#endif // LLVM_CLANG_BASIC_PRAGMA_SIMD_H
