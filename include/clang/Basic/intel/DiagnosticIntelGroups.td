//==--- DiagnosticIntelGroups.td - Diagnostic Group Definitions ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Cilk Plus warnings.
def SourceUsesCilkPlus : DiagGroup<"source-uses-cilk-plus">;
def CilkPlusLoopControlVarModification : DiagGroup<"cilk-loop-control-var-modification">;
def CilkPlusCEAN : DiagGroup<"extended-array-notation">;
def IntelSpecificCilkplus : DiagGroup <"intel-specific-cilkplus", [
    CilkPlusLoopControlVarModification,
    CilkPlusCEAN
  ]>;

// Put here any group if you'd like to add it to "all"
def IntelSpecific : DiagGroup <"intel-specific", [IntelSpecificCilkplus]>;
