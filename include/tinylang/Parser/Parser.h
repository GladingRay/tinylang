#pragma once

#include "tinylang/Basic/Diagnostic.h"
#include "tinylang/Lexer/Lexer.h"
#include "tinylang/Sema/Sema.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

namespace tinylang {

class Parser {

  Lexer &Lex;

  Sema &Actions;

  Token Tok;

  DiagnosticsEngine &getDiagnostics() const { return Lex.getDiagnostics(); }

  void advance() { Lex.next(Tok); }

  bool expect(tok::TokenKind ExpectedTok) {
    if (Tok.is(ExpectedTok)) {
      return false;
    }
    // There must be a better way!
    const char *Expected = tok::getPunctuatorSpelling(ExpectedTok);
    if (!Expected)
      Expected = tok::getKeywordSpelling(ExpectedTok);
    if (!Expected)
      Expected = tok::getTokenName(ExpectedTok);
    llvm::StringRef Actual(Tok.getLocation().getPointer(), Tok.getLength());
    getDiagnostics().report(Tok.getLocation(), diag::err_expected, Expected,
                            Actual);
    return true;
  }

  bool consume(tok::TokenKind ExpectedTok) {
    if (Tok.is(ExpectedTok)) {
      advance();
      return false;
    }
    return true;
  }

  template <typename... Tokens> bool skipUntil(Tokens &&...Toks) {
    while (true) {
      if ((... || Tok.is(Toks)))
        return false;

      if (Tok.is(tok::eof))
        return true;
      advance();
    }
  }

  bool parseCompilationUnit(std::unique_ptr<ModuleDeclaration> &D);
  bool parseImport();
  bool parseBlock(DeclList &Decls, StmtList &Stmts);
  bool parseDeclaration(DeclList &Decls);
  bool parseConstantDeclaration(DeclList &Decls);
  bool parseTypeDeclaration(DeclList &Decls);
  bool parseVariableDeclaration(DeclList &Decls);
  bool parseType(Decl *&D);
  bool parseRecordType(Decl *&D);
  bool parseFieldList(DeclList &Fields);
  bool parseProcedureDeclaration(DeclList &ParentDecls);
  bool parseFormalParameters(FormalParamList &Params, Decl *&RetType,
                             SMLoc &RetTypeLoc);
  bool parseFormalParameterList(FormalParamList &Params);
  bool parseFormalParameter(FormalParamList &Params);
  bool parseStatementSequence(StmtList &Stmts);
  bool parseStatement(StmtList &Stmts);
  bool parseIfStatement(StmtList &Stmts);
  bool parseWhileStatement(StmtList &Stmts);
  bool parseReturnStatement(StmtList &Stmts);
  bool parseExpList(ExprList &Exprs);
  bool parseExpression(std::unique_ptr<Expr> &E);
  bool parseRelation(OperatorInfo &Op);
  bool parseSimpleExpression(std::unique_ptr<Expr> &E);
  bool parseAddOperator(OperatorInfo &Op);
  bool parseTerm(std::unique_ptr<Expr> &E);
  bool parseMulOperator(OperatorInfo &Op);
  bool parseFactor(std::unique_ptr<Expr> &E);
  bool parseQualident(Decl *&D);
  bool parseDesignator(std::unique_ptr<Expr> &E);
  bool parseIdentList(IdentList &Ids);

public:
  Parser(Lexer &Lex, Sema &Actions);

  std::unique_ptr<ModuleDeclaration> parse();
};
} // namespace tinylang
