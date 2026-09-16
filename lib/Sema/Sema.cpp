#include "tinylang/Sema/Sema.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

using namespace tinylang;

static StringRef getOperatorSpelling(tok::TokenKind Kind) {
  if (const char *P = tok::getPunctuatorSpelling(Kind))
    return P;
  return tok::getKeywordSpelling(Kind);
}

/// A designator is an expression that denotes a variable: a variable, a
/// record field or an array element (possibly nested).
static bool isDesignator(Expr *E) {
  return isa<VariableAccess>(E) || isa<FieldAccess>(E) ||
         isa<IndexedExpression>(E);
}

static bool evalConstInt(Expr *E, int64_t &Value) {
  if (auto *Lit = dyn_cast_or_null<IntegerLiteral>(E)) {
    Value = Lit->getValue().getSExtValue();
    return true;
  }
  if (auto *Pre = dyn_cast_or_null<PrefixExpression>(E)) {
    if (Pre->getOperatorInfo().getKind() == tok::minus) {
      int64_t Operand;
      if (evalConstInt(Pre->getExpr(), Operand)) {
        Value = -Operand;
        return true;
      }
    }
  }
  return false;
}

void Sema::enterScope(Decl *D) {
  CurrentScope = new Scope(CurrentScope);
  CurrentDecl = D;
}

void Sema::leaveScope() {
  assert(CurrentScope && "Can't leave non-existing scope");
  Scope *Parent = CurrentScope->getParent();
  delete CurrentScope;
  CurrentScope = Parent;
  CurrentDecl = CurrentDecl->getEnclosingDecl();
}

bool Sema::isOperatorForType(tok::TokenKind Op, TypeDeclaration *Ty) {
  Ty = getUnderlyingType(Ty);
  switch (Op) {
  case tok::plus:
  case tok::minus:
  case tok::star:
    return Ty == IntegerType.get() || Ty == RealType.get();
  case tok::kw_DIV:
  case tok::kw_MOD:
    return Ty == IntegerType.get();
  case tok::slash:
    return Ty == RealType.get();
  case tok::kw_AND:
  case tok::kw_OR:
  case tok::kw_NOT:
    return Ty == BooleanType.get();
  default:
    llvm_unreachable("Unknown operator");
  }
}

bool Sema::isSameType(TypeDeclaration *LHS, TypeDeclaration *RHS) {
  if (!LHS || !RHS)
    return LHS == RHS;
  LHS = getUnderlyingType(LHS);
  RHS = getUnderlyingType(RHS);
  if (LHS == RHS)
    return true;
  if (auto *LArr = dyn_cast<ArrayTypeDeclaration>(LHS))
    if (auto *RArr = dyn_cast<ArrayTypeDeclaration>(RHS))
      return LArr->getLowBound() == RArr->getLowBound() &&
             LArr->getHighBound() == RArr->getHighBound() &&
             isSameType(LArr->getElementType(), RArr->getElementType());
  return false;
}

bool Sema::isCompatibleWithFormal(TypeDeclaration *Actual,
                                  TypeDeclaration *Formal, bool IsVar) {
  if (isSameType(Actual, Formal))
    return true;
  if (!Actual || !Formal)
    return false;
  // Oberon-2 style polymorphism: a VAR parameter of a base record type
  // accepts variables of any type extending it.
  if (!IsVar)
    return false;
  auto *ActualRec =
      dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Actual));
  auto *FormalRec =
      dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Formal));
  if (!ActualRec || !FormalRec)
    return false;
  return isExtensionOf(ActualRec, FormalRec);
}

bool Sema::hasSameSignature(ProcedureDeclaration *LHS,
                            ProcedureDeclaration *RHS) {
  const FormalParamList &L = LHS->getFormalParams();
  const FormalParamList &R = RHS->getFormalParams();
  size_t LStart = (!L.empty() && L.front()->isReceiver()) ? 1 : 0;
  size_t RStart = (!R.empty() && R.front()->isReceiver()) ? 1 : 0;
  if (L.size() - LStart != R.size() - RStart)
    return false;
  if (!isSameType(LHS->getRetType(), RHS->getRetType()))
    return false;
  for (size_t I = 0; I + LStart < L.size(); ++I)
    if (!isSameType(L[I + LStart]->getType(), R[I + RStart]->getType()))
      return false;
  return true;
}

