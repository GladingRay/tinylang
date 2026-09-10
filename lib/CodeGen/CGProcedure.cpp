#include "tinylang/CodeGen/CGProcedure.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ModRef.h"

using namespace tinylang;

void CGProcedure::writeLocalVariable(llvm::BasicBlock *BB, Decl *D,
                                     llvm::Value *Val) {
  assert(BB && "Basic block is nullptr");
  assert((llvm::isa<VariableDeclaration>(D) ||
          llvm::isa<FormalParameterDeclaration>(D)) &&
         "Declaration must be varaible or formal parameter");
  assert(Val && "Value is nullptr");
  CurrentDef[BB].Defs[D] = Val;
}

llvm::Value *CGProcedure::readLocalVariable(llvm::BasicBlock *BB, Decl *Decl) {
  assert(BB && "Basic block is nullptr");
  assert((llvm::isa<VariableDeclaration>(Decl) ||
          llvm::isa<FormalParameterDeclaration>(Decl)) &&
         "Declaration must be variable or formal parameter");
  auto Val = CurrentDef[BB].Defs.find(Decl);
  if (Val != CurrentDef[BB].Defs.end())
    return Val->second;
  return readLocalVariableRecursive(BB, Decl);
}

llvm::Value *
CGProcedure::readLocalVariableRecursive(llvm::BasicBlock *BB, Decl *Decl) {
  llvm::Value *Val = nullptr;
  if (!CurrentDef[BB].Sealed) {
    // Add incomplete phi for variable.
    llvm::PHINode *Phi = addEmptyPhi(BB, Decl);
    CurrentDef[BB].IncompletePhis[Phi] = Decl;
    Val = Phi;
  } else if (auto *PredBB = BB->getSinglePredecessor()) {
    // Only one predecessor.
    Val = readLocalVariable(PredBB, Decl);
  } else {
    // Create empty phi instruction to break potential
    // cycles.
    llvm::PHINode *Phi = addEmptyPhi(BB, Decl);
    writeLocalVariable(BB, Decl, Phi);
    Val = addPhiOperands(BB, Decl, Phi);
  }
  writeLocalVariable(BB, Decl, Val);
  return Val;
}

llvm::PHINode *CGProcedure::addEmptyPhi(llvm::BasicBlock *BB, Decl *Decl) {
  // Aggregates (arrays) are kept in memory; the phi must be of pointer type
  // so that it can be used as a getelementptr base.
  llvm::Type *Ty = mapType(Decl);
  if (Ty->isAggregateType())
    Ty = llvm::PointerType::get(CGM.getLLVMCtx(), /*AddressSpace=*/0);
  llvm::PHINode *Phi = BB->empty()
                           ? llvm::PHINode::Create(Ty, 0, "", BB)
                           : llvm::PHINode::Create(Ty, 0, "",
                                                   BB->getFirstInsertionPt());
#ifdef TINYLANG_ENABLE_IR_DUMP
  // Snapshot the IR right after the (empty) phi node was created.
  CGM.dump();
#endif
  return Phi;
}

llvm::Value *CGProcedure::addPhiOperands(llvm::BasicBlock *BB, Decl *D,
                                         llvm::PHINode *Phi) {
  for (auto *PredBB : predecessors(BB)) {
    Phi->addIncoming(readLocalVariable(PredBB, D), PredBB);
  }
#ifdef TINYLANG_ENABLE_IR_DUMP
  // Snapshot the IR right after the phi node received its incoming values.
  CGM.dump();
#endif
  return optimizePhi(Phi);
}

llvm::Value *CGProcedure::optimizePhi(llvm::PHINode *Phi) {
  llvm::Value *Same = nullptr;
  for (llvm::Value *V : Phi->incoming_values()) {
    if (V == Same || V == Phi)
      continue;
    if (Same && V != Same)
      return Phi;
    Same = V;
  }
  if (Same == nullptr)
    Same = llvm::UndefValue::get(Phi->getType());
  // Collect phi instructions using this one.
  llvm::SmallVector<llvm::PHINode *, 8> CandidatePhis;
  for (llvm::Use &U : Phi->uses()) {
    if (auto *P = llvm::dyn_cast<llvm::PHINode>(U.getUser()))
      if (P != Phi)
        CandidatePhis.push_back(P);
  }
  Phi->replaceAllUsesWith(Same);
  Phi->eraseFromParent();
  for (auto *P : CandidatePhis)
    optimizePhi(P);
  return Same;
}

