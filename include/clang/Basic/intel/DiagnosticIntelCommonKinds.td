//==--- DiagnosticCommonKinds.td - common diagnostics ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Common Helpers
//===----------------------------------------------------------------------===//

let Component = "Common" in {

let CategoryName = "Parse Issue" in {

def warn_attribute_not_type_attr : Warning<
  "%0 attribute cannot be applied to types">,
  InGroup<IgnoredAttributes>;
def err_pragma_simd_expected_for_loop : Error<
  "expected for statement following '#pragma simd'">;
def err_pragma_simd_invalid_linear_var : Error<
  "invalid linear variable">;
}

}