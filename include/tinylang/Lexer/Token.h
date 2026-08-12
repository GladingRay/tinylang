#pragma once

#include "tinylang/Basic/TokenKinds.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SMLoc.h"

namespace tinylang {
class Lexer;

class Token {
  friend class Lexer;

  const char *Ptr;
  size_t Length;
  tok::TokenKind Kind;

public:
  tok::TokenKind getKind() const { return this->Kind; }

  void setKind(tok::TokenKind K) { this->Kind = K; }

  bool is(tok::TokenKind K) const { return this->Kind == K; }
  bool isNot(tok::TokenKind K) const { return this->Kind != K; }

  template <typename... Tokens>
  bool isOneOf(Tokens &&... Toks) const {
    return (... || is(Toks));
  }

  const char *getName() const {
    return tok::getTokenName(Kind);
  }

  llvm::SMLoc getLocation() const {
    return llvm::SMLoc::getFromPointer(Ptr);
  }

  size_t getLength() const { return Length; }

  llvm::StringRef getIdentifier() {
    return llvm::StringRef(Ptr, Length);
  }

  llvm::StringRef getLiteralData() {
    return llvm::StringRef(Ptr, Length);
  }
};
}