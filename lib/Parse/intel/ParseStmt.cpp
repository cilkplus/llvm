//===--- ParseStmt.cpp - Statement and Block Parser -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Statement and Block portions of the Parser
// interface.
//
//===----------------------------------------------------------------------===//

#if INTEL_SPECIFIC_CILKPLUS
#include "clang/Basic/intel/StmtIntel.h"
#include "clang/Parse/Parser.h"
#include "RAIIObjectsForParser.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/Attributes.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/PrettyStackTrace.h"
#include "clang/Sema/DeclSpec.h"
#include "clang/Sema/LoopHint.h"
#include "clang/Sema/PrettyDeclStackTrace.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/TypoCorrection.h"
#include "llvm/ADT/SmallString.h"
#include "clang/AST/StmtCXX.h"
using namespace clang;

StmtResult Parser::ParsePragmaCilkGrainsize() {
  assert(getLangOpts().CilkPlus && "Cilk Plus extension not enabled");
  SourceLocation HashLoc = ConsumeToken(); // Eat 'annot_pragma_cilk_grainsize_begin'.

  ExprResult E = ParseExpression();
  if (E.isInvalid()) {
    SkipUntil(tok::annot_pragma_cilk_grainsize_end);
    return StmtError();
  }

  if (Tok.isNot(tok::annot_pragma_cilk_grainsize_end)) {
    Diag(Tok, diag::warn_pragma_extra_tokens_at_eol) << "cilk";
    SkipUntil(tok::annot_pragma_cilk_grainsize_end);
  } else
    ConsumeToken(); // Eat 'annot_pragma_cilk_grainsize_end'.

  // Parse the following statement.
  StmtResult FollowingStmt(ParseStatement());
  if (FollowingStmt.isInvalid())
    return StmtError();

  // NOTE: The following statement is not necessarily a _Cilk_for statement.
  // It can also be another pragma that appertains to the _Cilk_for. However,
  // since grainsize is the only pragma supported by _Cilk_for, we require the
  // following statement to be a _Cilk_for.
  if (!isa<CilkForStmt>(FollowingStmt.get())) {
    Diag(FollowingStmt.get()->getLocStart(),
         diag::warn_cilk_for_following_grainsize);
    return FollowingStmt;
  }

  return Actions.ActOnCilkForGrainsizePragma(E.get(), FollowingStmt.get(), HashLoc);
}

