def CilkSpawn : Decl {
  code OpenGuard = [{#if INTEL_SPECIFIC_CILKPLUS}];
  code CloseGuard = [{#endif // INTEL_SPECIFIC_CILKPLUS}];
}
