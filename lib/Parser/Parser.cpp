#include "tinylang/Parser/Parser.h"
#include "tinylang/Basic/TokenKinds.h"

using namespace tinylang;

namespace {
OperatorInfo fromTok(Token Tok) {
  return OperatorInfo(Tok.getLocation(), Tok.getKind());
}
} // namespace

Parser::Parser(Lexer &Lex, Sema &Actions) : Lex(Lex), Actions(Actions) {
  advance();
}

std::unique_ptr<ModuleDeclaration> Parser::parse() {
  std::unique_ptr<ModuleDeclaration> ModDecl;
  parseCompilationUnit(ModDecl);
  return ModDecl;
}

bool Parser::parseCompilationUnit(std::unique_ptr<ModuleDeclaration> &D) {
  auto _errorhandler = [this] { return skipUntil(); };
  if (consume(tok::kw_MODULE))
    return _errorhandler();
  if (expect(tok::identifier))
    return _errorhandler();
  D = Actions.actOnModuleDeclaration(Tok.getLocation(), Tok.getIdentifier());

  EnterDeclScope S(Actions, D.get());
  advance();
  if (consume(tok::semi))
    return _errorhandler();
  while (Tok.isOneOf(tok::kw_FROM, tok::kw_IMPORT)) {
    if (parseImport())
      return _errorhandler();
  }
  DeclList Decls;
  StmtList Stmts;
  if (parseBlock(Decls, Stmts))
    return _errorhandler();
  if (expect(tok::identifier))
    return _errorhandler();
  Actions.actOnModuleDeclaration(D.get(), Tok.getLocation(),
                                 Tok.getIdentifier(), Decls, Stmts);
  advance();
  if (consume(tok::period))
    return _errorhandler();
  return false;
}

bool Parser::parseImport() {
  auto _errorhandler = [this] {
    return skipUntil(tok::kw_BEGIN, tok::kw_CONST, tok::kw_END, tok::kw_FROM,
                     tok::kw_IMPORT, tok::kw_PROCEDURE, tok::kw_TYPE,
                     tok::kw_VAR);
  };
  IdentList Ids;
  StringRef ModuleName;
  if (Tok.is(tok::kw_FROM)) {
    advance();
    if (expect(tok::identifier))
      return _errorhandler();
    ModuleName = Tok.getIdentifier();
    advance();
  }
  if (consume(tok::kw_IMPORT))
    return _errorhandler();
  if (parseIdentList(Ids))
    return _errorhandler();
  if (expect(tok::semi))
    return _errorhandler();
  Actions.actOnImport(ModuleName, Ids);
  advance();
  return false;
}

bool Parser::parseBlock(DeclList &Decls, StmtList &Stmts) {
  auto _errorhandler = [this] { return skipUntil(tok::identifier); };
  while (Tok.isOneOf(tok::kw_CONST, tok::kw_PROCEDURE, tok::kw_TYPE,
                     tok::kw_VAR)) {
    if (parseDeclaration(Decls))
      return _errorhandler();
  }
  if (Tok.is(tok::kw_BEGIN)) {
    advance();
    if (parseStatementSequence(Stmts))
      return _errorhandler();
  }
  if (consume(tok::kw_END))
    return _errorhandler();
  return false;
}

bool Parser::parseDeclaration(DeclList &Decls) {
  auto _errorhandler = [this] {
    return skipUntil(tok::kw_BEGIN, tok::kw_CONST, tok::kw_END,
                     tok::kw_PROCEDURE, tok::kw_TYPE, tok::kw_VAR);
  };
  if (Tok.is(tok::kw_CONST)) {
    advance();
    while (Tok.is(tok::identifier)) {
      if (parseConstantDeclaration(Decls))
        return _errorhandler();
      if (consume(tok::semi))
        return _errorhandler();
    }
  } else if (Tok.is(tok::kw_VAR)) {
    advance();
    while (Tok.is(tok::identifier)) {
      if (parseVariableDeclaration(Decls))
        return _errorhandler();
      if (consume(tok::semi))
        return _errorhandler();
    }
  } else if (Tok.is(tok::kw_TYPE)) {
    advance();
    while (Tok.is(tok::identifier)) {
      if (parseTypeDeclaration(Decls))
        return _errorhandler();
      if (consume(tok::semi))
        return _errorhandler();
    }
  } else if (Tok.is(tok::kw_PROCEDURE)) {
    if (parseProcedureDeclaration(Decls))
      return _errorhandler();
    if (consume(tok::semi))
      return _errorhandler();
  } else {
    /*ERROR*/
    return _errorhandler();
  }
  return false;
}

