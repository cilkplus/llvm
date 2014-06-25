//===---------------- Intel Pragma Intrinsic ----------------===//
//
def int_intel_pragma : Intrinsic<[],
                                   [llvm_metadata_ty],
                                   [], "llvm.intel.pragma">;