void CGProcedure::sealBlock(llvm::BasicBlock *BB) {
  assert(!CurrentDef[BB].Sealed && "Attempt to seal already sealed block");
  for (auto PhiDecl : CurrentDef[BB].IncompletePhis) {
    addPhiOperands(BB, PhiDecl.second, PhiDecl.first);
  }
  CurrentDef[BB].IncompletePhis.clear();
  CurrentDef[BB].Sealed = true;
}

void CGProcedure::writeVariable(llvm::BasicBlock *BB, Decl *D,
                                llvm::Value *Val) {
  if (auto *V = llvm::dyn_cast<VariableDeclaration>(D)) {
    if (V->getEnclosingDecl() == Proc)
      writeLocalVariable(BB, D, Val);
    else if (V->getEnclosingDecl() == CGM.getModuleDeclaration()) {
      Builder.CreateStore(Val, CGM.getGlobal(D));
    } else
      llvm::report_fatal_error("Nested procedures not yet supported");
  } else if (auto *FP = llvm::dyn_cast<FormalParameterDeclaration>(D)) {
    if (FP->isVar()) {
      Builder.CreateStore(Val, FormalParams[FP]);
    } else
      writeLocalVariable(BB, D, Val);
  } else
    llvm::report_fatal_error("Unsupported declaration");
}

llvm::Value *CGProcedure::readVariable(llvm::BasicBlock *BB, Decl *D) {
  if (auto *V = llvm::dyn_cast<VariableDeclaration>(D)) {
    if (V->getEnclosingDecl() == Proc)
      return readLocalVariable(BB, D);
    else if (V->getEnclosingDecl() == CGM.getModuleDeclaration()) {
      llvm::Type *Ty = mapType(D);
      if (Ty->isAggregateType())
        return CGM.getGlobal(D); // arrays: the address of the global
      return Builder.CreateLoad(Ty, CGM.getGlobal(D));
    } else
      llvm::report_fatal_error("Nested procedures not yet supported");
  } else if (auto *FP = llvm::dyn_cast<FormalParameterDeclaration>(D)) {
    if (FP->isVar()) {
      llvm::Type *Ty = mapType(FP, false);
      if (Ty->isAggregateType())
        return FormalParams[FP]; // arrays: the address of the argument
      return Builder.CreateLoad(Ty, FormalParams[FP]);
    } else
      return readLocalVariable(BB, D);
  } else
    llvm::report_fatal_error("Unsupported declaration");
}

llvm::Type *CGProcedure::mapType(Decl *Decl, bool HonorReference) {
  if (auto *FP = llvm::dyn_cast<FormalParameterDeclaration>(Decl)) {
    if (FP->isVar() && HonorReference)
      return llvm::PointerType::get(CGM.getLLVMCtx(),
                                    /*AddressSpace=*/0);
    return CGM.convertType(FP->getType());
  }
  if (auto *V = llvm::dyn_cast<VariableDeclaration>(Decl))
    return CGM.convertType(V->getType());
  return CGM.convertType(llvm::cast<TypeDeclaration>(Decl));
}

llvm::FunctionType *
CGProcedure::createFunctionType(ProcedureDeclaration *Proc) {
  llvm::Type *ResultTy = CGM.VoidTy;
  if (Proc->getRetType()) {
    ResultTy = mapType(Proc->getRetType());
  }
  const auto &FormalParams = Proc->getFormalParams();
  llvm::SmallVector<llvm::Type *, 8> ParamTypes;
  for (const auto &FP : FormalParams) {
    llvm::Type *Ty = mapType(FP.get());
    ParamTypes.push_back(Ty);
  }
  return llvm::FunctionType::get(ResultTy, ParamTypes,
                                 /*IsVarArgs=*/false);
}

