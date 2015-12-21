def CilkSyncStmt : Stmt {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
def CilkForGrainsizeStmt : Stmt {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
def CilkForStmt : Stmt {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
def SIMDForStmt : Stmt {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
def CilkSpawnExpr : DStmt<Expr> {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
def CilkRankedStmt : Stmt {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
def CEANIndexExpr : DStmt<Expr> {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
def CEANBuiltinExpr : DStmt<Expr> {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