/// ParseCilkForStatement
///       cilk-for-statement:
/// [C]     '_Cilk_for' '(' assignment-expr';' condition ';' expr ')' stmt
/// [C++]   '_Cilk_for' '(' for-init-stmt condition ';' expr ')' stmt
StmtResult Parser::ParseCilkForStmt() {
  assert(Tok.is(tok::kw__Cilk_for) && "Not a Cilk for stmt!");
  SourceLocation CilkForLoc = ConsumeToken();  // eat the '_Cilk_for'.

  if (Tok.isNot(tok::l_paren)) {
    Diag(Tok, diag::err_expected_lparen_after) << "_Cilk_for";
    SkipUntil(tok::semi);
    return StmtError();
  }

  bool C99orCXXorObjC = getLangOpts().C99 || getLangOpts().CPlusPlus ||
                        getLangOpts().ObjC1;

  // Start the loop scope.
  //
  // A program contains a return, break or goto statement that would transfer
  // control into or out of a _Cilk_for loop is ill-formed.
  //
  unsigned ScopeFlags = Scope::ContinueScope;
  if (C99orCXXorObjC)
    ScopeFlags |= Scope::DeclScope | Scope::ControlScope;

  ParseScope CilkForScope(this, ScopeFlags);

  BalancedDelimiterTracker T(*this, tok::l_paren);
  T.consumeOpen();

  if (Tok.is(tok::code_completion)) {
    Actions.CodeCompleteOrdinaryName(getCurScope(),
                                     C99orCXXorObjC ? Sema::PCC_ForInit
                                                    : Sema::PCC_Expression);
    cutOffParsing();
    return StmtError();
  }

  ParsedAttributesWithRange attrs(AttrFactory);
  MaybeParseCXX11Attributes(attrs);

  // '_Cilk_for' '(' for-init-stmt
  // '_Cilk_for' '(' assignment-expr;
  StmtResult FirstPart;

  if (Tok.is(tok::semi)) {  // _Cilk_for (;
    ProhibitAttributes(attrs);
    // no control variable declaration initialization, eat the ';'.
    Diag(Tok, diag::err_cilk_for_missing_control_variable);
    ConsumeToken();
  } else if (isForInitDeclaration()) {  // _Cilk_for (int i = 0;
    // Parse declaration, which eats the ';'.
    if (!C99orCXXorObjC)   // Use of C99-style for loops in C90 mode?
      Diag(Tok, diag::ext_c99_variable_decl_in_for_loop);

    SourceLocation DeclStart = Tok.getLocation();
    SourceLocation DeclEnd;

    // Still use Declarator::ForContext. A new enum item CilkForContext
    // may be needed for extra checks.

    DeclGroupPtrTy DG = ParseSimpleDeclaration(Declarator::ForContext,
                                               DeclEnd, attrs,
                                               /*RequireSemi*/false,
                                               /*ForRangeInit*/0);
    FirstPart = Actions.ActOnDeclStmt(DG, DeclStart, Tok.getLocation());

    if(Tok.is(tok::semi))
      ConsumeToken(); // Eat the ';'.
    else
      Diag(Tok, diag::err_cilk_for_missing_semi);
  } else {
    ProhibitAttributes(attrs);
    ExprResult E = ParseExpression();

    if (!E.isInvalid())
      FirstPart = Actions.ActOnExprStmt(E);

    if (Tok.is(tok::semi))
      ConsumeToken(); // Eat the ';'
    else if (!E.isInvalid())
      Diag(Tok, diag::err_cilk_for_missing_semi);
    else {
      // Skip until semicolon or rparen, don't consume it.
      SkipUntil(tok::r_paren, StopAtSemi | StopBeforeMatch);
      if (Tok.is(tok::semi))
        ConsumeToken();
    }
  }

  // '_Cilk_for' '(' for-init-stmt condition ;
  // '_Cilk_for' '(' assignment-expr; condition ;
  FullExprArg SecondPart(Actions);
  FullExprArg ThirdPart(Actions);

  // True while inside the Cilk for variable capturing region.
  bool InCapturingRegion = false;

  if (Tok.is(tok::r_paren)) { // _Cilk_for (...;)
    Diag(Tok, diag::err_cilk_for_missing_condition);
    Diag(Tok, diag::err_cilk_for_missing_increment);
  } else {
    if (Tok.is(tok::semi)) {  // _Cilk_for (...;;
      // No condition part.
      Diag(Tok, diag::err_cilk_for_missing_condition);
      ConsumeToken(); // Eat the ';'
    } else {
      ExprResult E = ParseExpression();
      if (!E.isInvalid())
        E = Actions.ActOnBooleanCondition(getCurScope(), CilkForLoc, E.get());
      SecondPart = Actions.MakeFullExpr(E.get(), CilkForLoc);

      if (Tok.isNot(tok::semi)) {
        if (!E.isInvalid())
          Diag(Tok, diag::err_cilk_for_missing_semi);
        else
          // Skip until semicolon or rparen, don't consume it.
          SkipUntil(tok::r_paren, StopAtSemi | StopBeforeMatch);
      }

      if (Tok.is(tok::semi))
        ConsumeToken();
    }

    // Parse the third part.
    if (Tok.is(tok::r_paren)) { // _Cilk_for (...;...;)
      // No increment part
      Diag(Tok, diag::err_cilk_for_missing_increment);
    } else {
      // Enter the variable capturing region for both Cilk for
      // increment and body.
      //
      // For the following Cilk for loop,
      //
      // _Cilk_for (T x = a;  x < b; x += c) {
      //   // body
      // }
      //
      // The outlined function would look like
      //
      // void helper(Context, low, high) {
      //   _index_var = low
      //   x += low * c
      //
      //   while (_index_var < high) {
      //     // body
      //     x += c                // loop increment
      //     _index_var++;
      //   }
      // }
      //
      // The loop increment would be part of the outlined function,
      // together with the loop body. Hence, we enter a capturing region
      // before parsing the loop increment.
      Actions.ActOnStartOfCilkForStmt(CilkForLoc, getCurScope(), FirstPart);
      InCapturingRegion = true;

      ExprResult E = ParseExpression();
      // FIXME: The C++11 standard doesn't actually say that this is a
      // discarded-value expression, but it clearly should be.
      ThirdPart = Actions.MakeFullDiscardedValueExpr(E.get());
    }
  }

  // Match the ')'.
  T.consumeClose();

  // Enter the variable capturing region for Cilk for body.
  if (!InCapturingRegion)
    Actions.ActOnStartOfCilkForStmt(CilkForLoc, getCurScope(), FirstPart);

  // Cannot use goto to exit the Cilk for body.
  ParseScope InnerScope(this, Scope::BlockScope | Scope::FnScope |
                              Scope::DeclScope | Scope::ContinueScope);

  // Read the body statement.
  StmtResult Body(ParseStatement());

  // Pop the body scope if needed.
  InnerScope.Exit();

  // Leave the for-scope.
  CilkForScope.Exit();

  if (!FirstPart.isUsable() || !SecondPart.get() || !ThirdPart.get() ||
      !Body.isUsable()) {
    Actions.ActOnCilkForStmtError();
    return StmtError();
  }

  if (Actions.CheckIfBodyModifiesLoopControlVar(Body.get())) {
    Actions.ActOnCilkForStmtError();
    return StmtError();
  }

  StmtResult Result = Actions.ActOnCilkForStmt(CilkForLoc, T.getOpenLocation(),
                                               FirstPart.get(), SecondPart,
                                               ThirdPart, T.getCloseLocation(),
                                               Body.get());
  if (Result.isInvalid()) {
    Actions.ActOnCilkForStmtError();
    return StmtError();
  }

  return Result;
}

