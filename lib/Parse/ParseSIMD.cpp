//===--- ParseSIMD.cpp - simd loop parsing --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements parsing of simd loops.
//
//===----------------------------------------------------------------------===//

#include "clang/Parse/Parser.h"
#include "RAIIObjectsForParser.h"

using namespace clang;

typedef llvm::SmallVector<DeclarationNameInfo, 5> SIMDVariableList;

template <typename T>
bool ParseSIMDListItems(Parser &P, Sema &S, T &ItemParser) {
  const Token &Tok = P.getCurToken();

  while (1) {
    bool ParsedItem = ItemParser.Parse(P, S);

    if (!ParsedItem) {
      P.SkipUntil(tok::comma, tok::r_paren, tok::annot_pragma_simd_end,
                  false, // StopAtSemi
                  true); // DontConsume
    }

    if (Tok.is(tok::r_paren) || Tok.is(tok::annot_pragma_simd_end))
      return ParsedItem;

    if (Tok.isNot(tok::comma))
      P.Diag(Tok, diag::err_expected_comma);
    else
      P.ConsumeToken();
  }
}

// Parse a list of the form:
//   '(' { item ',' }* item ')'
template <typename T>
bool ParseSIMDList(Parser &P, Sema &S, T &ItemParser) {
  BalancedDelimiterTracker BDT(P, tok::l_paren);
  if (BDT.expectAndConsume(diag::err_expected_lparen))
    return false;

  bool ParsedLastItem = ParseSIMDListItems(P, S, ItemParser);

  const Token &Tok = P.getCurToken();
  assert(Tok.is(tok::annot_pragma_simd_end) || Tok.is(tok::r_paren));

  if (Tok.is(tok::annot_pragma_simd_end)) {
    if (ParsedLastItem)
      P.Diag(Tok, diag::err_expected_rparen);
    return false;
  } else {
    BDT.consumeClose();
    return !BDT.getCloseLocation().isInvalid();
  }
}

template <typename T>
static bool ParseSIMDExpression(Parser &P, Sema &S, T &ItemParser) {
  BalancedDelimiterTracker BDT(P, tok::l_paren);
  if (BDT.expectAndConsume(diag::err_expected_lparen))
    return false;

  bool ParsedItem = ItemParser.Parse(P, S);
  if (!ParsedItem) {
    P.SkipUntil(tok::r_paren, tok::annot_pragma_simd_end,
                false, // StopAtSemi
                true); // DontConsume
    return false;
  }

  const Token &Tok = P.getCurToken();
  if (Tok.isNot(tok::r_paren)) {
    if (ParsedItem) {
      P.Diag(Tok, diag::err_expected_rparen);
      P.Diag(BDT.getOpenLocation(), diag::note_matching) << "(";
      P.SkipUntil(tok::annot_pragma_simd_end, false, true);
    }
    return false;
  } else {
    BDT.consumeClose();
    return !BDT.getCloseLocation().isInvalid();
  }
}

namespace {

// Helper to parse an item of the vectorlength clause.
struct SIMDVectorLengthItemParser {
  Expr *E;
  bool Parse(Parser &P, Sema &S) {
    ExprResult C = P.ParseConstantExpression();
    if (C.isInvalid())
      return false;
    E = C.get();
    return E;
  }
};

struct SIMDVectorLengthForItemParser {
  QualType T;
  bool Parse(Parser &P, Sema &S) {
    TypeResult TN = P.ParseTypeName();
    if (TN.isInvalid())
      return false;
    T = TN.get().get();
    return true;
  }
};

// Helper to parse an item of the linear clause.
struct SIMDLinearItemParser {
  bool Parse(Parser &P, Sema &S) {
    CXXScopeSpec SS;
    SourceLocation TemplateKWLoc;
    UnqualifiedId Name;
    if (P.ParseUnqualifiedId(SS, 
                             false, // EnteringContext
                             false, // AllowDestructorName
                             false, // AllowConstructorName,
                             ParsedType(), TemplateKWLoc, Name)) {
      return false;
    }
    const Token &Tok = P.getCurToken();
    if (Tok.is(tok::colon)) {
      P.ConsumeToken();
      ExprResult C = P.ParseConstantExpression();
      if (C.isInvalid())
        return false;
    }
    return true;
  }
};

// Helper to parse an item of the [first, last]private clauses.
struct SIMDPrivateItemParser {
  bool Parse(Parser &P, Sema &S) {
    CXXScopeSpec SS;
    SourceLocation TemplateKWLoc;
    UnqualifiedId Name;
    if (P.ParseUnqualifiedId(SS, 
                             false, // EnteringContext
                             false, // AllowDestructorName
                             false, // AllowConstructorName,
                             ParsedType(), TemplateKWLoc, Name)) {
      return false;
    }
    return true;
  }
};

// Helper to parse an item of the reduction clause.
struct SIMDReductionItemParser {
  bool ParsedOperator;

  SIMDReductionItemParser()
  : ParsedOperator(false) {
  }

  bool Parse(Parser &P, Sema &S) {
    if (!ParsedOperator) {
      ParsedOperator = true;
      const Token &Tok = P.getCurToken();
      switch (Tok.getKind()) {
      case tok::plus:
      case tok::star:
      case tok::minus:
      case tok::amp:
      case tok::pipe:
      case tok::caret:
      case tok::ampamp:
      case tok::pipepipe:
        P.ConsumeToken();
        break;
      default:
        P.Diag(Tok, diag::err_simd_expected_reduction_operator);
        return false;
      }

      if (Tok.is(tok::colon))
        P.ConsumeToken();
      else {
        P.Diag(Tok, diag::err_expected_colon);
        return false;
      }
    }
    CXXScopeSpec SS;
    SourceLocation TemplateKWLoc;
    UnqualifiedId Name;
    if (P.ParseUnqualifiedId(SS, 
                             false, // EnteringContext
                             false, // AllowDestructorName
                             false, // AllowConstructorName,
                             ParsedType(), TemplateKWLoc, Name)) {
      return false;
    }
    return true;
  }
};

} // namespace

