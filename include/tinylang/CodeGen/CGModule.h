#pragma once

#include "tinylang/AST/AST.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace tinylang {
class CGModule {
  llvm::Module *M;
  ModuleDeclaration *Mod;
  llvm::DenseMap<Decl *, llvm::GlobalObject *> Globals;

public:
  llvm::Type *VoidTy;
  llvm::Type *Int1Ty;
  llvm::Type *Int32Ty;
  llvm::Type *Int64Ty;
  llvm::Type *FloatTy;
  llvm::Constant *Int32Zero;

  /// Counters used to generate unique basic block names for if and while
  /// statements.  Each counter is incremented when the corresponding
  /// statement is emitted, keeping block names unique across the module.
  unsigned IfCounter = 0;
  unsigned WhileCounter = 0;

public:
  CGModule(llvm::Module *M) : M(M), Mod(nullptr) { initialize(); }

  void initialize();

  llvm::LLVMContext &getLLVMCtx() { return M->getContext(); }

  llvm::Module *getModule() const { return M; }

  ModuleDeclaration *getModuleDeclaration() const { return Mod; }

  llvm::Type *convertType(TypeDeclaration *Ty);
  std::string mangleName(Decl *D);

  llvm::GlobalObject *getGlobal(Decl *D);

  /// Writes the current LLVM IR to "tinylang-dump-<N>.ll", where N is a
  /// monotonically increasing counter.  Intended for tracing how the IR
  /// evolves while phi nodes are created and updated.  The call sites are
  /// guarded by the TINYLANG_ENABLE_IR_DUMP macro (see CMake option
  /// TINYLANG_ENABLE_IR_DUMP).
  void dump();

  void run(ModuleDeclaration *Mod);
};
} // namespace tinylang
