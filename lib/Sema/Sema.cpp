#include "tinylang/Sema/Sema.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace tinylang;

static StringRef getOperatorSpelling(tok::TokenKind Kind) {
  if (const char *P = tok::getPunctuatorSpelling(Kind))
    return P;
  return tok::getKeywordSpelling(Kind);
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
  case tok::kw_DIV:
  case tok::kw_MOD:
    return Ty == IntegerType.get();
  case tok::slash:
    return false; // REAL not implemented
  case tok::kw_AND:
  case tok::kw_OR:
  case tok::kw_NOT:
    return Ty == BooleanType.get();
  default:
    llvm_unreachable("Unknown operator");
  }
}

bool Sema::isSameType(TypeDeclaration *LHS, TypeDeclaration *RHS) {
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

void Sema::checkFormalAndActualParameters(SMLoc Loc,
                                          const FormalParamList &Formals,
                                          const ExprList &Actuals) {
  if (Formals.size() != Actuals.size()) {
    Diags.report(Loc, diag::err_wrong_number_of_parameters);
    return;
  }
  auto A = Actuals.begin();
  for (auto I = Formals.begin(), E = Formals.end(); I != E; ++I, ++A) {
    FormalParameterDeclaration *F = I->get();
    Expr *Arg = A->get();
    if (!Arg)
      continue;
    if (!isSameType(F->getType(), Arg->getType()))
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
  BooleanType =
      std::make_unique<TypeDeclaration>(CurrentDecl, SMLoc(), "BOOLEAN");
  TrueConst = std::make_unique<ConstantDeclaration>(
      CurrentDecl, SMLoc(), "TRUE",
      std::make_unique<BooleanLiteral>(true, BooleanType.get()));
  FalseConst = std::make_unique<ConstantDeclaration>(
      CurrentDecl, SMLoc(), "FALSE",
      std::make_unique<BooleanLiteral>(false, BooleanType.get()));
  CurrentScope->insert(IntegerType.get());
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

TypeDeclaration *Sema::actOnRecordType(SMLoc Loc, DeclList Fields) {
  auto RecTy = std::make_unique<RecordTypeDeclaration>(
      CurrentDecl, Loc, StringRef(), std::move(Fields));
  TypeDeclaration *Result = RecTy.get();
  OwnedTypes.push_back(std::move(RecTy));
  return Result;
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
Sema::actOnProcedureDeclaration(SMLoc Loc, StringRef Name) {
  auto P = std::make_unique<ProcedureDeclaration>(CurrentDecl, Loc, Name);
  if (!CurrentScope->insert(P.get()))
    Diags.report(Loc, diag::err_symbold_declared, Name);
  return P;
}

void Sema::actOnProcedureHeading(ProcedureDeclaration *ProcDecl,
                                 FormalParamList Params, Decl *RetType,
                                 SMLoc RetTypeLoc) {
  ProcDecl->setFormalParams(std::move(Params));
  auto *RetTypeDecl = dyn_cast_or_null<TypeDeclaration>(RetType);
  if (!RetTypeDecl && RetType)
    Diags.report(RetTypeLoc, diag::err_returntype_must_be_type);
  else if (RetTypeDecl) {
    TypeDeclaration *UT = getUnderlyingType(RetTypeDecl);
    if (isa<ArrayTypeDeclaration>(UT) || isa<RecordTypeDeclaration>(UT))
      Diags.report(RetTypeLoc, diag::err_aggregate_return_not_supported);
    ProcDecl->setRetType(RetTypeDecl);
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
  if (isa<ArrayTypeDeclaration>(getUnderlyingType(Ty))) {
    Diags.report(Loc, diag::err_array_assignment_not_supported);
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

  if (Op.getKind() == tok::minus) {
    bool Ambiguous = true;
    if (isa<IntegerLiteral>(E.get()) || isa<VariableAccess>(E.get()) ||
        isa<ConstantAccess>(E.get()))
      Ambiguous = false;
    else if (auto *Infix = dyn_cast<InfixExpression>(E.get())) {
      tok::TokenKind Kind = Infix->getOperatorInfo().getKind();
      if (Kind == tok::star || Kind == tok::slash)
        Ambiguous = false;
    }
    if (Ambiguous) {
      Diags.report(Op.getLocation(), diag::warn_ambigous_negation);
    }
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
  if (!Base)
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

std::unique_ptr<Expr> Sema::actOnFunctionCall(SMLoc Loc, Decl *D,
                                              ExprList Params) {
  if (!D)
    return nullptr;
  if (auto *P = dyn_cast<ProcedureDeclaration>(D)) {
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
