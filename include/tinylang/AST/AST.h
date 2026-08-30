#pragma once

#include "tinylang/Basic/TokenKinds.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/SMLoc.h"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tinylang {

using namespace llvm;

class Decl;
class FormalParameterDeclaration;
class Expr;
class Stmt;

using DeclList = std::vector<std::unique_ptr<Decl>>;
using FormalParamList = std::vector<std::unique_ptr<FormalParameterDeclaration>>;
using ExprList = std::vector<std::unique_ptr<Expr>>;
using StmtList = std::vector<std::unique_ptr<Stmt>>;

class Ident {
  SMLoc Loc;
  StringRef Name;

public:
  Ident(SMLoc Loc, const StringRef &Name) : Loc(Loc), Name(Name) {}
  SMLoc getLocation() { return Loc; }
  const StringRef &getName() { return Name; }
};

using IdentList = std::vector<std::pair<SMLoc, StringRef>>;

class Decl {
public:
  enum DeclKind {
    DK_Module,
    DK_Const,
    DK_Type,
    DK_ArrayType,
    DK_TypeAlias,
    DK_Var,
    DK_Param,
    DK_Proc
  };

private:
  const DeclKind Kind;

protected:
  Decl *EnclosingDecl;
  SMLoc Loc;
  StringRef Name;

public:
  Decl(DeclKind Kind, Decl *EnclosingDecl, SMLoc Loc, StringRef Name)
      : Kind(Kind), EnclosingDecl(EnclosingDecl), Loc(Loc), Name(Name) {}
  virtual ~Decl() = default;

  DeclKind getKind() const { return Kind; }
  SMLoc getLocation() { return Loc; }
  StringRef getName() { return Name; }
  Decl *getEnclosingDecl() { return EnclosingDecl; }
};

class ModuleDeclaration : public Decl {
  DeclList Decls;
  StmtList Stmts;

public:
  ModuleDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name)
      : Decl(DK_Module, EnclosingDecl, Loc, Name) {}

  ModuleDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name,
                    DeclList Decls, StmtList Stmts)
      : Decl(DK_Module, EnclosingDecl, Loc, Name), Decls(std::move(Decls)),
        Stmts(std::move(Stmts)) {}

  const DeclList &getDecls() { return Decls; }
  void setDecls(DeclList D) { Decls = std::move(D); }
  const StmtList &getStmts() { return Stmts; }
  void setStmts(StmtList L) { Stmts = std::move(L); }

  static bool classof(const Decl *D) { return D->getKind() == DK_Module; }
};

class ConstantDeclaration : public Decl {
  std::unique_ptr<Expr> E;

public:
  ConstantDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name,
                      std::unique_ptr<Expr> E)
      : Decl(DK_Const, EnclosingDecl, Loc, Name), E(std::move(E)) {}

  Expr *getExpr() { return E.get(); }

  static bool classof(const Decl *D) { return D->getKind() == DK_Const; }
};

class TypeDeclaration : public Decl {
protected:
  TypeDeclaration(DeclKind Kind, Decl *EnclosingDecl, SMLoc Loc,
                  StringRef Name)
      : Decl(Kind, EnclosingDecl, Loc, Name) {}

public:
  TypeDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name)
      : TypeDeclaration(DK_Type, EnclosingDecl, Loc, Name) {}

  static bool classof(const Decl *D) {
    return D->getKind() == DK_Type || D->getKind() == DK_ArrayType ||
           D->getKind() == DK_TypeAlias;
  }
};

/// A named type alias, e.g. TYPE T = U.  The aliased type is referenced, not
/// owned; it is resolved to its underlying type wherever type compatibility
/// and LLVM type conversion are performed.
class TypeAliasDeclaration : public TypeDeclaration {
  TypeDeclaration *Aliased;

public:
  TypeAliasDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name,
                       TypeDeclaration *Aliased)
      : TypeDeclaration(DK_TypeAlias, EnclosingDecl, Loc, Name),
        Aliased(Aliased) {}

  TypeDeclaration *getAliasedType() { return Aliased; }

  static bool classof(const Decl *D) {
    return D->getKind() == DK_TypeAlias;
  }
};

