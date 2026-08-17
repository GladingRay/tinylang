#pragma once

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"

using namespace llvm;

namespace tinylang {

class Decl;

class Scope {
  Scope *Parent;
  StringMap<Decl *> Symbols;

public:
  Scope(Scope *Parent = nullptr) : Parent(Parent) {}

  bool insert(Decl *Declaration);
  Decl *lookup(StringRef Name);

  Scope *getParent() { return Parent; }
};
} // namespace tinylang