bool Parser::parseConstantDeclaration(DeclList &Decls) {
  auto _errorhandler = [this] { return skipUntil(tok::semi); };
  if (expect(tok::identifier))
    return _errorhandler();
  SMLoc Loc = Tok.getLocation();

  StringRef Name = Tok.getIdentifier();
  advance();
  if (expect(tok::equal))
    return _errorhandler();
  std::unique_ptr<Expr> E;
  advance();
  if (parseExpression(E))
    return _errorhandler();
  Actions.actOnConstantDeclaration(Decls, Loc, Name, std::move(E));
  return false;
}

bool Parser::parseTypeDeclaration(DeclList &Decls) {
  auto _errorhandler = [this] { return skipUntil(tok::semi); };
  if (expect(tok::identifier))
    return _errorhandler();
  SMLoc Loc = Tok.getLocation();

  StringRef Name = Tok.getIdentifier();
  advance();
  if (expect(tok::equal))
    return _errorhandler();
  Decl *D = nullptr;
  advance();
  if (parseType(D))
    return _errorhandler();
  Actions.actOnTypeDeclaration(Decls, Loc, Name, D);
  return false;
}

bool Parser::parseVariableDeclaration(DeclList &Decls) {
  auto _errorhandler = [this] { return skipUntil(tok::semi); };
  Decl *D;
  IdentList Ids;
  if (parseIdentList(Ids))
    return _errorhandler();
  if (consume(tok::colon))
    return _errorhandler();
  if (parseType(D))
    return _errorhandler();
  Actions.actOnVariableDeclaration(Decls, Ids, D);
  return false;
}

bool Parser::parseType(Decl *&D) {
  auto _errorhandler = [this] { return skipUntil(tok::semi); };
  if (!Tok.is(tok::kw_ARRAY))
    return parseQualident(D);

  SMLoc Loc = Tok.getLocation();
  advance(); // ARRAY

  // Collect all index ranges: [low..high] (, [low..high])*
  std::vector<std::unique_ptr<Expr>> Lows, Highs;
  while (true) {
    if (consume(tok::l_bracket))
      return _errorhandler();
    std::unique_ptr<Expr> Low, High;
    if (parseExpression(Low))
      return _errorhandler();
    if (consume(tok::dotdot))
      return _errorhandler();
    if (parseExpression(High))
      return _errorhandler();
    if (consume(tok::r_bracket))
      return _errorhandler();
    Lows.push_back(std::move(Low));
    Highs.push_back(std::move(High));
    if (!Tok.is(tok::comma))
      break;
    advance();
  }

  if (consume(tok::kw_OF))
    return _errorhandler();
  Decl *ElemDecl = nullptr;
  if (parseType(ElemDecl))
    return _errorhandler();
  TypeDeclaration *ElemTy = dyn_cast_or_null<TypeDeclaration>(ElemDecl);
  if (!ElemTy) {
    getDiagnostics().report(Loc, diag::err_vardecl_requires_type);
    return _errorhandler();
  }

  // Build nested array types from the innermost dimension outwards:
  // ARRAY [1..2], [3..4] OF T  ==  ARRAY [1..2] OF ARRAY [3..4] OF T
  TypeDeclaration *Ty = ElemTy;
  for (size_t I = Lows.size(); I > 0; --I) {
    size_t J = I - 1;
    Ty = Actions.actOnArrayType(Loc, std::move(Lows[J]), std::move(Highs[J]),
                                Ty);
  }
  D = Ty;
  return false;
}