llvm::Function *CGProcedure::declareFunction(ProcedureDeclaration *Proc) {
  llvm::FunctionType *FTy = createFunctionType(Proc);
  return llvm::Function::Create(FTy, llvm::GlobalValue::ExternalLinkage,
                                CGM.mangleName(Proc), CGM.getModule());
}

llvm::Function *CGProcedure::resolveFunction(ProcedureDeclaration *Proc) {
  if (llvm::Function *Fn =
          CGM.getModule()->getFunction(CGM.mangleName(Proc)))
    return Fn;
  return declareFunction(Proc);
}

llvm::Function *CGProcedure::createFunction(ProcedureDeclaration *Proc,
                                            llvm::FunctionType *FTy) {
  // Reuse the declaration created by declareFunction() (if any) instead of
  // inserting a second function with the same name.
  llvm::Function *Fn = CGM.getModule()->getFunction(CGM.mangleName(Proc));
  if (!Fn)
    Fn = llvm::Function::Create(Fty, llvm::GlobalValue::ExternalLinkage,
                                CGM.mangleName(Proc), CGM.getModule());
  // Give parameters a name.
  for (auto Pair : llvm::enumerate(Fn->args())) {
    llvm::Argument &Arg = Pair.value();
    FormalParameterDeclaration *FP =
        Proc->getFormalParams()[Pair.index()].get();
    if (FP->isVar()) {
      llvm::AttrBuilder Attr(CGM.getLLVMCtx());
      llvm::TypeSize Sz = CGM.getModule()->getDataLayout().getTypeStoreSize(
          CGM.convertType(FP->getType()));
      Attr.addDereferenceableAttr(Sz);
      Attr.addCapturesAttr(llvm::CaptureInfo::none());
      Arg.addAttrs(Attr);
    }
    Arg.setName(FP->getName());
  }
  return Fn;
}

llvm::Value *
CGProcedure::emitInfixExpr(InfixExpression *E) {
  llvm::Value *Left = emitExpr(E->getLeft());
  llvm::Value *Right = emitExpr(E->getRight());
  llvm::Value *Result = nullptr;
  // Modula-2 keeps INTEGER and REAL in separate type families, so the
  // operand types of a valid expression are the same; REAL is a 32-bit float.
  bool IsReal = getUnderlyingType(E->getLeft()->getType())->getName() ==
                "REAL";
  switch (E->getOperatorInfo().getKind()) {
  case tok::plus:
    Result = IsReal ? Builder.CreateFAdd(Left, Right)
                    : Builder.CreateNSWAdd(Left, Right);
    break;
  case tok::minus:
    Result = IsReal ? Builder.CreateFSub(Left, Right)
                    : Builder.CreateNSWSub(Left, Right);
    break;
  case tok::star:
    Result = IsReal ? Builder.CreateFMul(Left, Right)
                    : Builder.CreateNSWMul(Left, Right);
    break;
  case tok::kw_DIV:
    Result = Builder.CreateSDiv(Left, Right);
    break;
  case tok::kw_MOD:
    Result = Builder.CreateSRem(Left, Right);
    break;
  case tok::slash:
    Result = Builder.CreateFDiv(Left, Right);
    break;
  case tok::equal:
    Result = IsReal ? Builder.CreateFCmpOEQ(Left, Right)
                    : Builder.CreateICmpEQ(Left, Right);
    break;
  case tok::hash:
    Result = IsReal ? Builder.CreateFCmpONE(Left, Right)
                    : Builder.CreateICmpNE(Left, Right);
    break;
  case tok::less:
    Result = IsReal ? Builder.CreateFCmpOLT(Left, Right)
                    : Builder.CreateICmpSLT(Left, Right);
    break;
  case tok::lessequal:
    Result = IsReal ? Builder.CreateFCmpOLE(Left, Right)
                    : Builder.CreateICmpSLE(Left, Right);
    break;
  case tok::greater:
    Result = IsReal ? Builder.CreateFCmpOGT(Left, Right)
                    : Builder.CreateICmpSGT(Left, Right);
    break;
  case tok::greaterequal:
    Result = IsReal ? Builder.CreateFCmpOGE(Left, Right)
                    : Builder.CreateICmpSGE(Left, Right);
    break;
  case tok::kw_AND:
    Result = Builder.CreateAnd(Left, Right);
    break;
  case tok::kw_OR:
    Result = Builder.CreateOr(Left, Right);
    break;
  default:
    llvm_unreachable("Wrong operator");
  }
  return Result;
}

