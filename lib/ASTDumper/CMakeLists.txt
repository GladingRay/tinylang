set(LLVM_LINK_COMPONENTS support)

add_tinylang_library(tinylangASTDumper
  ASTDumper.cpp

  LINK_LIBS
  tinylangBasic
  )