void Sema::checkFormalAndActualParameters(SMLoc Loc,
                                          const FormalParamList &Formals,
                                          const ExprList &Actuals) {
  // The implicit receiver of a type-bound procedure is not an actual
  // parameter supplied by the caller.
  unsigned FirstFormal =
      (!Formals.empty() && Formals.front()->isReceiver()) ? 1 : 0;
  if (Formals.size() != Actuals.size() + FirstFormal) {
    Diags.report(Loc, diag::err_wrong_number_of_parameters);
    return;
  }
  auto A = Actuals.begin();
  for (auto I = Formals.begin() + FirstFormal, E = Formals.end(); I != E;
       ++I, ++A) {
    FormalParameterDeclaration *F = I->get();
    Expr *Arg = A->get();
    if (!Arg)
      continue;
    if (!isCompatibleWithFormal(Arg->getType(), F->getType(), F->isVar()))
      Diags.report(
          Loc, diag::err_type_of_formal_and_actual_parameter_not_compatible);
    bool IsLValue = isa<VariableAccess>(Arg) || isa<IndexedExpression>(Arg) ||
                    isa<FieldAccess>(Arg);
    if (F->isVar() && !IsLValue)
      Diags.report(Loc, diag::err_var_parameter_requires_var);
  }
}

void Sema::initialize() {
  // Setup global scope.
  CurrentScope = new Scope();
  CurrentDecl = nullptr;
  IntegerType =
      std::make_unique<TypeDeclaration>(CurrentDecl, SMLoc(), "INTEGER");
  RealType = std::make_unique<TypeDeclaration>(CurrentDecl, SMLoc(), "REAL");
  BooleanType =
      std::make_unique<TypeDeclaration>(CurrentDecl, SMLoc(), "BOOLEAN");
  TrueConst = std::make_unique<ConstantDeclaration>(
      CurrentDecl, SMLoc(), "TRUE",
      std::make_unique<BooleanLiteral>(true, BooleanType.get()));
  FalseConst = std::make_unique<ConstantDeclaration>(
      CurrentDecl, SMLoc(), "FALSE",
      std::make_unique<BooleanLiteral>(false, BooleanType.get()));
  CurrentScope->insert(IntegerType.get());
  CurrentScope->insert(RealType.get());
  CurrentScope->insert(BooleanType.get());
  CurrentScope->insert(TrueConst.get());
  CurrentScope->insert(FalseConst.get());
}

std::unique_ptr<ModuleDeclaration>
Sema::actOnModuleDeclaration(SMLoc Loc, StringRef Name) {
  return std::make_unique<ModuleDeclaration>(CurrentDecl, Loc, Name);
}

void Sema::actOnModuleDeclaration(ModuleDeclaration *ModDecl, SMLoc Loc,
                                  StringRef Name, DeclList &Decls,
                                  StmtList &Stmts) {
  if (Name != ModDecl->getName()) {
    Diags.report(Loc, diag::err_module_identifier_not_equal);
    Diags.report(ModDecl->getLocation(),
                 diag::note_module_identifier_declaration);
  }
  ModDecl->setDecls(std::move(Decls));
  ModDecl->setStmts(std::move(Stmts));

  // Every method declared inside a record body needs an implementation.
  for (auto *Method : DeclaredMethods)
    if (!Method->getDefinition())
      Diags.report(Method->getLocation(), diag::err_method_not_implemented,
                   Method->getName());
}

void Sema::actOnImport(StringRef ModuleName, IdentList &Ids) {
  Diags.report(SMLoc(), diag::err_not_yet_implemented);
}

void Sema::actOnConstantDeclaration(DeclList &Decls, SMLoc Loc, StringRef Name,
                                    std::unique_ptr<Expr> E) {
  assert(CurrentScope && "CurrentScope not set");
  auto Decl = std::make_unique<ConstantDeclaration>(CurrentDecl, Loc, Name,
                                                    std::move(E));
  if (CurrentScope->insert(Decl.get()))
    Decls.push_back(std::move(Decl));
  else
    Diags.report(Loc, diag::err_symbold_declared, Name);
}

void Sema::actOnTypeDeclaration(DeclList &Decls, SMLoc Loc, StringRef Name,
                                Decl *D) {
  assert(CurrentScope && "CurrentScope not set");
  TypeDeclaration *Aliased = dyn_cast_or_null<TypeDeclaration>(D);
  if (!Aliased) {
    Diags.report(Loc, diag::err_typedecl_requires_type);
    return;
  }
  auto Decl = std::make_unique<TypeAliasDeclaration>(CurrentDecl, Loc, Name,
                                                     Aliased);
  // Give an anonymous record or array type the name it is declared under, so
  // diagnostics and AST dumps can refer to it.
  if (Aliased->getName().empty() && !isa<TypeAliasDeclaration>(Aliased))
    Aliased->setName(Name);
  if (CurrentScope->insert(Decl.get()))
    Decls.push_back(std::move(Decl));
  else
    Diags.report(Loc, diag::err_symbold_declared, Name);
}