bool Parser::parseProcedureDeclaration(DeclList &ParentDecls) {
  auto _errorhandler = [this] { return skipUntil(tok::semi); };
  if (consume(tok::kw_PROCEDURE))
    return _errorhandler();
  if (expect(tok::identifier))
    return _errorhandler();
  auto D =
      Actions.actOnProcedureDeclaration(Tok.getLocation(), Tok.getIdentifier());

  EnterDeclScope S(Actions, D.get());
  FormalParamList Params;
  Decl *RetType = nullptr;
  SMLoc RetTypeLoc;
  advance();
  if (Tok.is(tok::l_paren)) {
    if (parseFormalParameters(Params, RetType, RetTypeLoc))
      return _errorhandler();
  }
  Actions.actOnProcedureHeading(D.get(), std::move(Params), RetType,
                                RetTypeLoc);
  if (expect(tok::semi))
    return _errorhandler();
  DeclList Decls;
  StmtList Stmts;
  advance();
  if (parseBlock(Decls, Stmts))
    return _errorhandler();
  if (expect(tok::identifier))
    return _errorhandler();
  Actions.actOnProcedureDeclaration(D.get(), Tok.getLocation(),
                                    Tok.getIdentifier(), Decls, Stmts);

  ParentDecls.push_back(std::move(D));
  advance();
  return false;
}

bool Parser::parseFormalParameters(FormalParamList &Params, Decl *&RetType,
                                   SMLoc &RetTypeLoc) {
  auto _errorhandler = [this] { return skipUntil(tok::semi); };
  if (consume(tok::l_paren))
    return _errorhandler();
  if (Tok.isOneOf(tok::kw_VAR, tok::identifier)) {
    if (parseFormalParameterList(Params))
      return _errorhandler();
  }
  if (consume(tok::r_paren))
    return _errorhandler();
  if (Tok.is(tok::colon)) {
    advance();
    RetTypeLoc = Tok.getLocation();
    if (parseQualident(RetType))
      return _errorhandler();
  }
  return false;
}

bool Parser::parseFormalParameterList(FormalParamList &Params) {
  auto _errorhandler = [this] { return skipUntil(tok::r_paren); };
  if (parseFormalParameter(Params))
    return _errorhandler();
  while (Tok.is(tok::semi)) {
    advance();
    if (parseFormalParameter(Params))
      return _errorhandler();
  }
  return false;
}

bool Parser::parseFormalParameter(FormalParamList &Params) {
  auto _errorhandler = [this] { return skipUntil(tok::r_paren, tok::semi); };
  IdentList Ids;
  Decl *D;
  bool IsVar = false;
  if (Tok.is(tok::kw_VAR)) {
    IsVar = true;
    advance();
  }
  if (parseIdentList(Ids))
    return _errorhandler();
  if (consume(tok::colon))
    return _errorhandler();
  if (parseType(D))
    return _errorhandler();
  Actions.actOnFormalParameterDeclaration(Params, Ids, D, IsVar);
  return false;
}

bool Parser::parseStatementSequence(StmtList &Stmts) {
  auto _errorhandler = [this] { return skipUntil(tok::kw_ELSE, tok::kw_END); };
  if (parseStatement(Stmts))
    return _errorhandler();
  while (Tok.is(tok::semi)) {
    advance();
    if (parseStatement(Stmts))
      return _errorhandler();
  }
  return false;
}

bool Parser::parseStatement(StmtList &Stmts) {
  auto _errorhandler = [this] {
    return skipUntil(tok::semi, tok::kw_ELSE, tok::kw_END);
  };
  if (Tok.is(tok::identifier)) {
    Decl *D;
    std::unique_ptr<Expr> E, Target;
    SMLoc Loc = Tok.getLocation();
    if (parseQualident(D))
      return _errorhandler();
    if (Tok.is(tok::l_paren)) {
      ExprList Exprs;
      if (Tok.is(tok::l_paren)) {
        advance();
        if (Tok.isOneOf(tok::l_paren, tok::plus, tok::minus, tok::kw_NOT,
                        tok::identifier, tok::integer_literal)) {
          if (parseExpList(Exprs))
            return _errorhandler();
        }
        if (consume(tok::r_paren))
          return _errorhandler();
      }
      Actions.actOnProcCall(Stmts, Loc, D, std::move(Exprs));
    } else {
      // Designator on the left-hand side, possibly indexed: a, a[i], m[i][j].
      Target = Actions.actOnVariable(D);
      while (Tok.is(tok::l_bracket)) {
        SMLoc IdxLoc = Tok.getLocation();
        std::unique_ptr<Expr> Index;
        advance();
        if (parseExpression(Index))
          return _errorhandler();
        if (expect(tok::r_bracket))
          return _errorhandler();
        Target = Actions.actOnIndexedExpression(IdxLoc, std::move(Target),
                                                std::move(Index));
        advance();
      }
      if (Tok.is(tok::colonequal)) {
        advance();
        if (parseExpression(E))
          return _errorhandler();
        Actions.actOnAssignment(Stmts, Loc, std::move(Target), std::move(E));
      }
    }
  } else if (Tok.is(tok::kw_IF)) {
    if (parseIfStatement(Stmts))
      return _errorhandler();
  } else if (Tok.is(tok::kw_WHILE)) {
    if (parseWhileStatement(Stmts))
      return _errorhandler();
  } else if (Tok.is(tok::kw_RETURN)) {
    if (parseReturnStatement(Stmts))
      return _errorhandler();
  } else {
    /*ERROR*/
    return _errorhandler();
  }
  return false;
}

