include_directories(..)
set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangParseIntel
  ParseSIMD.cpp
  ParseStmt.cpp
  ParseIntelExpr.cpp
  ParseIntelPragma.cpp
  )