TypeDeclaration *Sema::actOnArrayType(SMLoc Loc, std::unique_ptr<Expr> Low,
                                      std::unique_ptr<Expr> High,
                                      TypeDeclaration *ElementType) {
  int64_t LowBound = 0;
  int64_t HighBound = 0;
  if (!evalConstInt(Low.get(), LowBound) ||
      !evalConstInt(High.get(), HighBound)) {
    Diags.report(Loc, diag::err_array_bound_not_constant);
    LowBound = HighBound = 0;
  }
  if (LowBound > HighBound) {
    Diags.report(Loc, diag::err_array_bound_invalid);
    HighBound = LowBound;
  }
  auto ArrTy = std::make_unique<ArrayTypeDeclaration>(
      CurrentDecl, Loc, StringRef(), ElementType, LowBound, HighBound);
  TypeDeclaration *Result = ArrTy.get();
  OwnedTypes.push_back(std::move(ArrTy));
  return Result;
}

TypeDeclaration *Sema::actOnRecordType(SMLoc Loc, Decl *BaseType,
                                       DeclList Fields, DeclList Methods) {
  TypeDeclaration *Base = nullptr;
  if (BaseType) {
    Base = dyn_cast<TypeDeclaration>(BaseType);
    if (!Base ||
        !isa<RecordTypeDeclaration>(getUnderlyingType(Base))) {
      Diags.report(Loc, diag::err_extended_record_base_must_be_record);
      Base = nullptr;
    }
  }
  // A record that extends another one is represented by a prefix-compatible
  // struct, so both types carry the type descriptor pointer.
  auto *BaseRec = Base ? dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Base))
                       : nullptr;
  if (BaseRec)
    BaseRec->setHasTypeTag();
  auto RecTy = std::make_unique<RecordTypeDeclaration>(
      CurrentDecl, Loc, StringRef(), BaseRec, std::move(Fields));
  if (BaseRec)
    RecTy->setHasTypeTag();
  for (const auto &F : RecTy->getFields()) {
    if (BaseRec && BaseRec->lookupField(F->getName()))
      Diags.report(F->getLocation(), diag::err_field_conflicts_with_base,
                   F->getName());
  }
  // Methods declared in the record body get an implicit receiver of the
  // record type and are implemented outside of it.
  RecTy->setMethodDecls(std::move(Methods));
  for (const auto &MD : RecTy->getMethodDecls()) {
    auto *Method = cast<ProcedureDeclaration>(MD.get());
    Method->setReceiver(std::make_unique<FormalParameterDeclaration>(
        Method, Method->getLocation(), "self", RecTy.get(),
        /*IsVar=*/true, /*IsReceiver=*/true));
    bool Ok = true;
    for (auto *Own : RecTy->getMethods()) {
      if (Own->getName() == Method->getName()) {
        Diags.report(Method->getLocation(), diag::err_duplicate_method,
                     Method->getName());
        Ok = false;
        break;
      }
    }
    if (Ok)
      if (ProcedureDeclaration *Inherited =
              lookupMethod(RecTy.get(), Method->getName()))
        if (!hasSameSignature(Inherited, Method)) {
          Diags.report(Method->getLocation(),
                       diag::err_method_signature_mismatch,
                       Method->getName());
          Ok = false;
        }
    if (!Ok)
      continue;
    RecTy->addMethod(Method);
    DeclaredMethods.push_back(Method);
  }
  TypeDeclaration *Result = RecTy.get();
  OwnedTypes.push_back(std::move(RecTy));
  return Result;
}

std::unique_ptr<ProcedureDeclaration>
Sema::actOnMethodDeclaration(SMLoc Loc, StringRef Name) {
  auto Proc = std::make_unique<ProcedureDeclaration>(CurrentDecl, Loc, Name);
  Proc->setMethodDeclaration();
  return Proc;
}

void Sema::actOnMethodDeclaration(ProcedureDeclaration *ProcDecl,
                                  FormalParamList Params, Decl *RetType,
                                  SMLoc RetTypeLoc) {
  ProcDecl->setFormalParams(std::move(Params));
  auto *RetTypeDecl = dyn_cast_or_null<TypeDeclaration>(RetType);
  if (!RetTypeDecl && RetType)
    Diags.report(RetTypeLoc, diag::err_returntype_must_be_type);
  else if (RetTypeDecl)
    ProcDecl->setRetType(RetTypeDecl);
}