bool Parser::parseIfStatement(StmtList &Stmts) {
  auto _errorhandler = [this] {
    return skipUntil(tok::semi, tok::kw_ELSE, tok::kw_END);
  };
  std::unique_ptr<Expr> E;
  StmtList IfStmts, ElseStmts;
  SMLoc Loc = Tok.getLocation();
  if (consume(tok::kw_IF))
    return _errorhandler();
  if (parseExpression(E))
    return _errorhandler();
  if (consume(tok::kw_THEN))
    return _errorhandler();
  if (parseStatementSequence(IfStmts))
    return _errorhandler();
  if (Tok.is(tok::kw_ELSE)) {
    advance();
    if (parseStatementSequence(ElseStmts))
      return _errorhandler();
  }
  if (expect(tok::kw_END))
    return _errorhandler();
  Actions.actOnIfStatement(Stmts, Loc, std::move(E), std::move(IfStmts),
                           std::move(ElseStmts));
  advance();
  return false;
}

bool Parser::parseWhileStatement(StmtList &Stmts) {
  auto _errorhandler = [this] {
    return skipUntil(tok::semi, tok::kw_ELSE, tok::kw_END);
  };
  std::unique_ptr<Expr> E;
  StmtList WhileStmts;
  SMLoc Loc = Tok.getLocation();
  if (consume(tok::kw_WHILE))
    return _errorhandler();
  if (parseExpression(E))
    return _errorhandler();
  if (consume(tok::kw_DO))
    return _errorhandler();
  if (parseStatementSequence(WhileStmts))
    return _errorhandler();
  if (expect(tok::kw_END))
    return _errorhandler();
  Actions.actOnWhileStatement(Stmts, Loc, std::move(E),
                              std::move(WhileStmts));
  advance();
  return false;
}

bool Parser::parseReturnStatement(StmtList &Stmts) {
  auto _errorhandler = [this] {
    return skipUntil(tok::semi, tok::kw_ELSE, tok::kw_END);
  };
  std::unique_ptr<Expr> E;
  SMLoc Loc = Tok.getLocation();
  if (consume(tok::kw_RETURN))
    return _errorhandler();
  if (Tok.isOneOf(tok::l_paren, tok::plus, tok::minus, tok::kw_NOT,
                  tok::identifier, tok::integer_literal)) {
    if (parseExpression(E))
      return _errorhandler();
  }
  Actions.actOnReturnStatement(Stmts, Loc, std::move(E));
  return false;
}

bool Parser::parseExpList(ExprList &Exprs) {
  auto _errorhandler = [this] { return skipUntil(tok::r_paren); };
  std::unique_ptr<Expr> E;
  if (parseExpression(E))
    return _errorhandler();
  if (E)
    Exprs.push_back(std::move(E));
  while (Tok.is(tok::comma)) {
    E = nullptr;
    advance();
    if (parseExpression(E))
      return _errorhandler();
    if (E)
      Exprs.push_back(std::move(E));
  }
  return false;
}

