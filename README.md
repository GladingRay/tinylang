# tinylang

<p align="center"><a href="README_zh.md">中文版</a></p>

A **Modula-2 subset** compiler built on **LLVM**, for learning compiler construction and LLVM development.

This project follows the tinylang example from the *Learn LLVM* book series, building a complete compiler from scratch on top of LLVM infrastructure (`llvm::SourceMgr`, `llvm::StringMap`, `llvm::APSInt`, etc.): lexing → parsing → semantic analysis → (planned) LLVM IR generation and code generation.

> **Status: Work in progress (WIP).** The lexer, parser, semantic analyzer, driver, and CMake build system are complete, and a diagnostic test suite lives under `test/`. Code generation (AST → LLVM IR) is the next planned step.

## Language Features

tinylang implements a subset of Modula-2:

- **Modular structure**: `MODULE name` … `END name.`
- **Declarations**: `CONST` constants, `VAR` variables, `PROCEDURE` procedures/functions (with formal parameters, return types, and `VAR` reference parameters)
- **Statements**: assignment `:=`, procedure calls, `IF`/`THEN`/`ELSE`/`END`, `WHILE`/`DO`/`END`, `RETURN`
- **Expressions**: arithmetic `+ - * / DIV MOD`, relational `= # < <= > >=`, logical `AND OR NOT`
- **Literals**: decimal integers `123`, hexadecimal integers `123H`, strings `"foo"` or characters `'a'`
- **Comments**: nested block comments `(* ... (* ... *) ... *)`
- **Built-in types**: `INTEGER`, `BOOLEAN`

### Example

```modula2
MODULE Factorial;

CONST
  N = 10;

VAR
  Result : INTEGER;

PROCEDURE Fact(N : INTEGER) : INTEGER;
VAR
  I : INTEGER;
  R : INTEGER;
BEGIN
  R := 1;
  I := 1;
  WHILE I <= N DO
    R := R * I;
    I := I + 1
  END;
  RETURN R
END Fact;

BEGIN
  Result := Fact(N)
END Factorial.
```

More examples live in `example/` (e.g. `example/Gcd.mod`).

## Current Status

| Component | Status | Notes |
| --- | --- | --- |
| Basic (TokenKinds / Diagnostics) | ✅ Done | token, punctuator, keyword, and diagnostic definitions |
| Lexer | ✅ Done | lexing of identifiers, numbers, strings, and nested comments |
| AST | ✅ Done | `Decl` / `Expr` / `Stmt` class hierarchy and accessors |
| Parser | ✅ Done | recursive-descent parser (`lib/Parser`) |
| Sema | ✅ Done | scopes/symbol table, type checking, diagnostics (`lib/Sema`) |
| Driver | ✅ Done | `tinylang` executable; parses files, reports diagnostics, exits 1 on errors |
| Build system | ✅ Done | CMake + `find_package(LLVM)`, C++17, per-module libraries |
| Testing | ✅ Done | diagnostic regression tests under `test/` (23 `.mod` cases) |
| Examples | ✅ Done | `example/Gcd.mod` |
| Code generation | ⬜ Planned | AST → LLVM IR → target code |

## Project Structure

```
tinylang/
├── include/tinylang/
│   ├── Basic/    # TokenKinds.* (tokens/punctuators/keywords), Diagnostic.* (diagnostics engine)
│   ├── Lexer/    # Lexing: Token.h, Lexer.h
│   ├── AST/      # Abstract syntax tree: AST.h (declarations, expressions, statements)
│   ├── Parser/   # Parsing (recursive descent): Parser.h
│   └── Sema/     # Semantic analysis: Scope.h (scopes/symbol table), Sema.h (semantic actions)
├── lib/          # Module implementations
│   ├── Basic/    # TokenKinds.cpp, Diagnostic.cpp, Version.cpp
│   ├── Lexer/    # Lexer.cpp
│   ├── Parser/   # Parser.cpp
│   └── Sema/     # Scope.cpp, Sema.cpp
├── tools/driver/ # Driver.cpp — the `tinylang` executable
├── test/         # Diagnostic test suite (baseline + one case per diagnostic)
├── example/      # Sample Modula-2 programs (Gcd.mod)
└── cmake/        # CMake helper modules (AddTinylang.cmake)
```

## Building & Using

Prerequisites: CMake ≥ 3.20, a C++17 compiler, and an LLVM installation (headers + libraries).

Configure and build, pointing `LLVM_CMAKE_PATH` at your LLVM install (e.g. `/opt/homebrew/opt/llvm/lib/cmake/llvm` for Homebrew, or a source build):

```sh
cmake -S . -B build \
  -DLLVM_CMAKE_PATH=/path/to/llvm/lib/cmake/llvm \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Run the driver on a sample program:

```sh
build/tools/driver/tinylang example/Gcd.mod
```

The driver prints `Tinylang 0.1`, parses the file, and reports any diagnostics. It exits with status 1 if at least one error was reported.

## Testing

`test/` contains a positive baseline (`Gcd.mod`) and one negative case per diagnostic. Each file documents its expected diagnostics in a leading comment. Run all tests:

```sh
for f in test/*.mod; do
  echo "== $f"
  build/tools/driver/tinylang "$f"
done
```

`test/Gcd.mod` must produce no diagnostics and exit 0; every `Diag*.mod` must report its expected error(s) and exit 1.

## Design Notes

- **Table-driven `.def` files** (a classic LLVM pattern): `TokenKinds.def` and `Diagnostic.def` are included multiple times with different macros to generate enums, name tables, spelling tables, and diagnostic message tables — adding a token or a diagnostic requires a change in only one place.
- **Diagnostics engine**: `DiagnosticsEngine` wraps `llvm::SourceMgr`, carrying source locations (`SMLoc`) and an error count, and supports formatted messages (`llvm::formatv`).
- **Lexer**: keywords are matched via an `llvm::StringMap` hash table; numbers support decimal and `H`-suffixed hexadecimal literals; comments support nesting (`(* ... (* ... *) ... *)`).
- **AST**: LLVM-style polymorphic class hierarchy; every node supports `isa`/`cast`-style downcasting via `classof()`.
- **Semantic analysis**: `Scope` implements the symbol table with `llvm::StringMap`; `EnterDeclScope` enters/leaves scopes via RAII; `Sema` decouples from the parser through `actOn*` callbacks.
- **Robust error handling**: Sema tolerates parser error recovery — null declarations/expressions are guarded, invalid decimal literals reported by the lexer are treated as 0 instead of aborting, and diagnostics point at the offending source location.

## Roadmap

- [x] Lexer, parser, semantic analysis, driver, CMake build
- [x] Diagnostic test suite
- [ ] Code generation: AST → LLVM IR → target code
- [ ] Module imports (`IMPORT`) and richer types (arrays/records)
- [ ] AST visualization and more examples

## References

- *Learn LLVM 17* — Kai Nacke; the origin of the tinylang example
- [LLVM Documentation](https://llvm.org/docs/)
- [Modula-2 (Wikipedia)](https://en.wikipedia.org/wiki/Modula-2)

## License

[MIT](LICENSE)