void Sema::actOnVariableDeclaration(DeclList &Decls, IdentList &Ids, Decl *D) {
  assert(CurrentScope && "CurrentScope not set");
  if (!D)
    return;
  if (TypeDeclaration *Ty = dyn_cast<TypeDeclaration>(D)) {
    for (auto &[Loc, Name] : Ids) {
      auto Decl =
          std::make_unique<VariableDeclaration>(CurrentDecl, Loc, Name, Ty);
      if (CurrentScope->insert(Decl.get()))
        Decls.push_back(std::move(Decl));
      else
        Diags.report(Loc, diag::err_symbold_declared, Name);
    }
  } else if (!Ids.empty()) {
    SMLoc Loc = Ids.front().first;
    Diags.report(Loc, diag::err_vardecl_requires_type);
  }
}

void Sema::actOnFieldDeclaration(DeclList &Fields, IdentList &Ids, Decl *D) {
  assert(CurrentScope && "CurrentScope not set");
  if (!D)
    return;
  TypeDeclaration *Ty = dyn_cast<TypeDeclaration>(D);
  if (!Ty) {
    if (!Ids.empty())
      Diags.report(Ids.front().first, diag::err_vardecl_requires_type);
    return;
  }
  for (auto &[Loc, Name] : Ids) {
    bool Duplicate = false;
    for (const auto &F : Fields) {
      if (F->getName() == Name) {
        Duplicate = true;
        break;
      }
    }
    if (Duplicate) {
      Diags.report(Loc, diag::err_symbold_declared, Name);
      continue;
    }
    Fields.push_back(
        std::make_unique<VariableDeclaration>(CurrentDecl, Loc, Name, Ty));
  }
}

void Sema::actOnFormalParameterDeclaration(FormalParamList &Params,
                                           IdentList &Ids, Decl *D,
                                           bool IsVar) {
  assert(CurrentScope && "CurrentScope not set");
  if (!D)
    return;
  if (TypeDeclaration *Ty = dyn_cast<TypeDeclaration>(D)) {
    if (!IsVar && isa<ArrayTypeDeclaration>(getUnderlyingType(Ty))) {
      if (!Ids.empty())
        Diags.report(Ids.front().first, diag::err_array_param_requires_var);
      return;
    }
    for (auto &[Loc, Name] : Ids) {
      auto Decl = std::make_unique<FormalParameterDeclaration>(
          CurrentDecl, Loc, Name, Ty, IsVar);
      if (CurrentScope->insert(Decl.get()))
        Params.push_back(std::move(Decl));
      else
        Diags.report(Loc, diag::err_symbold_declared, Name);
    }
  } else if (!Ids.empty()) {
    SMLoc Loc = Ids.front().first;
    Diags.report(Loc, diag::err_vardecl_requires_type);
  }
}

std::unique_ptr<ProcedureDeclaration>
Sema::actOnProcedureDeclaration(SMLoc Loc, StringRef Name, bool IsMethod) {
  auto P = std::make_unique<ProcedureDeclaration>(CurrentDecl, Loc, Name);
  // Type-bound procedures live in the method table of their receiver type,
  // not in the enclosing scope.
  if (!IsMethod && !CurrentScope->insert(P.get()))
    Diags.report(Loc, diag::err_symbold_declared, Name);
  return P;
}

void Sema::actOnReceiverParameter(ProcedureDeclaration *ProcDecl, SMLoc Loc,
                                  StringRef Name, Decl *D,
                                  FormalParamList &Params) {
  assert(CurrentScope && "CurrentScope not set");
  auto *Ty = dyn_cast_or_null<TypeDeclaration>(D);
  auto *RecTy =
      Ty ? dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Ty)) : nullptr;
  if (!RecTy) {
    Diags.report(Loc, diag::err_receiver_requires_record);
    return;
  }
  RecTy->setHasTypeTag();
  auto Param = std::make_unique<FormalParameterDeclaration>(
      ProcDecl, Loc, Name, RecTy, /*IsVar=*/true, /*IsReceiver=*/true);
  if (CurrentScope->insert(Param.get()))
    Params.push_back(std::move(Param));
  else
    Diags.report(Loc, diag::err_symbold_declared, Name);
}

