include_directories(..)
set(LLVM_LINK_COMPONENTS
  Support
  )

add_clang_library(clangSemaIntel
  SemaCilk.cpp
  )