bool Parser::parseExpression(std::unique_ptr<Expr> &E) {
  auto _errorhandler = [this] {
    return skipUntil(tok::r_paren, tok::comma, tok::semi, tok::kw_DO,
                     tok::kw_ELSE, tok::kw_END, tok::kw_THEN);
  };
  if (parseSimpleExpression(E))
    return _errorhandler();
  if (Tok.isOneOf(tok::hash, tok::less, tok::lessequal, tok::equal,
                  tok::greater, tok::greaterequal)) {
    OperatorInfo Op;
    std::unique_ptr<Expr> Right;
    if (parseRelation(Op))
      return _errorhandler();
    if (parseSimpleExpression(Right))
      return _errorhandler();
    E = Actions.actOnExpression(std::move(E), std::move(Right), Op);
  }
  return false;
}

bool Parser::parseRelation(OperatorInfo &Op) {
  auto _errorhandler = [this] {
    return skipUntil(tok::l_paren, tok::plus, tok::minus, tok::kw_NOT,
                     tok::identifier, tok::integer_literal);
  };
  if (Tok.is(tok::equal)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::hash)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::less)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::lessequal)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::greater)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::greaterequal)) {
    Op = fromTok(Tok);
    advance();
  } else {
    /*ERROR*/
    return _errorhandler();
  }
  return false;
}

bool Parser::parseSimpleExpression(std::unique_ptr<Expr> &E) {
  auto _errorhandler = [this] {
    return skipUntil(tok::hash, tok::r_paren, tok::comma, tok::semi, tok::less,
                     tok::lessequal, tok::equal, tok::greater,
                     tok::greaterequal, tok::kw_DO, tok::kw_ELSE, tok::kw_END,
                     tok::kw_THEN);
  };
  OperatorInfo PrefixOp;
  if (Tok.isOneOf(tok::plus, tok::minus)) {
    if (Tok.is(tok::plus)) {
      PrefixOp = fromTok(Tok);
      advance();
    } else if (Tok.is(tok::minus)) {
      PrefixOp = fromTok(Tok);
      advance();
    }
  }
  if (parseTerm(E))
    return _errorhandler();
  while (Tok.isOneOf(tok::plus, tok::minus, tok::kw_OR)) {
    OperatorInfo Op;
    std::unique_ptr<Expr> Right;
    if (parseAddOperator(Op))
      return _errorhandler();
    if (parseTerm(Right))
      return _errorhandler();
    E = Actions.actOnSimpleExpression(std::move(E), std::move(Right), Op);
  }
  if (!PrefixOp.isUnspecified())

    E = Actions.actOnPrefixExpression(std::move(E), PrefixOp);
  return false;
}

bool Parser::parseAddOperator(OperatorInfo &Op) {
  auto _errorhandler = [this] {
    return skipUntil(tok::l_paren, tok::kw_NOT, tok::identifier,
                     tok::integer_literal);
  };
  if (Tok.is(tok::plus)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::minus)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::kw_OR)) {
    Op = fromTok(Tok);
    advance();
  } else {
    /*ERROR*/
    return _errorhandler();
  }
  return false;
}

bool Parser::parseTerm(std::unique_ptr<Expr> &E) {
  auto _errorhandler = [this] {
    return skipUntil(tok::hash, tok::r_paren, tok::plus, tok::comma, tok::minus,
                     tok::semi, tok::less, tok::lessequal, tok::equal,
                     tok::greater, tok::greaterequal, tok::kw_DO, tok::kw_ELSE,
                     tok::kw_END, tok::kw_OR, tok::kw_THEN);
  };
  if (parseFactor(E))
    return _errorhandler();
  while (Tok.isOneOf(tok::star, tok::slash, tok::kw_AND, tok::kw_DIV,
                     tok::kw_MOD)) {
    OperatorInfo Op;
    std::unique_ptr<Expr> Right;
    if (parseMulOperator(Op))
      return _errorhandler();
    if (parseFactor(Right))
      return _errorhandler();
    E = Actions.actOnTerm(std::move(E), std::move(Right), Op);
  }
  return false;
}

bool Parser::parseMulOperator(OperatorInfo &Op) {
  auto _errorhandler = [this] {
    return skipUntil(tok::l_paren, tok::kw_NOT, tok::identifier,
                     tok::integer_literal);
  };
  if (Tok.is(tok::star)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::slash)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::kw_DIV)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::kw_MOD)) {
    Op = fromTok(Tok);
    advance();
  } else if (Tok.is(tok::kw_AND)) {
    Op = fromTok(Tok);
    advance();
  } else {
    /*ERROR*/
    return _errorhandler();
  }
  return false;
}

