#pragma once

#include "tinylang/AST/AST.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

namespace tinylang {

/// Prints the abstract syntax tree of a module in a nested,
/// indentation-based layout so that the nesting of declarations,
/// statements and expressions is directly visible.
class ASTDumper {
  llvm::raw_ostream &OS;
  unsigned Indent;

  void printIndent();

  void dumpDecl(Decl *D);
  void dumpDeclList(llvm::StringRef Label, const DeclList &Decls);
  void dumpStmt(Stmt *S);
  void dumpStmtList(llvm::StringRef Label, const StmtList &Stmts);
  void dumpExpr(Expr *E);
  void dumpExprList(llvm::StringRef Label, const ExprList &Exprs);

  void dumpModule(ModuleDeclaration *M);
  void dumpConstant(ConstantDeclaration *C);
  void dumpType(TypeDeclaration *T);
  void dumpTypeAlias(TypeAliasDeclaration *T);
  void dumpTypeName(TypeDeclaration *T);
  void dumpVariable(VariableDeclaration *V);
  void dumpFormalParameter(FormalParameterDeclaration *P);
  void dumpFormalParams(const FormalParamList &Params);
  void dumpProcedure(ProcedureDeclaration *P);

  void dumpAssignment(AssignmentStatement *S);
  void dumpProcCall(ProcedureCallStatement *S);
  void dumpIf(IfStatement *S);
  void dumpWhile(WhileStatement *S);
  void dumpReturn(ReturnStatement *S);

  void dumpInfix(InfixExpression *E);
  void dumpPrefix(PrefixExpression *E);
  void dumpIntLiteral(IntegerLiteral *E);
  void dumpBoolLiteral(BooleanLiteral *E);
  void dumpVariableAccess(VariableAccess *E);
  void dumpConstantAccess(ConstantAccess *E);
  void dumpFuncCall(FunctionCallExpr *E);
  void dumpIndexedExpression(IndexedExpression *E);

public:
  ASTDumper(llvm::raw_ostream &OS) : OS(OS), Indent(0) {}

  void dump(ModuleDeclaration *M);
};

} // namespace tinylang
