//===--- DeclIntel.h - Types for Pragma SIMD -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_INTEL_DECL_H
#define LLVM_CLANG_AST_INTEL_DECL_H

#if INTEL_SPECIFIC_CILKPLUS

#include "clang/AST/Stmt.h"
#include "clang/AST/Decl.h"

namespace clang {
class CilkSpawnDecl : public Decl {
  /// \brief The CapturedStmt associated to the expression or statement with
  /// a Cilk spawn call.
  CapturedStmt *CapturedSpawn;

  CilkSpawnDecl(DeclContext *DC, CapturedStmt *Spawn);

public:
  static CilkSpawnDecl *Create(ASTContext &C, DeclContext *DC,
                               CapturedStmt *Spawn);
  static CilkSpawnDecl *CreateDeserialized(ASTContext &C, unsigned ID);

  /// \brief Returns if this Cilk spawn has a receiver.
  bool hasReceiver() const;

  /// \brief Returns the receiver declaration.
  VarDecl *getReceiverDecl() const;

  /// \brief Returns the expression or statement with a Cilk spawn.
  Stmt *getSpawnStmt();
  const Stmt *getSpawnStmt() const {
    return const_cast<CilkSpawnDecl *>(this)->getSpawnStmt();
  }

  /// \brief Returns the associated CapturedStmt.
  CapturedStmt *getCapturedStmt() { return CapturedSpawn; }
  const CapturedStmt *getCapturedStmt() const { return CapturedSpawn; }

  static bool classof(const Decl *D) { return classofKind(D->getKind()); }
  static bool classofKind(Kind K) { return K == CilkSpawn; }

  friend class ASTDeclReader;
  friend class ASTDeclWriter;
};
} // end namespace clang

#endif // INTEL_SPECIFIC_CILKPLUS
#endif // LLVM_CLANG_AST_INTEL_DECL_H