void Sema::actOnProcedureHeading(ProcedureDeclaration *ProcDecl,
                                 FormalParamList Params, Decl *RetType,
                                 SMLoc RetTypeLoc) {
  ProcDecl->setFormalParams(std::move(Params));
  auto *RetTypeDecl = dyn_cast_or_null<TypeDeclaration>(RetType);
  if (!RetTypeDecl && RetType)
    Diags.report(RetTypeLoc, diag::err_returntype_must_be_type);
  else if (RetTypeDecl) {
    ProcDecl->setRetType(RetTypeDecl);
  }

  // Register a type-bound procedure with its receiver type, checking that an
  // override matches the inherited signature.
  if (FormalParameterDeclaration *Receiver = ProcDecl->getReceiver()) {
    auto *RecTy = dyn_cast<RecordTypeDeclaration>(
        getUnderlyingType(Receiver->getType()));
    RecTy->setHasTypeTag();
    // The implementation of a method declared inside the record body.
    for (auto *Own : RecTy->getMethods()) {
      if (Own->getName() != ProcDecl->getName())
        continue;
      if (!Own->isMethodDeclaration()) {
        Diags.report(ProcDecl->getLocation(), diag::err_duplicate_method,
                     ProcDecl->getName());
        return;
      }
      if (!hasSameSignature(Own, ProcDecl)) {
        Diags.report(ProcDecl->getLocation(),
                     diag::err_method_signature_mismatch,
                     ProcDecl->getName());
      }
      Own->setDefinition(ProcDecl);
      RecTy->replaceMethod(Own, ProcDecl);
      return;
    }
    if (ProcedureDeclaration *Inherited =
            lookupMethod(RecTy, ProcDecl->getName())) {
      if (!hasSameSignature(Inherited, ProcDecl)) {
        Diags.report(ProcDecl->getLocation(),
                     diag::err_method_signature_mismatch, ProcDecl->getName());
        return;
      }
    }
    RecTy->addMethod(ProcDecl);
  }
}

void Sema::actOnProcedureDeclaration(ProcedureDeclaration *ProcDecl, SMLoc Loc,
                                     StringRef Name, DeclList &Decls,
                                     StmtList &Stmts) {

  if (Name != ProcDecl->getName()) {
    Diags.report(Loc, diag::err_proc_identifier_not_equal);
    Diags.report(ProcDecl->getLocation(),
                 diag::note_proc_identifier_declaration);
  }
  ProcDecl->setDecls(std::move(Decls));
  ProcDecl->setStmts(std::move(Stmts));
}

void Sema::actOnAssignment(StmtList &Stmts, SMLoc Loc,
                           std::unique_ptr<Expr> Target,
                           std::unique_ptr<Expr> E) {
  if (!Target || !E)
    return;
  TypeDeclaration *Ty = nullptr;
  if (auto *Var = dyn_cast<VariableAccess>(Target.get())) {
    Decl *D = Var->getDecl();
    if (auto *VD = dyn_cast<VariableDeclaration>(D))
      Ty = VD->getType();
    else if (auto *FP = dyn_cast<FormalParameterDeclaration>(D))
      Ty = FP->getType();
    else {
      // TODO Emit error
      return;
    }
  } else if (auto *Idx = dyn_cast<IndexedExpression>(Target.get())) {
    Ty = Idx->getType();
  } else if (auto *Field = dyn_cast<FieldAccess>(Target.get())) {
    Ty = Field->getType();
  } else {
    return;
  }
  if (!isSameType(Ty, E->getType())) {
    Diags.report(Loc, diag::err_types_for_operator_not_compatible,
                 getOperatorSpelling(tok::colonequal));
  }
  Stmts.push_back(
      std::make_unique<AssignmentStatement>(std::move(Target), std::move(E)));
}

void Sema::actOnProcCall(StmtList &Stmts, SMLoc Loc, Decl *D,
                         ExprList Params) {
  if (!D)
    return;
  if (auto Proc = dyn_cast<ProcedureDeclaration>(D)) {
    if (Proc->isMethod()) {
      Diags.report(Loc, diag::err_method_requires_receiver, Proc->getName());
      return;
    }
    checkFormalAndActualParameters(Loc, Proc->getFormalParams(), Params);
    if (Proc->getRetType())
      Diags.report(Loc, diag::err_procedure_call_on_nonprocedure);
    Stmts.push_back(
        std::make_unique<ProcedureCallStatement>(Proc, std::move(Params)));
  } else if (D) {
    Diags.report(Loc, diag::err_procedure_call_on_nonprocedure);
  }
}

