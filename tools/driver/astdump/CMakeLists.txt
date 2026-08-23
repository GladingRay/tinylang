set(LLVM_LINK_COMPONENTS support)

add_tinylang_tool(tinylang-astdump
  ASTDump.cpp
  )

target_link_libraries(tinylang-astdump
  PRIVATE
  tinylangASTDumper
  tinylangBasic
  tinylangLexer
  tinylangParser
  tinylangSema
  )
