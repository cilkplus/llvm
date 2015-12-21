//==--- DiagnosticParseKinds.td - libparse diagnostics --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Parser Diagnostics
//===----------------------------------------------------------------------===//

let Component = "Parse" in {

let CategoryName = "Parse Issue" in {

// CQ#364426
// if INTEL_SPECIFIC_CILKPLUS
// Cilk Plus
def err_cilkplus_disable: Error<
  "Cilk Plus support disabled - compile with -fcilkplus">;

def err_cilk_for_missing_control_variable: Error<
  "missing control variable declaration and initialization in '_Cilk_for'">;

def err_cilk_for_missing_condition: Error<
  "missing loop condition expression in '_Cilk_for'">;

def err_cilk_for_missing_increment: Error<
  "missing loop increment expression in '_Cilk_for'">;

def err_cilk_for_missing_semi: Error<
  "expected ';' in '_Cilk_for'">;

def err_cilk_for_expect_grainsize: Error<
  "expected 'grainsize' in '#pragma cilk'">;

def err_cilk_for_expect_assign: Error<
  "expected '=' in '#pragma cilk'">;

def warn_cilk_for_following_grainsize: Warning<
  "'#pragma cilk' ignored, because it is not followed by a '_Cilk_for' loop">,
  InGroup<SourceUsesCilkPlus>;

// Cilk elemental functions
def err_elemental_parameter_pack_unsupported: Error<
  "template parameter pack is not supported in '%0' attribute">;

def err_expected_ident_or_this : Error<
  "expected identifier or this">;

def err_expected_ident : Error<
  "expected identifier">;

def err_expected_comma : Error<"expected ','">;
def err_expected_lparen : Error<"expected '('">;
def err_expected_rparen : Error<"expected ')'">;
def err_expected_colon : Error<"expected ':'">;

// Pragma SIMD
def err_simd_for_missing_initialization: Error<
  "missing initialization in simd for">;

def err_simd_for_missing_condition: Error<
  "missing loop condition expression in simd for">;

def err_simd_for_missing_increment: Error<
  "missing loop increment expression in simd for">;

def err_simd_for_missing_semi: Error<
  "expected ';' in simd for">;

def err_simd_invalid_clause: Error<
  "invalid pragma simd clause">;

def err_simd_expected_reduction_operator: Error<
  "expected reduction operator">;
// endif INTEL_SPECIFIC_CILKPLUS
} // end of Parse Issue category.
} // end of Parser diagnostics