bool Parser::parseFactor(std::unique_ptr<Expr> &E) {
  auto _errorhandler = [this] {
    return skipUntil(
        tok::hash, tok::r_paren, tok::star, tok::plus, tok::comma, tok::minus,
        tok::slash, tok::semi, tok::less, tok::lessequal, tok::equal,
        tok::greater, tok::greaterequal, tok::kw_AND, tok::kw_DIV, tok::kw_DO,
        tok::kw_ELSE, tok::kw_END, tok::kw_MOD, tok::kw_OR, tok::kw_THEN);
  };
  if (Tok.is(tok::integer_literal)) {
    E = Actions.actOnIntegerLiteral(Tok.getLocation(), Tok.getLiteralData());
    advance();
  } else if (Tok.is(tok::identifier)) {
    Decl *D;
    ExprList Exprs;
    SMLoc Loc = Tok.getLocation();
    if (parseQualident(D))
      return _errorhandler();
    if (Tok.is(tok::l_paren)) {
      advance();
      if (Tok.isOneOf(tok::l_paren, tok::plus, tok::minus, tok::kw_NOT,
                      tok::identifier, tok::integer_literal)) {
        if (parseExpList(Exprs))
          return _errorhandler();
      }
      if (expect(tok::r_paren))
        return _errorhandler();
      E = Actions.actOnFunctionCall(Loc, D, std::move(Exprs));
      advance();
    } else {
      E = Actions.actOnVariable(D);
      while (Tok.is(tok::l_bracket)) {
        SMLoc IdxLoc = Tok.getLocation();
        std::unique_ptr<Expr> Index;
        advance();
        if (parseExpression(Index))
          return _errorhandler();
        if (expect(tok::r_bracket))
          return _errorhandler();
        E = Actions.actOnIndexedExpression(IdxLoc, std::move(E),
                                           std::move(Index));
        advance();
      }
    }
  } else if (Tok.is(tok::l_paren)) {
    advance();
    if (parseExpression(E))
      return _errorhandler();
    if (consume(tok::r_paren))
      return _errorhandler();
  } else if (Tok.is(tok::kw_NOT)) {
    OperatorInfo Op = fromTok(Tok);
    advance();
    if (parseFactor(E))
      return _errorhandler();
    E = Actions.actOnPrefixExpression(std::move(E), Op);
  } else {
    /*ERROR*/
    return _errorhandler();
  }
  return false;
}

bool Parser::parseQualident(Decl *&D) {
  auto _errorhandler = [this] {
    return skipUntil(tok::hash, tok::l_paren, tok::r_paren, tok::star,
                     tok::plus, tok::comma, tok::minus, tok::slash,
                     tok::colonequal, tok::semi, tok::less, tok::lessequal,
                     tok::equal, tok::greater, tok::greaterequal, tok::kw_AND,
                     tok::kw_DIV, tok::kw_DO, tok::kw_ELSE, tok::kw_END,
                     tok::kw_MOD, tok::kw_OR, tok::kw_THEN);
  };
  D = nullptr;
  if (expect(tok::identifier))
    return _errorhandler();
  D = Actions.actOnQualIdentPart(D, Tok.getLocation(), Tok.getIdentifier());
  advance();
  while (Tok.is(tok::period) && D && isa<ModuleDeclaration>(D)) {
    advance();
    if (expect(tok::identifier))
      return _errorhandler();
    D = Actions.actOnQualIdentPart(D, Tok.getLocation(), Tok.getIdentifier());
    advance();
  }
  return false;
}

bool Parser::parseIdentList(IdentList &Ids) {
  auto _errorhandler = [this] { return skipUntil(tok::colon, tok::semi); };
  if (expect(tok::identifier))
    return _errorhandler();
  Ids.push_back(
      std::pair<SMLoc, StringRef>(Tok.getLocation(), Tok.getIdentifier()));
  advance();
  while (Tok.is(tok::comma)) {
    advance();
    if (expect(tok::identifier))
      return _errorhandler();
    Ids.push_back(
        std::pair<SMLoc, StringRef>(Tok.getLocation(), Tok.getIdentifier()));
    advance();
  }
  return false;
}
