#include "tinylang/ASTDumper/ASTDumper.h"
#include "tinylang/Basic/TokenKinds.h"
#include "llvm/Support/Casting.h"

using namespace tinylang;

namespace {

StringRef getOperatorSpelling(tok::TokenKind Kind) {
  if (const char *P = tok::getPunctuatorSpelling(Kind))
    return P;
  return tok::getKeywordSpelling(Kind);
}

} // namespace

void ASTDumper::printIndent() {
  for (unsigned I = 0; I < Indent; ++I)
    OS << "  ";
}

void ASTDumper::dump(ModuleDeclaration *M) {
  dumpModule(M);
}

void ASTDumper::dumpModule(ModuleDeclaration *M) {
  printIndent();
  OS << "ModuleDeclaration '" << M->getName() << "'\n";
  ++Indent;
  dumpDeclList("Declarations", M->getDecls());
  dumpStmtList("Statements", M->getStmts());
  --Indent;
}

void ASTDumper::dumpDeclList(StringRef Label, const DeclList &Decls) {
  if (Decls.empty())
    return;
  printIndent();
  OS << Label << ":\n";
  ++Indent;
  for (const auto &D : Decls)
    dumpDecl(D.get());
  --Indent;
}

void ASTDumper::dumpStmtList(StringRef Label, const StmtList &Stmts) {
  if (Stmts.empty())
    return;
  printIndent();
  OS << Label << ":\n";
  ++Indent;
  for (const auto &S : Stmts)
    dumpStmt(S.get());
  --Indent;
}

void ASTDumper::dumpExprList(StringRef Label, const ExprList &Exprs) {
  if (Exprs.empty())
    return;
  printIndent();
  OS << Label << ":\n";
  ++Indent;
  for (const auto &E : Exprs)
    dumpExpr(E.get());
  --Indent;
}

void ASTDumper::dumpDecl(Decl *D) {
  switch (D->getKind()) {
  case Decl::DK_Module:
    dumpModule(cast<ModuleDeclaration>(D));
    break;
  case Decl::DK_Const:
    dumpConstant(cast<ConstantDeclaration>(D));
    break;
  case Decl::DK_Type:
    dumpType(cast<TypeDeclaration>(D));
    break;
  case Decl::DK_Var:
    dumpVariable(cast<VariableDeclaration>(D));
    break;
  case Decl::DK_Param:
    dumpFormalParameter(cast<FormalParameterDeclaration>(D));
    break;
  case Decl::DK_Proc:
    dumpProcedure(cast<ProcedureDeclaration>(D));
    break;
  }
}

void ASTDumper::dumpConstant(ConstantDeclaration *C) {
  printIndent();
  OS << "ConstantDeclaration '" << C->getName() << "'\n";
  ++Indent;
  dumpExpr(C->getExpr());
  --Indent;
}

void ASTDumper::dumpType(TypeDeclaration *T) {
  printIndent();
  OS << "TypeDeclaration '" << T->getName() << "'\n";
}

void ASTDumper::dumpVariable(VariableDeclaration *V) {
  printIndent();
  OS << "VariableDeclaration '" << V->getName() << "' : "
     << V->getType()->getName() << "\n";
}

void ASTDumper::dumpFormalParameter(FormalParameterDeclaration *P) {
  printIndent();
  OS << "FormalParameterDeclaration '" << P->getName() << "' : "
     << P->getType()->getName();
  if (P->isVar())
    OS << " (VAR)";
  OS << "\n";
}

void ASTDumper::dumpFormalParams(const FormalParamList &Params) {
  if (Params.empty())
    return;
  printIndent();
  OS << "FormalParameters:\n";
  ++Indent;
  for (const auto &P : Params)
    dumpFormalParameter(P.get());
  --Indent;
}

void ASTDumper::dumpProcedure(ProcedureDeclaration *P) {
  printIndent();
  OS << "ProcedureDeclaration '" << P->getName() << "'";
  if (P->getRetType())
    OS << " : " << P->getRetType()->getName();
  OS << "\n";
  ++Indent;
  dumpFormalParams(P->getFormalParams());
  dumpDeclList("Declarations", P->getDecls());
  dumpStmtList("Statements", P->getStmts());
  --Indent;
}

void ASTDumper::dumpStmt(Stmt *S) {
  switch (S->getKind()) {
  case Stmt::SK_Assign:
    dumpAssignment(cast<AssignmentStatement>(S));
    break;
  case Stmt::SK_ProcCall:
    dumpProcCall(cast<ProcedureCallStatement>(S));
    break;
  case Stmt::SK_If:
    dumpIf(cast<IfStatement>(S));
    break;
  case Stmt::SK_While:
    dumpWhile(cast<WhileStatement>(S));
    break;
  case Stmt::SK_Return:
    dumpReturn(cast<ReturnStatement>(S));
    break;
  }
}