void Sema::actOnIfStatement(StmtList &Stmts, SMLoc Loc,
                            std::unique_ptr<Expr> Cond, StmtList IfStmts,
                            StmtList ElseStmts) {
  if (!Cond)
    Cond = std::make_unique<BooleanLiteral>(false, BooleanType.get());

  if (!isSameType(Cond->getType(), BooleanType.get())) {
    Diags.report(Loc, diag::err_if_expr_must_be_bool);
  }
  Stmts.push_back(std::make_unique<IfStatement>(
      std::move(Cond), std::move(IfStmts), std::move(ElseStmts)));
}

void Sema::actOnWhileStatement(StmtList &Stmts, SMLoc Loc,
                               std::unique_ptr<Expr> Cond,
                               StmtList WhileStmts) {
  if (!Cond)
    Cond = std::make_unique<BooleanLiteral>(false, BooleanType.get());

  if (!isSameType(Cond->getType(), BooleanType.get())) {
    Diags.report(Loc, diag::err_while_expr_must_be_bool);
  }
  Stmts.push_back(
      std::make_unique<WhileStatement>(std::move(Cond), std::move(WhileStmts)));
}

void Sema::actOnReturnStatement(StmtList &Stmts, SMLoc Loc,
                                std::unique_ptr<Expr> RetVal) {
  auto *Proc = cast<ProcedureDeclaration>(CurrentDecl);
  if (Proc->getRetType() && !RetVal)
    Diags.report(Loc, diag::err_function_requires_return);
  else if (!Proc->getRetType() && RetVal)
    Diags.report(Loc, diag::err_procedure_requires_empty_return);
  else if (Proc->getRetType() && RetVal) {
    if (!isSameType(Proc->getRetType(), RetVal->getType()))
      Diags.report(Loc, diag::err_function_and_return_type);
  }

  Stmts.push_back(std::make_unique<ReturnStatement>(std::move(RetVal)));
}

std::unique_ptr<Expr>
Sema::actOnExpression(std::unique_ptr<Expr> Left, std::unique_ptr<Expr> Right,
                      const OperatorInfo &Op) {
  // Relation
  if (!Left)
    return Right;
  if (!Right)
    return Left;

  if (!isSameType(Left->getType(), Right->getType())) {
    Diags.report(Op.getLocation(), diag::err_types_for_operator_not_compatible,
                 getOperatorSpelling(Op.getKind()));
  }
  bool IsConst = Left->isConst() && Right->isConst();
  return std::make_unique<InfixExpression>(std::move(Left), std::move(Right),
                                           Op, BooleanType.get(), IsConst);
}

std::unique_ptr<Expr>
Sema::actOnSimpleExpression(std::unique_ptr<Expr> Left,
                            std::unique_ptr<Expr> Right,
                            const OperatorInfo &Op) {
  // Addition
  if (!Left)
    return Right;
  if (!Right)
    return Left;

  if (!isSameType(Left->getType(), Right->getType())) {
    Diags.report(Op.getLocation(), diag::err_types_for_operator_not_compatible,
                 getOperatorSpelling(Op.getKind()));
  }
  TypeDeclaration *Ty = Left->getType();
  bool IsConst = Left->isConst() && Right->isConst();
  if (IsConst && Op.getKind() == tok::kw_OR) {
    if (auto *L = dyn_cast<BooleanLiteral>(Left.get()))
      if (auto *R = dyn_cast<BooleanLiteral>(Right.get()))
        return std::make_unique<BooleanLiteral>(L->getValue() || R->getValue(),
                                                BooleanType.get());
  }
  return std::make_unique<InfixExpression>(std::move(Left), std::move(Right),
                                           Op, Ty, IsConst);
}

std::unique_ptr<Expr> Sema::actOnTerm(std::unique_ptr<Expr> Left,
                                      std::unique_ptr<Expr> Right,
                                      const OperatorInfo &Op) {
  // Multiplication
  if (!Left)
    return Right;
  if (!Right)
    return Left;

  if (!isSameType(Left->getType(), Right->getType()) ||
      !isOperatorForType(Op.getKind(), Left->getType())) {
    Diags.report(Op.getLocation(), diag::err_types_for_operator_not_compatible,
                 getOperatorSpelling(Op.getKind()));
  }
  TypeDeclaration *Ty = Left->getType();
  bool IsConst = Left->isConst() && Right->isConst();
  if (IsConst && Op.getKind() == tok::kw_AND) {
    if (auto *L = dyn_cast<BooleanLiteral>(Left.get()))
      if (auto *R = dyn_cast<BooleanLiteral>(Right.get()))
        return std::make_unique<BooleanLiteral>(L->getValue() && R->getValue(),
                                                BooleanType.get());
  }
  return std::make_unique<InfixExpression>(std::move(Left), std::move(Right),
                                           Op, Ty, IsConst);
}

