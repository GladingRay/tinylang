#pragma once

#include "tinylang/AST/AST.h"
#include "tinylang/Basic/Diagnostic.h"
#include "tinylang/Sema/Scope.h"
#include <memory>

namespace tinylang {

class Sema {
  friend class EnterDeclScope;
  void enterScope(Decl *);
  void leaveScope();

  bool isOperatorForType(tok::TokenKind Op, TypeDeclaration *Ty);

  bool isSameType(TypeDeclaration *LHS, TypeDeclaration *RHS);

  void checkFormalAndActualParameters(SMLoc Loc, const FormalParamList &Formals,
                                      const ExprList &Actuals);

  Scope *CurrentScope;
  Decl *CurrentDecl;
  DiagnosticsEngine &Diags;

  std::unique_ptr<TypeDeclaration> IntegerType;
  std::unique_ptr<TypeDeclaration> RealType;
  std::unique_ptr<TypeDeclaration> BooleanType;
  std::unique_ptr<ConstantDeclaration> TrueConst;
  std::unique_ptr<ConstantDeclaration> FalseConst;
  /// Anonymous array types created while parsing type declarations.
  std::vector<std::unique_ptr<TypeDeclaration>> OwnedTypes;

public:
  Sema(DiagnosticsEngine &Diags)
      : CurrentScope(nullptr), CurrentDecl(nullptr), Diags(Diags) {
    initialize();
  }
  ~Sema() { delete CurrentScope; }

  void initialize();

  std::unique_ptr<ModuleDeclaration> actOnModuleDeclaration(SMLoc Loc,
                                                            StringRef Name);
  void actOnModuleDeclaration(ModuleDeclaration *ModDecl, SMLoc Loc,
                              StringRef Name, DeclList &Decls, StmtList &Stmts);
  void actOnImport(StringRef ModuleName, IdentList &Ids);
  void actOnConstantDeclaration(DeclList &Decls, SMLoc Loc, StringRef Name,
                                std::unique_ptr<Expr> E);
  void actOnTypeDeclaration(DeclList &Decls, SMLoc Loc, StringRef Name,
                            Decl *D);
  TypeDeclaration *actOnArrayType(SMLoc Loc, std::unique_ptr<Expr> Low,
                                  std::unique_ptr<Expr> High,
                                  TypeDeclaration *ElementType);
  TypeDeclaration *actOnRecordType(SMLoc Loc, DeclList Fields);
  void actOnVariableDeclaration(DeclList &Decls, IdentList &Ids, Decl *D);
  void actOnFieldDeclaration(DeclList &Fields, IdentList &Ids, Decl *D);
  void actOnFormalParameterDeclaration(FormalParamList &Params, IdentList &Ids,
                                       Decl *D, bool IsVar);
  std::unique_ptr<ProcedureDeclaration> actOnProcedureDeclaration(SMLoc Loc,
                                                                  StringRef Name);
  void actOnProcedureHeading(ProcedureDeclaration *ProcDecl,
                             FormalParamList Params, Decl *RetType,
                             SMLoc RetTypeLoc);
  void actOnProcedureDeclaration(ProcedureDeclaration *ProcDecl, SMLoc Loc,
                                 StringRef Name, DeclList &Decls,
                                 StmtList &Stmts);
  void actOnAssignment(StmtList &Stmts, SMLoc Loc,
                       std::unique_ptr<Expr> Target, std::unique_ptr<Expr> E);
  void actOnProcCall(StmtList &Stmts, SMLoc Loc, Decl *D, ExprList Params);
  void actOnIfStatement(StmtList &Stmts, SMLoc Loc, std::unique_ptr<Expr> Cond,
                        StmtList IfStmts, StmtList ElseStmts);
  void actOnWhileStatement(StmtList &Stmts, SMLoc Loc,
                           std::unique_ptr<Expr> Cond, StmtList WhileStmts);
  void actOnReturnStatement(StmtList &Stmts, SMLoc Loc,
                            std::unique_ptr<Expr> RetVal);

  std::unique_ptr<Expr> actOnExpression(std::unique_ptr<Expr> Left,
                                        std::unique_ptr<Expr> Right,
                                        const OperatorInfo &Op);
  std::unique_ptr<Expr> actOnSimpleExpression(std::unique_ptr<Expr> Left,
                                              std::unique_ptr<Expr> Right,
                                              const OperatorInfo &Op);
  std::unique_ptr<Expr> actOnTerm(std::unique_ptr<Expr> Left,
                                  std::unique_ptr<Expr> Right,
                                  const OperatorInfo &Op);
  std::unique_ptr<Expr> actOnPrefixExpression(std::unique_ptr<Expr> E,
                                              const OperatorInfo &Op);
  std::unique_ptr<Expr> actOnIntegerLiteral(SMLoc Loc, StringRef Literal);
  std::unique_ptr<Expr> actOnRealLiteral(SMLoc Loc, StringRef Literal);
  std::unique_ptr<Expr> actOnVariable(Decl *D);
  std::unique_ptr<Expr> actOnIndexedExpression(SMLoc Loc,
                                               std::unique_ptr<Expr> Base,
                                               std::unique_ptr<Expr> Index);
  std::unique_ptr<Expr> actOnFieldAccess(SMLoc Loc,
                                         std::unique_ptr<Expr> Base,
                                         StringRef Name);
  std::unique_ptr<Expr> actOnFunctionCall(SMLoc Loc, Decl *D,
                                          ExprList Params);
  Decl *actOnQualIdentPart(Decl *Prev, SMLoc Loc, StringRef Name);
};

class EnterDeclScope {
  Sema &Semantics;

public:
  EnterDeclScope(Sema &Semantics, Decl *D) : Semantics(Semantics) {
    Semantics.enterScope(D);
  }
  ~EnterDeclScope() { Semantics.leaveScope(); }
};
} // namespace tinylang
