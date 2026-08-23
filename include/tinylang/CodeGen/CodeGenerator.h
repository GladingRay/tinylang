#pragma once

#include "tinylang/AST/AST.h"
#include "llvm/Target/TargetMachine.h"
#include <string>

namespace tinylang {
class CodeGenerator {
  llvm::LLVMContext &Ctx;
  llvm::TargetMachine *TM;

protected:
  CodeGenerator(llvm::LLVMContext &Ctx, llvm::TargetMachine *TM)
      : Ctx(Ctx), TM(TM) {}

public:
  static CodeGenerator *create(llvm::LLVMContext &Ctx, llvm::TargetMachine *TM);

  std::unique_ptr<llvm::Module> run(ModuleDeclaration *Mod,
                                    const std::string &FileName);
};
} // namespace tinylang