/// \brief Parse Cilk Plus elemental function attribute clauses.
///
/// We need to special case the parsing due to the fact that a Cilk Plus
/// vector() attribute can contain arguments that themselves have arguments.
/// Each property is parsed and stored as if it were a distinct attribute.
/// This drastically simplifies parsing and Sema at the cost of re-grouping
/// the attributes in CodeGen.
///
/// elemental-clauses:
///   elemental-clause
///   elemental-clauses , elemental-clause
///
/// elemental-clause:
///   processor-clause
///   vectorlength-clause
///   elemental-uniform-clause
///   elemental-linear-clause
///   mask-clause
void Parser::ParseCilkPlusElementalAttribute(IdentifierInfo &AttrName,
                                             SourceLocation AttrNameLoc,
                                             ParsedAttributes &Attrs,
                                             SourceLocation *EndLoc,
                                             AttributeList::Syntax Syntax) {
  assert(Tok.is(tok::l_paren) && "Attribute arg list not starting with '('");

  IdentifierInfo *CilkScopeName = PP.getIdentifierInfo("_Cilk_elemental");

  // Add the 'vector' attribute itself.
  Attrs.addNew(&AttrName, AttrNameLoc, 0, AttrNameLoc,
               0, 0, AttributeList::AS_GNU);

  BalancedDelimiterTracker T(*this, tok::l_paren);
  T.consumeOpen();

  while (Tok.isNot(tok::r_paren)) {
    if (Tok.isNot(tok::identifier)) {
      Diag(Tok, diag::err_expected_ident);
      T.skipToEnd();
      return;
    }
    IdentifierInfo *SubAttrName = Tok.getIdentifierInfo();
    SourceLocation SubAttrNameLoc = ConsumeToken();

    if (Tok.is(tok::l_paren)) {
      if (SubAttrName->isStr("uniform") || SubAttrName->isStr("linear") ||
          SubAttrName->isStr("aligned")) {
        // These sub-attributes are parsed specially because their
        // arguments are function parameters not yet declared.
        ParseFunctionParameterAttribute(*SubAttrName, SubAttrNameLoc,
                                        Attrs, EndLoc, *CilkScopeName,
                                        AttrNameLoc);
      } else {
        ParseGNUAttributeArgs(SubAttrName, SubAttrNameLoc, Attrs, EndLoc,
                              CilkScopeName, AttrNameLoc,
                              AttributeList::AS_CXX11, nullptr);
      }
    } else {
      Attrs.addNew(SubAttrName, SubAttrNameLoc,
                   CilkScopeName, AttrNameLoc,
                   0, 0, AttributeList::AS_CXX11);
    }

    if (Tok.isNot(tok::comma))
      break;
    ConsumeToken(); // Eat the comma, move to the next argument
  }
  // Match the ')'.
  T.consumeClose();
  if (EndLoc)
    *EndLoc = T.getCloseLocation();
}

