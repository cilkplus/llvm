//==--- DiagnosticDriverKinds.td - libdriver diagnostics ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

let Component = "Driver" in {
def err_drv_cilk_objc : Error<
  "Cilk Plus does not support Objective-C">;
def err_drv_cilk_unsupported: Error<
  "Cilk Plus not yet supported for this target">;
}