std::unique_ptr<Expr>
Sema::actOnPrefixExpression(std::unique_ptr<Expr> E, const OperatorInfo &Op) {
  if (!E)
    return nullptr;

  if (!isOperatorForType(Op.getKind(), E->getType())) {
    Diags.report(Op.getLocation(), diag::err_types_for_operator_not_compatible,
                 getOperatorSpelling(Op.getKind()));
  }

  if (E->isConst() && Op.getKind() == tok::kw_NOT) {
    if (auto *L = dyn_cast<BooleanLiteral>(E.get()))
      return std::make_unique<BooleanLiteral>(!L->getValue(),
                                              BooleanType.get());
  }

  TypeDeclaration *Ty = E->getType();
  bool IsConst = E->isConst();
  return std::make_unique<PrefixExpression>(std::move(E), Op, Ty, IsConst);
}

std::unique_ptr<Expr> Sema::actOnIntegerLiteral(SMLoc Loc, StringRef Literal) {
  uint8_t Radix = 10;
  if (Literal.ends_with("H")) {
    Literal = Literal.drop_back();
    Radix = 16;
  }
  // The lexer already diagnosed hex digits in decimal literals.  Do not feed
  // an invalid literal to APInt, which would abort the compiler.
  if (Radix == 10 &&
      !llvm::all_of(Literal, [](char C) { return C >= '0' && C <= '9'; }))
    Literal = "0";
  llvm::APInt Value(64, Literal, Radix);
  return std::make_unique<IntegerLiteral>(Loc, llvm::APSInt(Value, false),
                                          IntegerType.get());
}

std::unique_ptr<Expr> Sema::actOnRealLiteral(SMLoc Loc, StringRef Literal) {
  // The lexer guarantees the shape "digits '.' [digits] [E sign digits]".
  // Parse directly into IEEE single precision so the stored value is the
  // correctly rounded 32-bit float, not a double-rounded double.  Strings
  // that cannot be parsed (a malformed exponent, or the D exponent of a
  // LONGREAL literal that the lexer already diagnosed) become 0.0 so the
  // compiler never aborts on bad input.
  llvm::APFloat Value(llvm::APFloat::IEEEsingle());
  auto ParseResult =
      Value.convertFromString(Literal, llvm::APFloat::rmNearestTiesToEven);
  if (!ParseResult) {
    llvm::consumeError(ParseResult.takeError());
    Value = llvm::APFloat(0.0f);
  }
  return std::make_unique<RealLiteral>(Loc, Literal, Value, RealType.get());
}

std::unique_ptr<Expr> Sema::actOnVariable(Decl *D) {
  if (!D)
    return nullptr;
  if (auto *V = dyn_cast<VariableDeclaration>(D))
    return std::make_unique<VariableAccess>(V);
  else if (auto *P = dyn_cast<FormalParameterDeclaration>(D))
    return std::make_unique<VariableAccess>(P);
  else if (auto *C = dyn_cast<ConstantDeclaration>(D))
    return std::make_unique<ConstantAccess>(C);
  return nullptr;
}

std::unique_ptr<Expr> Sema::actOnIndexedExpression(SMLoc Loc,
                                                   std::unique_ptr<Expr> Base,
                                                   std::unique_ptr<Expr> Index) {
  if (!Base || !Index)
    return nullptr;
  auto *ArrTy =
      dyn_cast<ArrayTypeDeclaration>(getUnderlyingType(Base->getType()));
  if (!ArrTy) {
    Diags.report(Loc, diag::err_indexed_expression_requires_array);
    return nullptr;
  }
  if (!isSameType(Index->getType(), IntegerType.get())) {
    Diags.report(Loc, diag::err_index_expression_must_be_integer);
  }
  return std::make_unique<IndexedExpression>(
      std::move(Base), std::move(Index), ArrTy->getElementType());
}

std::unique_ptr<Expr> Sema::actOnFieldAccess(SMLoc Loc,
                                             std::unique_ptr<Expr> Base,
                                             StringRef Name) {
  if (!Base || !Base->getType())
    return nullptr;
  auto *RecTy =
      dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Base->getType()));
  if (!RecTy) {
    Diags.report(Loc, diag::err_field_access_requires_record);
    return nullptr;
  }
  if (VariableDeclaration *Field = RecTy->lookupField(Name))
    return std::make_unique<FieldAccess>(std::move(Base), Field);
  Diags.report(Loc, diag::err_undeclared_field, Name);
  return nullptr;
}