/// Resolves chains of type aliases to the first non-alias type.
inline TypeDeclaration *getUnderlyingType(TypeDeclaration *Ty) {
  while (auto *Alias = dyn_cast<TypeAliasDeclaration>(Ty))
    Ty = Alias->getAliasedType();
  return Ty;
}

/// A static array type, e.g. ARRAY [1..10] OF INTEGER.  The element type is
/// referenced (not owned); bounds are inclusive and fixed at compile time.
class ArrayTypeDeclaration : public TypeDeclaration {
  TypeDeclaration *ElementType;
  int64_t LowBound;
  int64_t HighBound;

public:
  ArrayTypeDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name,
                       TypeDeclaration *ElementType, int64_t LowBound,
                       int64_t HighBound)
      : TypeDeclaration(DK_ArrayType, EnclosingDecl, Loc, Name),
        ElementType(ElementType), LowBound(LowBound), HighBound(HighBound) {}

  TypeDeclaration *getElementType() { return ElementType; }
  int64_t getLowBound() const { return LowBound; }
  int64_t getHighBound() const { return HighBound; }
  int64_t getNumElements() const { return HighBound - LowBound + 1; }

  static bool classof(const Decl *D) { return D->getKind() == DK_ArrayType; }
};

class VariableDeclaration : public Decl {
  TypeDeclaration *Ty;

public:
  VariableDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name,
                      TypeDeclaration *Ty)
      : Decl(DK_Var, EnclosingDecl, Loc, Name), Ty(Ty) {}

  TypeDeclaration *getType() { return Ty; }

  static bool classof(const Decl *D) { return D->getKind() == DK_Var; }
};

class FormalParameterDeclaration : public Decl {
  TypeDeclaration *Ty;
  bool IsVar;

public:
  FormalParameterDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name,
                             TypeDeclaration *Ty, bool IsVar)
      : Decl(DK_Param, EnclosingDecl, Loc, Name), Ty(Ty), IsVar(IsVar) {}

  TypeDeclaration *getType() { return Ty; }
  bool isVar() { return IsVar; }

  static bool classof(const Decl *D) { return D->getKind() == DK_Param; }
};

class ProcedureDeclaration : public Decl {
  FormalParamList Params;
  TypeDeclaration *RetType;
  DeclList Decls;
  StmtList Stmts;

public:
  ProcedureDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name)
      : Decl(DK_Proc, EnclosingDecl, Loc, Name) {}

  ProcedureDeclaration(Decl *EnclosingDecl, SMLoc Loc, StringRef Name,
                       FormalParamList Params, TypeDeclaration *RetType,
                       DeclList Decls, StmtList Stmts)
      : Decl(DK_Proc, EnclosingDecl, Loc, Name), Params(std::move(Params)),
        RetType(RetType), Decls(std::move(Decls)), Stmts(std::move(Stmts)) {}

  const FormalParamList &getFormalParams() { return Params; }
  void setFormalParams(FormalParamList FP) { Params = std::move(FP); }
  TypeDeclaration *getRetType() { return RetType; }
  void setRetType(TypeDeclaration *Ty) { RetType = Ty; }

  const DeclList &getDecls() { return Decls; }
  void setDecls(DeclList D) { Decls = std::move(D); }
  const StmtList &getStmts() { return Stmts; }
  void setStmts(StmtList L) { Stmts = std::move(L); }

  static bool classof(const Decl *D) { return D->getKind() == DK_Proc; }
};

class OperatorInfo {
  SMLoc Loc;
  uint32_t Kind : 16;
  uint32_t IsUnspecified : 1;

public:
  OperatorInfo() : Loc(), Kind(tok::unknown), IsUnspecified(true) {}
  OperatorInfo(SMLoc Loc, tok::TokenKind Kind, bool IsUnspecified = false)
      : Loc(Loc), Kind(Kind), IsUnspecified(IsUnspecified) {}