llvm::Value *
CGProcedure::emitPrefixExpr(PrefixExpression *E) {
  llvm::Value *Result = emitExpr(E->getExpr());
  bool IsReal = getUnderlyingType(E->getExpr()->getType())->getName() ==
                "REAL";
  switch (E->getOperatorInfo().getKind()) {
  case tok::plus:
    // Identity - nothing to do.
    break;
  case tok::minus:
    Result = IsReal ? Builder.CreateFNeg(Result)
                    : Builder.CreateNeg(Result);
    break;
  case tok::kw_NOT:
    Result = Builder.CreateNot(Result);
    break;
  default:
    llvm_unreachable("Wrong operator");
  }
  return Result;
}

llvm::Value *CGProcedure::emitExpr(Expr *E) {
  if (auto *Infix = llvm::dyn_cast<InfixExpression>(E)) {
    return emitInfixExpr(Infix);
  } else if (auto *Prefix =
                 llvm::dyn_cast<PrefixExpression>(E)) {
    return emitPrefixExpr(Prefix);
  } else if (auto *Var =
                 llvm::dyn_cast<VariableAccess>(E)) {
    auto *Decl = Var->getDecl();
    // With more languages features in place, here you need
    // to add array and record support.
    return readVariable(Curr, Decl);
  } else if (auto *Const =
                 llvm::dyn_cast<ConstantAccess>(E)) {
    return emitExpr(Const->getDecl()->getExpr());
  } else if (auto *IntLit =
                 llvm::dyn_cast<IntegerLiteral>(E)) {
    return llvm::ConstantInt::get(CGM.Int64Ty,
                                  IntLit->getValue());
  } else if (auto *RealLit = llvm::dyn_cast<RealLiteral>(E)) {
    return llvm::ConstantFP::get(CGM.FloatTy, RealLit->getValue());
  } else if (auto *BoolLit =
                 llvm::dyn_cast<BooleanLiteral>(E)) {
    return llvm::ConstantInt::get(CGM.Int1Ty,
                                  BoolLit->getValue());
  } else if (auto *FuncCall = llvm::dyn_cast<FunctionCallExpr>(E)) {
    ProcedureDeclaration *Proc = FuncCall->getDecl();
    llvm::SmallVector<llvm::Value *, 8> Args;
    const ExprList &Actuals = FuncCall->getParams();
    for (size_t I = 0; I < Actuals.size(); ++I) {
      llvm::Value *V = emitExpr(Actuals[I].get());
      FormalParameterDeclaration *FP = Proc->getFormalParams()[I].get();
      if (!FP->isVar()) {
        llvm::Type *Ty = mapType(FP);
        if (Ty->isAggregateType())
          V = Builder.CreateLoad(Ty, V);
      }
      Args.push_back(V);
    }
    llvm::Function *Callee = resolveFunction(Proc);
    llvm::Value *Call = Builder.CreateCall(Callee->getFunctionType(), Callee,
                                           Args);
    if (Proc->getRetType()) {
      llvm::Type *RetTy = CGM.convertType(Proc->getRetType());
      if (RetTy->isAggregateType()) {
        // Aggregates are represented by address everywhere else, so spill the
        // by-value call result into a temporary and return its address.
        llvm::Value *Addr = Builder.CreateAlloca(RetTy);
        Builder.CreateStore(Call, Addr);
        return Addr;
      }
    }
    return Call;
  } else if (auto *MethodCall = llvm::dyn_cast<MethodCallExpr>(E)) {
    return emitMethodCall(MethodCall);
  } else if (auto *TypeTest = llvm::dyn_cast<TypeTestExpr>(E)) {
    return emitTypeTest(TypeTest);
  } else if (auto *Idx = llvm::dyn_cast<IndexedExpression>(E)) {
    auto *ArrTy = llvm::cast<ArrayTypeDeclaration>(
        getUnderlyingType(Idx->getBase()->getType()));
    llvm::Value *Addr = emitLValue(Idx);
    if (llvm::isa<ArrayTypeDeclaration>(
            getUnderlyingType(ArrTy->getElementType())))
      return Addr; // address of the sub-array (multi-dimensional arrays)
    return Builder.CreateLoad(CGM.convertType(ArrTy->getElementType()), Addr);
  } else if (auto *Field = llvm::dyn_cast<FieldAccess>(E)) {
    llvm::Value *Addr = emitLValue(Field);
    llvm::Type *FieldTy = CGM.convertType(Field->getField()->getType());
    if (FieldTy->isAggregateType())
      return Addr; // address of the record/array field
    return Builder.CreateLoad(FieldTy, Addr);
  }
  llvm::report_fatal_error("Unsupported expression");
}

