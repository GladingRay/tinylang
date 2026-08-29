#pragma once

#include "tinylang/AST/AST.h"
#include "tinylang/CodeGen/CGModule.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace llvm {
class Function;
}

namespace tinylang {
class CGProcedure {
  CGModule &CGM;
  llvm::IRBuilder<> Builder;

  llvm::BasicBlock *Curr;

  ProcedureDeclaration *Proc;
  llvm::FunctionType *Fty;
  llvm::Function *Fn;

  struct BasicBlockDef {

    llvm::DenseMap<Decl *, llvm::TrackingVH<llvm::Value>> Defs;

    llvm::DenseMap<llvm::PHINode *, Decl *> IncompletePhis;

    unsigned Sealed : 1;

    BasicBlockDef() : Sealed(0) {}
  };

  llvm::DenseMap<llvm::BasicBlock *, BasicBlockDef> CurrentDef;

  void writeLocalVariable(llvm::BasicBlock *BB, Decl *D, llvm::Value *Val);

  llvm::Value *readLocalVariable(llvm::BasicBlock *BB, Decl *D);

  llvm::Value *readLocalVariableRecursive(llvm::BasicBlock *BB, Decl *D);

  llvm::PHINode *addEmptyPhi(llvm::BasicBlock *BB, Decl *D);

  llvm::Value *addPhiOperands(llvm::BasicBlock *BB, Decl *D,
                              llvm::PHINode *Phi);

  llvm::Value *optimizePhi(llvm::PHINode *Phi);

  void sealBlock(llvm::BasicBlock *BB);

  llvm::DenseMap<FormalParameterDeclaration *, llvm::Argument *> FormalParams;

  void writeVariable(llvm::BasicBlock *BB, Decl *D, llvm::Value *Val);

  llvm::Value *readVariable(llvm::BasicBlock *BB, Decl *D);

  llvm::Type *mapType(Decl *D, bool HonorReference = true);

  llvm::FunctionType *createFunctionType(ProcedureDeclaration *Proc);

  /// Returns the llvm::Function implementing the given procedure,
  /// declaring it on demand if it does not exist yet.
  llvm::Function *resolveFunction(ProcedureDeclaration *Proc);

  llvm::Function *createFunction(ProcedureDeclaration *Proc,
                                 llvm::FunctionType *Fty);

protected:
  void setCurr(llvm::BasicBlock *BB) {
    Curr = BB;
    Builder.SetInsertPoint(Curr);
  }

  llvm::Value *emitInfixExpr(InfixExpression *E);

  llvm::Value *emitPrefixExpr(PrefixExpression *E);

  llvm::Value *emitExpr(Expr *E);

  llvm::Value *emitLValue(IndexedExpression *E);

  void emitStmt(AssignmentStatement *Stmt);

  void emitStmt(ProcedureCallStatement *Stmt);

  void emitStmt(IfStatement *Stmt);

  void emitStmt(WhileStatement *Stmt);

  void emitStmt(ReturnStatement *Stmt);

  void emit(const StmtList &Stmts);

public:
  CGProcedure(CGModule &CGM)
      : CGM(CGM), Builder(CGM.getLLVMCtx()), Curr(nullptr) {}

  /// Creates a declaration (signature without body) for the given
  /// procedure in the current module, so that calls can reference it
  /// before its body is emitted.
  llvm::Function *declareFunction(ProcedureDeclaration *Proc);

  void run(ProcedureDeclaration *Proc);
};
} // namespace tinylang