  SMLoc getLocation() const { return Loc; }
  tok::TokenKind getKind() const { return static_cast<tok::TokenKind>(Kind); }
  bool isUnspecified() const { return IsUnspecified; }
};

class Expr {
public:
  enum ExprKind {
    EK_Infix,
    EK_Prefix,
    EK_Int,
    EK_Bool,
    EK_Var,
    EK_Const,
    EK_Func,
    EK_Indexed,
  };

private:
  const ExprKind Kind;
  TypeDeclaration *Ty;
  bool IsConstant;

protected:
  Expr(ExprKind Kind, TypeDeclaration *Ty, bool IsConst)
      : Kind(Kind), Ty(Ty), IsConstant(IsConst) {}

public:
  virtual ~Expr() = default;
  ExprKind getKind() const { return Kind; }
  TypeDeclaration *getType() { return Ty; }
  void setType(TypeDeclaration *T) { Ty = T; }
  bool isConst() { return IsConstant; }
};

class InfixExpression : public Expr {
  std::unique_ptr<Expr> Left;
  std::unique_ptr<Expr> Right;
  const OperatorInfo Op;

public:
  InfixExpression(std::unique_ptr<Expr> Left, std::unique_ptr<Expr> Right,
                  OperatorInfo Op, TypeDeclaration *Ty, bool IsConst)
      : Expr(EK_Infix, Ty, IsConst), Left(std::move(Left)),
        Right(std::move(Right)), Op(Op) {}

  Expr *getLeft() { return Left.get(); }
  Expr *getRight() { return Right.get(); }
  const OperatorInfo &getOperatorInfo() { return Op; }

  static bool classof(const Expr *E) { return E->getKind() == EK_Infix; }
};

class PrefixExpression : public Expr {
  std::unique_ptr<Expr> E;
  const OperatorInfo Op;

public:
  PrefixExpression(std::unique_ptr<Expr> E, OperatorInfo Op,
                   TypeDeclaration *Ty, bool IsConst)
      : Expr(EK_Prefix, Ty, IsConst), E(std::move(E)), Op(Op) {}

  Expr *getExpr() { return E.get(); }
  const OperatorInfo &getOperatorInfo() { return Op; }

  static bool classof(const Expr *E) { return E->getKind() == EK_Prefix; }
};

class IntegerLiteral : public Expr {
  SMLoc Loc;
  llvm::APSInt Value;

public:
  IntegerLiteral(SMLoc Loc, const llvm::APSInt &Value, TypeDeclaration *Ty)
      : Expr(EK_Int, Ty, true), Loc(Loc), Value(Value) {}
  llvm::APSInt &getValue() { return Value; }

  static bool classof(const Expr *E) { return E->getKind() == EK_Int; }
};

class BooleanLiteral : public Expr {
  bool Value;

public:
  BooleanLiteral(bool Value, TypeDeclaration *Ty)
      : Expr(EK_Bool, Ty, true), Value(Value) {}
  bool getValue() { return Value; }

  static bool classof(const Expr *E) { return E->getKind() == EK_Bool; }
};

class VariableAccess : public Expr {
  Decl *Var;

public:
  VariableAccess(VariableDeclaration *Var)
      : Expr(EK_Var, Var->getType(), false), Var(Var) {}
  VariableAccess(FormalParameterDeclaration *Param)
      : Expr(EK_Var, Param->getType(), false), Var(Param) {}

  Decl *getDecl() { return Var; }

  static bool classof(const Expr *E) { return E->getKind() == EK_Var; }
};

class ConstantAccess : public Expr {
  ConstantDeclaration *Const;

public:
  ConstantAccess(ConstantDeclaration *Const)
      : Expr(EK_Const, Const->getExpr()->getType(), true), Const(Const) {}

  ConstantDeclaration *getDecl() { return Const; }

  static bool classof(const Expr *E) { return E->getKind() == EK_Const; }
};