llvm::Value *CGProcedure::emitLValue(Expr *E) {
  if (auto *Var = llvm::dyn_cast<VariableAccess>(E)) {
    Decl *D = Var->getDecl();
    if (auto *V = llvm::dyn_cast<VariableDeclaration>(D)) {
      if (V->getEnclosingDecl() == Proc)
        return readLocalVariable(Curr, D);
      if (V->getEnclosingDecl() == CGM.getModuleDeclaration())
        return CGM.getGlobal(D);
      llvm::report_fatal_error("Nested procedures not yet supported");
    }
    if (auto *FP = llvm::dyn_cast<FormalParameterDeclaration>(D)) {
      if (FP->isVar())
        return FormalParams[FP];
      llvm::Type *Ty = mapType(FP);
      if (Ty->isAggregateType())
        return readLocalVariable(Curr, D);
      llvm::report_fatal_error("Cannot take address of scalar value parameter");
    }
  } else if (auto *Idx = llvm::dyn_cast<IndexedExpression>(E)) {
    return emitLValue(Idx);
  } else if (auto *Field = llvm::dyn_cast<FieldAccess>(E)) {
    return emitLValue(Field);
  }
  llvm::report_fatal_error("Unsupported lvalue");
}

llvm::Value *CGProcedure::emitLValue(IndexedExpression *E) {
  auto *ArrTy = llvm::cast<ArrayTypeDeclaration>(
      getUnderlyingType(E->getBase()->getType()));
  llvm::Value *Base = emitExpr(E->getBase());
  llvm::Value *Index = emitExpr(E->getIndex());
  // Normalize the (inclusive) Modula-2 subrange to a zero-based LLVM index.
  Index = Builder.CreateSub(Index, Builder.getInt64(ArrTy->getLowBound()));
  llvm::Type *ArrLLVMTy = CGM.convertType(ArrTy);
  return Builder.CreateGEP(ArrLLVMTy, Base, {Builder.getInt64(0), Index});
}

llvm::Value *CGProcedure::emitLValue(FieldAccess *E) {
  llvm::Value *Base = emitExpr(E->getBase());
  auto *RecTy = llvm::cast<RecordTypeDeclaration>(
      getUnderlyingType(E->getBase()->getType()));
  unsigned Index = RecTy->getFieldIndex(E->getField());
  llvm::Type *RecLLVMTy = CGM.convertType(RecTy);
  return Builder.CreateGEP(RecLLVMTy, Base,
                           {Builder.getInt32(0), Builder.getInt32(Index)});
}

