include_directories(..)

set(LLVM_LINK_COMPONENTS
  Support
  )

# In a standard Clang+LLVM build, we need to generate intrinsics before
# building codegen. In a standalone build, LLVM is already built and we don't
# need this dependency. Furthermore, LLVM doesn't export it so we can't have
# this dependency.
set(codegen_deps intrinsics_gen)
if (CLANG_BUILT_STANDALONE)
  set(codegen_deps)
endif()

add_clang_library(clangCodeGenIntel
  CGCilkPlusRuntime.cpp
  CGElementalFunction.cpp
  CGIntelEmit.cpp
  CGIntelStmt.cpp

  DEPENDS
  ${codegen_deps}
  )
