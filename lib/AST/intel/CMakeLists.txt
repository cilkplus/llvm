include_directories(..)
set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangASTIntel
  DeclIntel.cpp
  ExprIntel.cpp
  StmtIntel.cpp
  )