llvm::Value *CGProcedure::emitMethodCall(MethodCallExpr *E) {
  llvm::Value *Self = emitLValue(E->getReceiver());
  auto *RecTy = llvm::cast<RecordTypeDeclaration>(
      getUnderlyingType(E->getReceiver()->getType()));
  ProcedureDeclaration *Method = E->getDecl();

  // Load the implementation from the receiver's type descriptor, so an
  // overridden method of the dynamic type is called.
  llvm::GlobalVariable *Desc = CGM.getTypeDescriptor(RecTy);
  unsigned Slot = getMethodSlot(RecTy, Method->getName());
  llvm::Type *DescTy = Desc->getValueType();
  llvm::Value *DynamicDesc = Builder.CreateLoad(
      llvm::PointerType::get(CGM.getLLVMCtx(), 0), Self);
  llvm::Value *SlotPtr = Builder.CreateGEP(
      DescTy, DynamicDesc, {Builder.getInt32(0), Builder.getInt32(Slot)});
  llvm::Value *Impl = Builder.CreateLoad(
      llvm::PointerType::get(CGM.getLLVMCtx(), 0), SlotPtr);

  // The receiver is always passed by reference, then the actual parameters.
  llvm::SmallVector<llvm::Value *, 8> Args{Self};
  const ExprList &Actuals = E->getParams();
  const FormalParamList &Formals = Method->getFormalParams();
  for (size_t I = 0; I < Actuals.size(); ++I) {
    llvm::Value *V = emitExpr(Actuals[I].get());
    FormalParameterDeclaration *FP = Formals[I + 1].get();
    if (!FP->isVar()) {
      llvm::Type *Ty = mapType(FP);
      if (Ty->isAggregateType())
        V = Builder.CreateLoad(Ty, V);
    }
    Args.push_back(V);
  }
  llvm::Function *Static = CGM.getModule()->getFunction(
      CGM.mangleName(Method));
  llvm::Value *Call = Builder.CreateCall(Static->getFunctionType(), Impl,
                                         Args);
  if (Method->getRetType()) {
    llvm::Type *RetTy = CGM.convertType(Method->getRetType());
    if (RetTy->isAggregateType()) {
      llvm::Value *Addr = Builder.CreateAlloca(RetTy);
      Builder.CreateStore(Call, Addr);
      return Addr;
    }
  }
  return Call;
}

llvm::Value *CGProcedure::emitTypeTest(TypeTestExpr *E) {
  llvm::Value *Addr = emitLValue(E->getExpr());
  llvm::Value *Tag =
      Builder.CreateLoad(llvm::PointerType::get(CGM.getLLVMCtx(), 0), Addr);
  auto *TestedTy = llvm::cast<RecordTypeDeclaration>(
      getUnderlyingType(E->getTestedType()));
  llvm::GlobalVariable *Desc = CGM.getTypeDescriptor(TestedTy);
  return Builder.CreateICmpEQ(Tag, Desc);
}

void CGProcedure::emitMemCpy(llvm::Value *Dst, llvm::Value *Src,
                             llvm::Type *Ty) {
  const llvm::DataLayout &DL = CGM.getModule()->getDataLayout();
  llvm::Align A = DL.getABITypeAlign(Ty);
  Builder.CreateMemCpy(Dst, A, Src, A, DL.getTypeStoreSize(Ty));
}

void CGProcedure::emitStmt(AssignmentStatement *Stmt) {
  if (auto *Var = llvm::dyn_cast<VariableAccess>(Stmt->getTarget())) {
    llvm::Type *Ty = CGM.convertType(Var->getType());
    if (Ty->isAggregateType()) {
      emitMemCpy(emitLValue(Var), emitExpr(Stmt->getExpr()), Ty);
      return;
    }
    writeVariable(Curr, Var->getDecl(), emitExpr(Stmt->getExpr()));
  } else if (auto *Idx = llvm::dyn_cast<IndexedExpression>(Stmt->getTarget())) {
    llvm::Type *Ty = CGM.convertType(Idx->getType());
    if (Ty->isAggregateType()) {
      emitMemCpy(emitLValue(Idx), emitExpr(Stmt->getExpr()), Ty);
      return;
    }
    Builder.CreateStore(emitExpr(Stmt->getExpr()), emitLValue(Idx));
  } else if (auto *Field =
                 llvm::dyn_cast<FieldAccess>(Stmt->getTarget())) {
    llvm::Type *Ty = CGM.convertType(Field->getType());
    if (Ty->isAggregateType()) {
      emitMemCpy(emitLValue(Field), emitExpr(Stmt->getExpr()), Ty);
      return;
    }
    Builder.CreateStore(emitExpr(Stmt->getExpr()), emitLValue(Field));
  } else {
    llvm::report_fatal_error("Unsupported assignment target");
  }
}

