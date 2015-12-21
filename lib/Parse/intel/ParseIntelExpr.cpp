//===--- ParseExpr.cpp - Expression Parsing -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Provides the Expression parsing implementation.
///
/// Expressions in C99 basically consist of a bunch of binary operators with
/// unary operators and other random stuff at the leaves.
///
/// In the C99 grammar, these unary operators bind tightest and are represented
/// as the 'cast-expression' production.  Everything else is either a binary
/// operator (e.g. '/') or a ternary operator ("?:").  The unary leaves are
/// handled by ParseCastExpression, the higher level pieces are handled by
/// ParseBinaryExpression.
///
//===----------------------------------------------------------------------===//

#include "clang/Parse/Parser.h"
#include "RAIIObjectsForParser.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/PrettyStackTrace.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/ParsedTemplate.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/TypoCorrection.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
using namespace clang;

#if INTEL_SPECIFIC_CILKPLUS
/// ParseSecreduceExpressionList - Used for C/C++ (argument-)expression-list.
///
/// \verbatim
///       argument-expression-list:
///         assignment-expression
///         argument-expression-list , assignment-expression
///
/// [C++] expression-list:
/// [C++]   assignment-expression
/// [C++]   expression-list , assignment-expression
///
/// [C++0x] expression-list:
/// [C++0x]   initializer-list
///
/// [C++0x] initializer-list
/// [C++0x]   initializer-clause ...[opt]
/// [C++0x]   initializer-list , initializer-clause ...[opt]
///
/// [C++0x] initializer-clause:
/// [C++0x]   assignment-expression
/// [C++0x]   braced-init-list
/// \endverbatim
bool Parser::ParseSecReduceExpressionList(
    SmallVectorImpl<Expr *> &Exprs, SmallVectorImpl<SourceLocation> &CommaLocs,
    bool CheckReturnType,
    void (Sema::*Completer)(Scope *S, Expr *Data, ArrayRef<Expr *> Args),
    Expr *Data) {
  unsigned ArgCounter = 0;
  while (1) {
    if (Tok.is(tok::code_completion)) {
      if (Completer)
        (Actions.*Completer)(getCurScope(), Data, Exprs);
      else
        Actions.CodeCompleteOrdinaryName(getCurScope(), Sema::PCC_Expression);
      cutOffParsing();
      return true;
    }

    ExprResult Expr = ExprError();
    if (ArgCounter == 2 && getLangOpts().CPlusPlus &&
        Exprs[0]->getType().getCanonicalType()->isStructureOrClassType()) {
      TentativeParsingAction PA(*this);
      CXXScopeSpec SS;
      if (ParseOptionalCXXScopeSpecifier(SS, ParsedType(), false)) {
        PA.Commit();
        return true;
      }
      // Ttrying to locate methods in base class.
      CXXRecordDecl *CXXRD =
          Exprs[0]->getType().getCanonicalType()->getAsCXXRecordDecl();
      QualType ClassType =
          Exprs[0]->getType().getNonReferenceType().getCanonicalType();
      // Trying to find method T.<Tok>(T arg)
      // SS.clear();
      SourceLocation TemplateKWLoc;
      UnqualifiedId Name;
      // TentativeParsingAction PA(*this);
      if (ParseUnqualifiedId(SS, false, false, false, ParsedType(),
                             TemplateKWLoc, Name)) {
        PA.Commit();
        return true;
      }
      LookupResult LRes(Actions, Actions.GetNameFromUnqualifiedId(Name),
                        Sema::LookupOrdinaryName);
      LRes.suppressDiagnostics();
      Actions.LookupQualifiedName(LRes, CXXRD);
      if (!LRes.empty()) {
        SmallVector<NamedDecl *, 16> FoundDecls;
        for (LookupResult::iterator I = LRes.begin(), E = LRes.end(); I != E;
             ++I) {
          NamedDecl *D = *I;
          if (CXXMethodDecl *CXXMD = dyn_cast<CXXMethodDecl>(D)) {
            if (CXXMD->isInstance() && !CXXMD->isPure() &&
                !CXXMD->isDeleted() && !CXXMD->getPrimaryTemplate() &&
                !CXXMD->getDescribedFunctionTemplate() &&
                CXXMD->getNumParams() == 1 &&
                Actions.CheckMemberAccess(Tok.getLocation(), CXXRD,
                                          I.getPair()) == Sema::AR_accessible) {
              QualType ParamType =
                  CXXMD->getParamDecl(0)->getType().getCanonicalType();
              QualType ResType = CXXMD->getReturnType().getCanonicalType();
              if (Actions.getASTContext().hasSameUnqualifiedType(ClassType,
                                                                 ParamType) &&
                  (!CheckReturnType ||
                   Actions.getASTContext().hasSameUnqualifiedType(ClassType,
                                                                  ResType)))
                FoundDecls.push_back(D);
            }
          }
        }
        if (FoundDecls.size() > 0) {
          PA.Commit();
          SS.Extend(Actions.getASTContext(), SourceLocation(),
                    Actions.getASTContext()
                        .getTrivialTypeSourceInfo(ClassType)
                        ->getTypeLoc(),
                    SourceLocation());
          Expr = Actions.ActOnIdExpression(getCurScope(), SS, TemplateKWLoc,
                                           Name, false, true);
          if (Expr.isInvalid())
            return true;
        } else
          PA.Revert();
      } else
        PA.Revert();
    }

    if (Expr.isInvalid()) {
      if (getLangOpts().CPlusPlus11 && Tok.is(tok::l_brace)) {
        Diag(Tok, diag::warn_cxx98_compat_generalized_initializer_lists);
        Expr = ParseBraceInitializer();
      } else
        Expr = ParseAssignmentExpression();

      if (Tok.is(tok::ellipsis))
        Expr = Actions.ActOnPackExpansion(Expr.get(), ConsumeToken());
      if (Expr.isInvalid())
        return true;
    }

    Exprs.push_back(Expr.get());

    if (Tok.isNot(tok::comma))
      return false;
    // Move to the next argument, remember where the comma was.
    CommaLocs.push_back(ConsumeToken());
    ++ArgCounter;
  }
}
#endif // INTEL_SPECIFIC_CILKPLUS
