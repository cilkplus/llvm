//===--- ParsePragma.h - simd/cilk pragmas parsing --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes handlers of cilk and simd pragmas
//
//===----------------------------------------------------------------------===//
#if INTEL_SPECIFIC_CILKPLUS
class PragmaSIMDHandler : public PragmaHandler {
  public:
    explicit PragmaSIMDHandler() : PragmaHandler("simd") {}

    virtual void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer,
                                Token &FirstToken);
};

class PragmaCilkGrainsizeHandler : public PragmaHandler {
  public:
    PragmaCilkGrainsizeHandler() : PragmaHandler("cilk") {}
    virtual void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer,
                              Token &FirstToken);
};
#endif // INTEL_SPECIFIC_CILKPLUS