void CGProcedure::emitStmt(ProcedureCallStatement *Stmt) {
  ProcedureDeclaration *Proc = Stmt->getProc();
  llvm::SmallVector<llvm::Value *, 8> Args;
  const ExprList &Actuals = Stmt->getParams();
  for (size_t I = 0; I < Actuals.size(); ++I) {
    llvm::Value *V = emitExpr(Actuals[I].get());
    FormalParameterDeclaration *FP = Proc->getFormalParams()[I].get();
    if (!FP->isVar()) {
      llvm::Type *Ty = mapType(FP);
      if (Ty->isAggregateType())
        V = Builder.CreateLoad(Ty, V);
    }
    Args.push_back(V);
  }
  llvm::Function *Callee = resolveFunction(Proc);
  Builder.CreateCall(Callee->getFunctionType(), Callee, Args);
}

void CGProcedure::emitStmt(MethodCallStatement *Stmt) {
  // The result of the method (if any) is discarded.
  emitMethodCall(llvm::cast<MethodCallExpr>(Stmt->getCall()));
}

void CGProcedure::emitStmt(IfStatement *Stmt) {
  bool HasElse = Stmt->getElseStmts().size() > 0;
  unsigned IfNo = CGM.IfCounter++;

  // Create the required basic blocks.
  llvm::BasicBlock *IfBB = llvm::BasicBlock::Create(
      CGM.getLLVMCtx(), llvm::Twine("if.body.") + llvm::Twine(IfNo), Fn);
  llvm::BasicBlock *ElseBB =
      HasElse ? llvm::BasicBlock::Create(CGM.getLLVMCtx(),
                                         llvm::Twine("else.body.") +
                                             llvm::Twine(IfNo),
                                         Fn)
              : nullptr;
  llvm::BasicBlock *AfterIfBB = llvm::BasicBlock::Create(
      CGM.getLLVMCtx(), llvm::Twine("after.if.") + llvm::Twine(IfNo), Fn);

  llvm::Value *Cond = emitExpr(Stmt->getCond());
  Builder.CreateCondBr(Cond, IfBB,
                       HasElse ? ElseBB : AfterIfBB);
  sealBlock(Curr);

  setCurr(IfBB);
  emit(Stmt->getIfStmts());
  if (!Curr->getTerminatorOrNull()) {
    Builder.CreateBr(AfterIfBB);
  }
  sealBlock(Curr);

  if (HasElse) {
    setCurr(ElseBB);
    emit(Stmt->getElseStmts());
    if (!Curr->getTerminatorOrNull()) {
      Builder.CreateBr(AfterIfBB);
    }
    sealBlock(Curr);
  }
  setCurr(AfterIfBB);
}

void CGProcedure::emitStmt(WhileStatement *Stmt) {
  unsigned WhileNo = CGM.WhileCounter++;

  // The basic block for the condition.
  llvm::BasicBlock *WhileCondBB = llvm::BasicBlock::Create(
      CGM.getLLVMCtx(), llvm::Twine("while.cond.") + llvm::Twine(WhileNo),
      Fn);
  // The basic block for the while body.
  llvm::BasicBlock *WhileBodyBB = llvm::BasicBlock::Create(
      CGM.getLLVMCtx(), llvm::Twine("while.body.") + llvm::Twine(WhileNo),
      Fn);
  // The basic block after the while statement.
  llvm::BasicBlock *AfterWhileBB = llvm::BasicBlock::Create(
      CGM.getLLVMCtx(), llvm::Twine("after.while.") + llvm::Twine(WhileNo),
      Fn);

  Builder.CreateBr(WhileCondBB);
  sealBlock(Curr);
  setCurr(WhileCondBB);
  llvm::Value *Cond = emitExpr(Stmt->getCond());
  Builder.CreateCondBr(Cond, WhileBodyBB, AfterWhileBB);

  setCurr(WhileBodyBB);
  emit(Stmt->getWhileStmts());
  Builder.CreateBr(WhileCondBB);
  sealBlock(WhileCondBB);
  sealBlock(Curr);

  setCurr(AfterWhileBB);
}