void ASTDumper::dumpAssignment(AssignmentStatement *S) {
  printIndent();
  OS << "AssignmentStatement\n";
  ++Indent;
  printIndent();
  OS << "Target: '" << S->getVar()->getName() << "'\n";
  printIndent();
  OS << "Value:\n";
  ++Indent;
  dumpExpr(S->getExpr());
  --Indent;
  --Indent;
}

void ASTDumper::dumpProcCall(ProcedureCallStatement *S) {
  printIndent();
  OS << "ProcedureCallStatement '" << S->getProc()->getName() << "'\n";
  ++Indent;
  dumpExprList("Arguments", S->getParams());
  --Indent;
}

void ASTDumper::dumpIf(IfStatement *S) {
  printIndent();
  OS << "IfStatement\n";
  ++Indent;
  printIndent();
  OS << "Condition:\n";
  ++Indent;
  dumpExpr(S->getCond());
  --Indent;
  dumpStmtList("Then", S->getIfStmts());
  dumpStmtList("Else", S->getElseStmts());
  --Indent;
}

void ASTDumper::dumpWhile(WhileStatement *S) {
  printIndent();
  OS << "WhileStatement\n";
  ++Indent;
  printIndent();
  OS << "Condition:\n";
  ++Indent;
  dumpExpr(S->getCond());
  --Indent;
  dumpStmtList("Body", S->getWhileStmts());
  --Indent;
}

void ASTDumper::dumpReturn(ReturnStatement *S) {
  printIndent();
  OS << "ReturnStatement\n";
  ++Indent;
  if (Expr *RetVal = S->getRetVal())
    dumpExpr(RetVal);
  --Indent;
}

void ASTDumper::dumpExpr(Expr *E) {
  switch (E->getKind()) {
  case Expr::EK_Infix:
    dumpInfix(cast<InfixExpression>(E));
    break;
  case Expr::EK_Prefix:
    dumpPrefix(cast<PrefixExpression>(E));
    break;
  case Expr::EK_Int:
    dumpIntLiteral(cast<IntegerLiteral>(E));
    break;
  case Expr::EK_Bool:
    dumpBoolLiteral(cast<BooleanLiteral>(E));
    break;
  case Expr::EK_Var:
    dumpVariableAccess(cast<VariableAccess>(E));
    break;
  case Expr::EK_Const:
    dumpConstantAccess(cast<ConstantAccess>(E));
    break;
  case Expr::EK_Func:
    dumpFuncCall(cast<FunctionCallExpr>(E));
    break;
  }
}

void ASTDumper::dumpInfix(InfixExpression *E) {
  printIndent();
  OS << "InfixExpression '" << getOperatorSpelling(E->getOperatorInfo().getKind())
     << "' : " << E->getType()->getName() << "\n";
  ++Indent;
  dumpExpr(E->getLeft());
  dumpExpr(E->getRight());
  --Indent;
}

void ASTDumper::dumpPrefix(PrefixExpression *E) {
  printIndent();
  OS << "PrefixExpression '"
     << getOperatorSpelling(E->getOperatorInfo().getKind())
     << "' : " << E->getType()->getName() << "\n";
  ++Indent;
  dumpExpr(E->getExpr());
  --Indent;
}

void ASTDumper::dumpIntLiteral(IntegerLiteral *E) {
  printIndent();
  OS << "IntegerLiteral ";
  E->getValue().print(OS, /*IsSigned=*/true);
  OS << " : " << E->getType()->getName() << "\n";
}

void ASTDumper::dumpBoolLiteral(BooleanLiteral *E) {
  printIndent();
  OS << "BooleanLiteral " << (E->getValue() ? "true" : "false") << " : "
     << E->getType()->getName() << "\n";
}

void ASTDumper::dumpVariableAccess(VariableAccess *E) {
  printIndent();
  OS << "VariableAccess '" << E->getDecl()->getName() << "' : "
     << E->getType()->getName() << "\n";
}

void ASTDumper::dumpConstantAccess(ConstantAccess *E) {
  printIndent();
  OS << "ConstantAccess '" << E->getDecl()->getName() << "' : "
     << E->getType()->getName() << "\n";
}

void ASTDumper::dumpFuncCall(FunctionCallExpr *E) {
  printIndent();
  OS << "FunctionCallExpr '" << E->getDecl()->getName() << "' : "
     << E->getType()->getName() << "\n";
  ++Indent;
  dumpExprList("Arguments", E->getParams());
  --Indent;
}