static bool ParseSIMDClauses(Parser &P, Sema &S, SourceLocation BeginLoc,
                             SmallVector<const Attr *, 4> &AttrList) {
  const Token &Tok = P.getCurToken();

  while (Tok.isNot(tok::annot_pragma_simd_end)) {
    const IdentifierInfo *II = Tok.getIdentifierInfo();
    if (!II) {
      return false;
    }

    AttrResult A = AttrEmpty();
    if (II->isStr("vectorlength")) {
      P.ConsumeToken();
      SIMDVectorLengthItemParser ItemParser;
      if (!ParseSIMDExpression(P, S, ItemParser) || !ItemParser.E)
        return false;
      A = S.ActOnPragmaSIMDLength(BeginLoc, ItemParser.E);
    }
    else if (II->isStr("vectorlengthfor")) {
      P.ConsumeToken();
      SIMDVectorLengthForItemParser ItemParser;
      if (!ParseSIMDExpression(P, S, ItemParser))
        return false;
      A = S.ActOnPragmaSIMDLengthFor(BeginLoc, ItemParser.T);
    }
    else if (II->isStr("linear")) {
      P.ConsumeToken();
      SIMDLinearItemParser ItemParser;
      if (!ParseSIMDList(P, S, ItemParser))
        return false;
    }
    else if (II->isStr("private")) {
      P.ConsumeToken();
      SIMDPrivateItemParser ItemParser;
      if (!ParseSIMDList(P, S, ItemParser))
        return false;
    }
    else if (II->isStr("firstprivate")) {
      P.ConsumeToken();
      SIMDPrivateItemParser ItemParser;
      if (!ParseSIMDList(P, S, ItemParser))
        return false;
    }
    else if (II->isStr("lastprivate")) {
      P.ConsumeToken();
      SIMDPrivateItemParser ItemParser;
      if (!ParseSIMDList(P, S, ItemParser))
        return false;
    }
    else if (II->isStr("reduction")) {
      P.ConsumeToken();
      SIMDReductionItemParser ItemParser;
      if (!ParseSIMDList(P, S, ItemParser))
        return false;
    }
    else {
      P.Diag(Tok, diag::err_simd_invalid_clause);
      return false;
    }

    if (A.isInvalid()) {
      return false;
    }
    AttrList.push_back(A.get());
  }

  return true;
}

void Parser::HandlePragmaSIMD() {
  assert(Tok.is(tok::annot_pragma_simd));
  SourceLocation Loc = Tok.getLocation();
  SkipUntil(tok::annot_pragma_simd_end);
  PP.Diag(Loc, diag::warn_pragma_simd_expected_loop);
}

static void FinishPragmaSIMD(Parser &P, SourceLocation BeginLoc) {
  const Token &Tok = P.getCurToken();
  assert(Tok.is(tok::annot_pragma_simd_end));
  SourceLocation EndLoc = Tok.getLocation();

  const SourceManager &SM = P.getPreprocessor().getSourceManager();
  unsigned StartLineNumber = SM.getSpellingLineNumber(BeginLoc);
  unsigned EndLineNumber = SM.getSpellingLineNumber(EndLoc);
  assert(StartLineNumber == EndLineNumber);

  P.ConsumeToken();
}

/// \brief Parse a pragma simd directive.
///
/// '#' 'pragma' 'simd' simd-clauses[opt] new-line for-statement
///
/// simd-clauses:
///   simd-clause
///   simd-clauses simd-clause
///
/// simd-clause:
///   vectorlength-clause
///   linear-clause
///   openmp-data-clause
///
/// vectorlength-clause:
///   'vectorlength' '(' constant-expression ')'
///   'vectorlengthfor' '(' type-name ')' (deprecated)
///
/// linear-clause:
///   'linear' '(' linear-variable-list ')'
///
/// linear-variable-list:
///   linear-variable
///   linear-variable-list ',' linear-variable
///
/// linear-variable:
///   id-expression
///   id-expression ':' linear-step
///
/// linear-step:
///   conditional-expression
///
/// openmp-data-clause:
///   private-clause
///   firstprivate-clause (deprecated)
///   lastprivate-clause
///   reduction-clause
///
StmtResult Parser::HandlePragmaSIMDStatementOrDeclaration() {
  assert(Tok.is(tok::annot_pragma_simd));
  SourceLocation Loc = PP.getDirectiveHashLoc();
  ConsumeToken();

  SmallVector<const Attr *, 4> SIMDAttrList;
  SIMDAttrList.push_back(::new SIMDAttr(Loc, Actions.Context));

  if (!ParseSIMDClauses(*this, Actions, Loc, SIMDAttrList)) {
    SkipUntil(tok::annot_pragma_simd_end, false, true);
    FinishPragmaSIMD(*this, Loc);
    return StmtError();
  }

  FinishPragmaSIMD(*this, Loc);

  if (Tok.is(tok::r_brace)) {
    PP.Diag(Loc, diag::warn_pragma_simd_expected_loop);
    return StmtEmpty();
  }

  StmtVector Stmts;
  StmtResult R(ParseStatementOrDeclaration(Stmts, false));
  if (R.isInvalid())
    return R;

  return Actions.ActOnPragmaSIMD(Loc, R.get(), SIMDAttrList);
}