void CGProcedure::emitStmt(ReturnStatement *Stmt) {
  if (Stmt->getRetVal()) {
    llvm::Value *RetVal = emitExpr(Stmt->getRetVal());
    if (Proc->getRetType()) {
      llvm::Type *RetTy = CGM.convertType(Proc->getRetType());
      if (RetTy->isAggregateType())
        RetVal = Builder.CreateLoad(RetTy, RetVal);
    }
    Builder.CreateRet(RetVal);
  } else {
    Builder.CreateRetVoid();
  }
}

void CGProcedure::emit(const StmtList &Stmts) {
  for (const auto &S : Stmts) {
    if (auto *Stmt = llvm::dyn_cast<AssignmentStatement>(S.get()))
      emitStmt(Stmt);
    else if (auto *Stmt = llvm::dyn_cast<ProcedureCallStatement>(S.get()))
      emitStmt(Stmt);
    else if (auto *Stmt = llvm::dyn_cast<MethodCallStatement>(S.get()))
      emitStmt(Stmt);
    else if (auto *Stmt = llvm::dyn_cast<IfStatement>(S.get()))
      emitStmt(Stmt);
    else if (auto *Stmt = llvm::dyn_cast<WhileStatement>(S.get()))
      emitStmt(Stmt);
    else if (auto *Stmt = llvm::dyn_cast<ReturnStatement>(S.get()))
      emitStmt(Stmt);
    else
      llvm_unreachable("Unknown statement");
  }
}

void CGProcedure::run(ProcedureDeclaration *Proc) {
  this->Proc = Proc;
  Fty = createFunctionType(Proc);
  Fn = createFunction(Proc, Fty);

  llvm::BasicBlock *BB = llvm::BasicBlock::Create(
      CGM.getLLVMCtx(), "entry", Fn);
  setCurr(BB);

  for (auto Pair : llvm::enumerate(Fn->args())) {
    llvm::Argument *Arg = &Pair.value();
    FormalParameterDeclaration *FP =
        Proc->getFormalParams()[Pair.index()].get();
    llvm::Type *Ty = mapType(FP);
    if (FP->isVar()) {
      // Create mapping FormalParameter -> llvm::Argument for
      // VAR parameters.
      FormalParams[FP] = Arg;
      writeLocalVariable(Curr, FP, Arg);
    } else if (Ty->isAggregateType()) {
      // Aggregate value parameters are copied into an alloca so that field
      // access can use the same pointer-based path as local records/arrays.
      llvm::Value *Addr = Builder.CreateAlloca(Ty);
      Builder.CreateStore(Arg, Addr);
      writeLocalVariable(Curr, FP, Addr);
    } else {
      writeLocalVariable(Curr, FP, Arg);
    }
  }

  for (const auto &D : Proc->getDecls()) {
    if (auto *Var =
            llvm::dyn_cast<VariableDeclaration>(D.get())) {
      llvm::Type *Ty = mapType(Var);
      if (Ty->isAggregateType()) {
        llvm::Value *Val = Builder.CreateAlloca(Ty);
        // Initialise the aggregate, including the type descriptor stored in
        // an extended record.
        Builder.CreateStore(CGM.getZeroValue(Var->getType()), Val);
        writeLocalVariable(Curr, Var, Val);
      }
    }
  }

  emit(Proc->getStmts());
  if (!Curr->getTerminatorOrNull()) {
    Builder.CreateRetVoid();
  }
  sealBlock(Curr);
}