class FunctionCallExpr : public Expr {
  ProcedureDeclaration *Proc;
  ExprList Params;

public:
  FunctionCallExpr(ProcedureDeclaration *Proc, ExprList Params)
      : Expr(EK_Func, Proc->getRetType(), false), Proc(Proc),
        Params(std::move(Params)) {}

  ProcedureDeclaration *getDecl() { return Proc; }
  const ExprList &getParams() { return Params; }

  static bool classof(const Expr *E) { return E->getKind() == EK_Func; }
};

class IndexedExpression : public Expr {
  std::unique_ptr<Expr> Base;
  std::unique_ptr<Expr> Index;

public:
  IndexedExpression(std::unique_ptr<Expr> Base, std::unique_ptr<Expr> Index,
                    TypeDeclaration *Ty)
      : Expr(EK_Indexed, Ty, false), Base(std::move(Base)),
        Index(std::move(Index)) {}

  Expr *getBase() { return Base.get(); }
  Expr *getIndex() { return Index.get(); }

  static bool classof(const Expr *E) { return E->getKind() == EK_Indexed; }
};

class Stmt {
public:
  enum StmtKind { SK_Assign, SK_ProcCall, SK_If, SK_While, SK_Return };

private:
  const StmtKind Kind;

protected:
  Stmt(StmtKind Kind) : Kind(Kind) {}

public:
  virtual ~Stmt() = default;
  StmtKind getKind() const { return Kind; }
};

class AssignmentStatement : public Stmt {
  std::unique_ptr<Expr> Target;
  std::unique_ptr<Expr> E;

public:
  AssignmentStatement(std::unique_ptr<Expr> Target, std::unique_ptr<Expr> E)
      : Stmt(SK_Assign), Target(std::move(Target)), E(std::move(E)) {}

  Expr *getTarget() { return Target.get(); }
  Expr *getExpr() { return E.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == SK_Assign; }
};

class ProcedureCallStatement : public Stmt {
  ProcedureDeclaration *Proc;
  ExprList Params;

public:
  ProcedureCallStatement(ProcedureDeclaration *Proc, ExprList Params)
      : Stmt(SK_ProcCall), Proc(Proc), Params(std::move(Params)) {}

  ProcedureDeclaration *getProc() { return Proc; }
  const ExprList &getParams() { return Params; }

  static bool classof(const Stmt *S) { return S->getKind() == SK_ProcCall; }
};

class IfStatement : public Stmt {
  std::unique_ptr<Expr> Cond;
  StmtList IfStmts;
  StmtList ElseStmts;

public:
  IfStatement(std::unique_ptr<Expr> Cond, StmtList IfStmts, StmtList ElseStmts)
      : Stmt(SK_If), Cond(std::move(Cond)), IfStmts(std::move(IfStmts)),
        ElseStmts(std::move(ElseStmts)) {}

  Expr *getCond() { return Cond.get(); }
  const StmtList &getIfStmts() { return IfStmts; }
  const StmtList &getElseStmts() { return ElseStmts; }

  static bool classof(const Stmt *S) { return S->getKind() == SK_If; }
};

class WhileStatement : public Stmt {
  std::unique_ptr<Expr> Cond;
  StmtList Stmts;

public:
  WhileStatement(std::unique_ptr<Expr> Cond, StmtList Stmts)
      : Stmt(SK_While), Cond(std::move(Cond)), Stmts(std::move(Stmts)) {}

  Expr *getCond() { return Cond.get(); }
  const StmtList &getWhileStmts() { return Stmts; }

  static bool classof(const Stmt *S) { return S->getKind() == SK_While; }
};

class ReturnStatement : public Stmt {
  std::unique_ptr<Expr> RetVal;

public:
  ReturnStatement(std::unique_ptr<Expr> RetVal)
      : Stmt(SK_Return), RetVal(std::move(RetVal)) {}

  Expr *getRetVal() { return RetVal.get(); }

  static bool classof(const Stmt *S) { return S->getKind() == SK_Return; }
};

} // namespace tinylang
