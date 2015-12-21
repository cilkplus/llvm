#include "RAIIObjectsForParser.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseDiagnostic.h"
#include "clang/Parse/Parser.h"
#include "clang/Sema/LoopHint.h"
#include "clang/Sema/Scope.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;

namespace {
#include "intel/ParsePragma.h"
}

#if INTEL_SPECIFIC_CILKPLUS
// #pragma simd
void PragmaSIMDHandler::HandlePragma(Preprocessor &PP,
                                     PragmaIntroducerKind Introducer,
                                     Token &FirstTok) {
  SmallVector<Token, 16> Pragma;
  Token Tok;
  Tok.startToken();
  Tok.setKind(tok::annot_pragma_simd);
  Tok.setLocation(FirstTok.getLocation());
        
  while (Tok.isNot(tok::eod)) {
    Pragma.push_back(Tok);
    PP.Lex(Tok);
  }
  SourceLocation EodLoc = Tok.getLocation();
  Tok.startToken();
  Tok.setKind(tok::annot_pragma_simd_end);
  Tok.setLocation(EodLoc);
  Pragma.push_back(Tok);
  Token *Toks = new Token[Pragma.size()];
  std::copy(Pragma.begin(), Pragma.end(), Toks);
  PP.EnterTokenStream(Toks, Pragma.size(),
                      /*DisableMacroExpansion=*/true, /*OwnsTokens=*/true);
}

/// \brief Handle Cilk Plus grainsize pragma.
///
/// #pragma 'cilk' 'grainsize' '=' expr new-line
///
void PragmaCilkGrainsizeHandler::HandlePragma(Preprocessor &PP,
                                              PragmaIntroducerKind Introducer,
                                              Token &FirstToken) {
  Token Tok;
  PP.Lex(Tok);
  if (Tok.isNot(tok::identifier)) {
    PP.Diag(Tok, diag::warn_pragma_expected_identifier) << "cilk";
    return;
  }

  IdentifierInfo *Grainsize = Tok.getIdentifierInfo();
  SourceLocation GrainsizeLoc = Tok.getLocation();

  if (!Grainsize->isStr("grainsize")) {
    PP.Diag(Tok, diag::err_cilk_for_expect_grainsize);
    return;
  }

  PP.Lex(Tok);
  if (Tok.isNot(tok::equal)) {
    PP.Diag(Tok, diag::err_cilk_for_expect_assign);
    return;
  }

  // Cache tokens after '=' and store them back to the token stream.
  SmallVector<Token, 5> CachedToks;
  while (true) {
    PP.Lex(Tok);
    if (Tok.is(tok::eod))
      break;
    CachedToks.push_back(Tok);
  }

  llvm::BumpPtrAllocator &Allocator = PP.getPreprocessorAllocator();
  unsigned Size = CachedToks.size();

  Token *Toks = (Token *) Allocator.Allocate(sizeof(Token) * (Size + 2),
                                             llvm::alignOf<Token>());
  Token &GsBeginTok = Toks[0];
  GsBeginTok.startToken();
  GsBeginTok.setKind(tok::annot_pragma_cilk_grainsize_begin);
  GsBeginTok.setLocation(FirstToken.getLocation());

  SourceLocation EndLoc = Size ? CachedToks.back().getLocation()
                               : GrainsizeLoc;

  Token &GsEndTok = Toks[Size + 1];
  GsEndTok.startToken();
  GsEndTok.setKind(tok::annot_pragma_cilk_grainsize_end);
  GsEndTok.setLocation(EndLoc);

  for (unsigned i = 0; i < Size; ++i)
    Toks[i + 1] = CachedToks[i];

  PP.EnterTokenStream(Toks, Size + 2, /*DisableMacroExpansion=*/true,
                      /*OwnsTokens=*/false);
}

void Parser::initializeIntelPragmaHandlers() {
  if (getLangOpts().CilkPlus) {
    CilkGrainsizeHandler.reset(new PragmaCilkGrainsizeHandler());
    PP.AddPragmaHandler(CilkGrainsizeHandler.get());
    SIMDHandler.reset(new PragmaSIMDHandler());
    PP.AddPragmaHandler(SIMDHandler.get());
  }
}

void Parser::resetIntelPragmaHandlers() {
  // Remove the pragma handlers we installed.
  if (getLangOpts().CilkPlus) {
    PP.RemovePragmaHandler(CilkGrainsizeHandler.get());
    CilkGrainsizeHandler.reset();
    PP.RemovePragmaHandler(SIMDHandler.get());
    SIMDHandler.reset(new PragmaSIMDHandler());
  }
}
#endif // INTEL_SPECIFIC_CILKPLUS