std::unique_ptr<Expr>
Sema::actOnMethodCall(SMLoc Loc, std::unique_ptr<Expr> Receiver,
                      StringRef Name, ExprList Params) {
  if (!Receiver)
    return nullptr;
  if (!isDesignator(Receiver.get())) {
    Diags.report(Loc, diag::err_method_call_requires_variable);
    return nullptr;
  }
  auto *RecTy = Receiver->getType()
                    ? dyn_cast<RecordTypeDeclaration>(
                          getUnderlyingType(Receiver->getType()))
                    : nullptr;
  if (!RecTy) {
    Diags.report(Loc, diag::err_method_call_requires_record);
    return nullptr;
  }
  ProcedureDeclaration *Method = lookupMethod(RecTy, Name);
  if (!Method) {
    Diags.report(Loc, diag::err_undeclared_method, Name);
    return nullptr;
  }
  // The visible method may be overridden by the dynamic type, so the
  // receiver type needs a type descriptor.
  RecTy->setHasTypeTag();
  checkFormalAndActualParameters(Loc, Method->getFormalParams(), Params);
  return std::make_unique<MethodCallExpr>(std::move(Receiver), Method,
                                          std::move(Params));
}

void Sema::actOnMethodCallStatement(StmtList &Stmts, SMLoc Loc,
                                    std::unique_ptr<Expr> E) {
  auto *Call = dyn_cast_or_null<MethodCallExpr>(E.get());
  if (!Call)
    return;
  if (Call->getType())
    Diags.report(Loc, diag::err_procedure_call_on_nonprocedure);
  Stmts.push_back(std::make_unique<MethodCallStatement>(std::move(E)));
}

std::unique_ptr<Expr> Sema::actOnTypeTest(SMLoc Loc, std::unique_ptr<Expr> E,
                                          Decl *D) {
  if (!E)
    return nullptr;
  auto *TestedTy = dyn_cast_or_null<TypeDeclaration>(D);
  auto *TestedRec = TestedTy ? dyn_cast<RecordTypeDeclaration>(
                                   getUnderlyingType(TestedTy))
                             : nullptr;
  auto *StaticRec = E->getType() ? dyn_cast<RecordTypeDeclaration>(
                                       getUnderlyingType(E->getType()))
                                 : nullptr;
  if (!TestedRec || !StaticRec || !isDesignator(E.get())) {
    Diags.report(Loc, diag::err_type_test_requires_record);
    return nullptr;
  }
  if (!isExtensionOf(TestedRec, StaticRec)) {
    Diags.report(Loc, diag::err_type_test_not_extension);
    return nullptr;
  }
  StaticRec->setHasTypeTag();
  TestedRec->setHasTypeTag();
  return std::make_unique<TypeTestExpr>(std::move(E), TestedRec,
                                        BooleanType.get());
}

std::unique_ptr<Expr> Sema::actOnFunctionCall(SMLoc Loc, Decl *D,
                                              ExprList Params) {
  if (!D)
    return nullptr;
  if (auto *P = dyn_cast<ProcedureDeclaration>(D)) {
    if (P->isMethod()) {
      Diags.report(Loc, diag::err_method_requires_receiver, P->getName());
      return nullptr;
    }
    checkFormalAndActualParameters(Loc, P->getFormalParams(), Params);
    if (!P->getRetType())
      Diags.report(Loc, diag::err_function_call_on_nonfunction);
    return std::make_unique<FunctionCallExpr>(P, std::move(Params));
  }
  Diags.report(Loc, diag::err_function_call_on_nonfunction);
  return nullptr;
}

Decl *Sema::actOnQualIdentPart(Decl *Prev, SMLoc Loc, StringRef Name) {
  if (!Prev) {
    if (Decl *D = CurrentScope->lookup(Name))
      return D;
  } else if (auto *Mod = dyn_cast<ModuleDeclaration>(Prev)) {
    const auto &Decls = Mod->getDecls();
    for (auto I = Decls.begin(), E = Decls.end(); I != E; ++I) {
      if ((*I)->getName() == Name) {
        return I->get();
      }
    }
  } else {
    llvm_unreachable("actOnQualIdentPart only callable "
                     "with module declarations");
  }
  Diags.report(Loc, diag::err_undeclared_name, Name);
  return nullptr;
}