/// \brief Parse an identifer list.
///
/// We need to special case the parsing due to the fact that identifiers
/// may appear as attribute arguments before they appear as parameters
/// in a function declaration.
///
/// uniform-param-list:
///   parameter-name
///   uniform-param-list , parameter-name
///
/// elemental-linear-param-list:
///   elemental-linear-param
///   elemental-linear-param-list , elemental-linear-param
///
/// elemental-linear-param:
///   parameter-name
///   parameter-name : elemental-linear-step
///
/// elemental-linear-step:
///   constant-expression
///   parameter-name
///
/// parameter-name:
///   identifier
///   this
///
void Parser::ParseFunctionParameterAttribute(IdentifierInfo &AttrName,
                                             SourceLocation AttrNameLoc,
                                             ParsedAttributes &Attrs,
                                             SourceLocation *EndLoc,
                                             IdentifierInfo &ScopeName,
                                             SourceLocation ScopeLoc) {
  assert(Tok.is(tok::l_paren) && "Attribute arg list not starting with '('");

  BalancedDelimiterTracker T(*this, tok::l_paren);
  T.consumeOpen();

  // Now parse the list of identifiers or this.
  do {
    // Create a separate attribute for each identifier or this, in order to
    // be able to use the Parm field.
    IdentifierInfo *ParmName = 0;
    SourceLocation ParmLoc;

    if (getLangOpts().CPlusPlus && Tok.is(tok::kw_this)) {
      // Create a 'this' identifier if the current token is keyword 'this'.
      ParmName = PP.getIdentifierInfo("this");
      ParmLoc = ConsumeToken();
    } else if (Tok.isNot(tok::identifier)) {
      Diag(Tok, getLangOpts().CPlusPlus ? diag::err_expected_ident_or_this
                                        : diag::err_expected_ident);
      T.skipToEnd();
      return;
    } else {
      ParmName = Tok.getIdentifierInfo();
      ParmLoc = ConsumeToken();
    }
    IdentifierInfo *StepName = 0;
    ArgsVector ArgExprs;
    IdentifierLoc *IdArg = IdentifierLoc::create(Actions.getASTContext(),
                                                 ParmLoc,
                                                 ParmName);
    ArgExprs.push_back(IdArg);

    if (AttrName.isStr("linear") || AttrName.isStr("aligned")) {
      if (Tok.is(tok::colon)) {
        ConsumeToken();
        // The grammar is ambiguous for the linear step, which could be a
        // parameter name or a reference of a variable. To disambiguate it,
        // we do a one token look ahead and perform a name lookup when all
        // parameter names are available.
        //
        // If the linear step starts with an identifier and the following token
        // is ')' or ',', then the step could be a parameter name or a reference
        // to a declared variable. The second case will be left to Sema.
        const Token &Next = GetLookAheadToken(1);
        if (Tok.is(tok::identifier) && (Next.is(tok::r_paren) ||
                                        Next.is(tok::comma))) {
          StepName = Tok.getIdentifierInfo();
          IdArg = IdentifierLoc::create(Actions.getASTContext(),
                                        Tok.getLocation(),
                                        StepName);
          ArgExprs.push_back(IdArg);
          ConsumeToken();
        }

        // If StepName is not null, then the step must be a compile time
        // integer constant.
        if (!StepName) {
          ExprResult StepExpr = ParseConstantExpression();
          if (StepExpr.isInvalid()) {
            SkipUntil(tok::r_paren);
            return;
          }
          ArgExprs.push_back(StepExpr.get());
        }
      }
    }

    if (Tok.is(tok::ellipsis)) {
      SourceLocation EllipsisLoc = ConsumeToken();
      if (getLangOpts().CPlusPlus11) {
        Diag(EllipsisLoc, diag::err_elemental_parameter_pack_unsupported)
          << AttrName.getName();
        SkipUntil(tok::r_paren);
        return;
      }
    }

    Attrs.addNew(&AttrName, AttrNameLoc, &ScopeName, ScopeLoc,
                 ArgExprs.data(), ArgExprs.size(),
                 AttributeList::AS_CXX11);

    if (Tok.isNot(tok::comma))
      break;
    ConsumeToken(); // Eat the comma, move to the next argument
  } while (Tok.isNot(tok::r_paren));

  // Match the ')'.
  T.consumeClose();
  if (EndLoc)
    *EndLoc = T.getCloseLocation();
}
#endif // INTEL_SPECIFIC_CILKPLUS
