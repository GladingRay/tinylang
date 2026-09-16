# tinylang

<p align="center"><a href="README_zh.md">中文版</a></p>

A **Modula-2 subset** compiler built on **LLVM**, for learning compiler construction and LLVM development.

This project follows the tinylang example from the *Learn LLVM* book series, building a complete compiler from scratch on top of LLVM infrastructure (`llvm::SourceMgr`, `llvm::StringMap`, `llvm::APSInt`, `llvm::IRBuilder`, etc.). The full pipeline is implemented: lexing → parsing → semantic analysis → LLVM IR generation → target assembly.

## Language Features

tinylang implements a practical subset of Modula-2:

- **Modular structure**: `MODULE name` … `END name.`
- **Declarations**
  - `CONST` constants
  - `VAR` variables (module-level globals and locals)
  - `TYPE` type aliases (`TYPE MyInt = INTEGER;`), static arrays (`ARRAY [low..high] OF T`, including multi-dimensional arrays), and records (`RECORD ... END`)
  - Extended records (`Circle = RECORD (Shape) ... END`) with inherited fields, and method declarations inside the record body
  - `PROCEDURE` procedures/functions with formal parameters, `VAR` reference parameters, and return types (including arrays and records); type-bound procedures (`PROCEDURE (c: Circle) Area(): INTEGER`) with overriding
- **Statements**: assignment `:=`, procedure calls, `IF`/`THEN`/`ELSE`/`END`, `WHILE`/`DO`/`END`, `RETURN`
- **Expressions**: arithmetic `+ - * / DIV MOD`, relational `= # < <= > >=`, dynamic type test `IS`, logical `AND OR NOT`
- **Literals**: decimal integers `123`, hexadecimal integers `123H`, real literals with a decimal point and optional `E` exponent (`12.3`, `4.567E8`, `1.0E-3`); string/char literals are recognized by the lexer
- **Comments**: nested block comments `(* ... (* ... *) ... *)`
- **Built-in types**: `INTEGER`, `REAL`, `BOOLEAN`

Record fields can be nested, combined with array indexing (`a[i].f`, `r.f[k]`), and records support whole-value assignment. Type aliases are transparently resolved to their underlying type.

Records also form an Oberon-2 style object model, the direction Modula-2's successor took: `RECORD (Base)` extends a record and inherits its fields, and a record body may declare its methods (`PROCEDURE Area(): INTEGER;`) between the fields. Every declaration is implemented outside the record with a receiver, `PROCEDURE (c: Circle) Area(): INTEGER`, and a derived type may override a method with the same name and signature. Every record taking part in an extension carries a type descriptor pointer, so method calls on a designator dispatch dynamically, a `VAR` parameter of a base record type accepts any extension of it, and `v IS T` tests the dynamic type. Value assignment still requires identical types, and a declared method without an implementation is an error.

`REAL` follows Modula-2 semantics: it is a 32-bit IEEE single-precision floating-point type of its own family, `/` performs real division, and `+ - *` work on `REAL` operands (`DIV`, `MOD` and the logical operators stay with `INTEGER`/`BOOLEAN`). As in (ISO) Modula-2, `REAL` and `INTEGER` are neither assignment- nor expression-compatible, so mixing them is a compile-time error until explicit conversion functions such as `FLOAT`/`TRUNC` are implemented.

### Example

```modula2
MODULE Gcd;

VAR x: INTEGER;

PROCEDURE GCD(a, b: INTEGER) : INTEGER;
VAR t: INTEGER;
BEGIN
  IF b = 0 THEN
    RETURN a;
  END;
  WHILE b # 0 DO
    t := a MOD b;
    a := b;
    b := t;
  END;
  RETURN a;
END GCD;

END Gcd.
```

More examples live in `example/`: `Gcd.mod`, `Fib.mod`, `Arrays.mod`, `TypeAlias.mod`, `Record.mod`, `Real.mod`, `Shapes.mod`, `ReturnRecord.mod`, `ReturnArray.mod`, and `VarParam.mod`, each with a matching `call*.c` harness.

## Current Status

| Component | Status | Notes |
| --- | --- | --- |
| Basic (TokenKinds / Diagnostics) | ✅ Done | token, punctuator, keyword, and diagnostic definitions |
| Lexer | ✅ Done | identifiers, numbers, strings, nested comments |
| AST | ✅ Done | `Decl` / `Expr` / `Stmt` class hierarchy, type declarations, field access |
| Parser | ✅ Done | recursive-descent parser (`lib/Parser`) |
| Sema | ✅ Done | scopes/symbol table, type checking, diagnostics (`lib/Sema`) |
| Driver | ✅ Done | `tinylang` executable; parse, diagnose, emit IR/assembly |
| ASTDumper | ✅ Done | `tinylang-astdump` tool for printing the AST |
| Build system | ✅ Done | CMake + `find_package(LLVM)`, C++17, per-module libraries |
| Testing | ✅ Done | diagnostic regression tests under `test/` (42 `.mod` cases) plus end-to-end examples |
| Code generation | ✅ Done | functions, control flow, globals, arrays, records, type aliases, `REAL` (32-bit float) arithmetic, aggregate return values, type extension with virtual method dispatch |

Known limitations: `IMPORT` is not implemented yet and module body statements are parsed but not yet emitted.

## Project Structure

```
tinylang/
├── include/tinylang/
│   ├── Basic/    # TokenKinds.*, Diagnostic.*
│   ├── Lexer/    # Token.h, Lexer.h
│   ├── AST/      # AST.h (declarations, expressions, statements)
│   ├── Parser/   # Parser.h (recursive descent)
│   ├── Sema/     # Scope.h (symbol tables), Sema.h (semantic actions)
│   ├── CodeGen/  # CGModule, CGProcedure, CodeGenerator
│   └── ASTDumper/# ASTDumper.h
├── lib/          # Module implementations
│   ├── Basic/    # TokenKinds.cpp, Diagnostic.cpp, Version.cpp
│   ├── Lexer/    # Lexer.cpp
│   ├── Parser/   # Parser.cpp
│   ├── Sema/     # Scope.cpp, Sema.cpp
│   ├── CodeGen/  # CGModule.cpp, CGProcedure.cpp, CodeGenerator.cpp
│   └── ASTDumper/# ASTDumper.cpp
├── tools/driver/
│   ├── Driver.cpp          # the `tinylang` executable
│   └── astdump/ASTDump.cpp # the `tinylang-astdump` executable
├── test/         # Diagnostic regression tests
├── example/      # Sample Modula-2 programs and C harnesses
├── docs/         # Repository Guidelines and the design/implementation notes
└── cmake/        # CMake helper modules
```

## Building & Using

Prerequisites: CMake ≥ 3.20, a C++17 compiler, and an LLVM installation (headers + libraries).

Configure and build, pointing `LLVM_DIR` at your LLVM CMake directory:

```sh
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

Check a source file:

```sh
build/tools/driver/tinylang example/Gcd.mod
```

Emit LLVM IR or target assembly:

```sh
build/tools/driver/tinylang example/Gcd.mod -o /tmp/Gcd.ll --emit-llvm
build/tools/driver/tinylang example/Gcd.mod -o /tmp/Gcd.s
```

Print the AST:

```sh
build/tools/driver/astdump/tinylang-astdump example/Gcd.mod
```

## Testing

`test/` contains a positive baseline (`Gcd.mod`) and negative cases covering individual diagnostics. Run all diagnostic tests:

```sh
for f in test/*.mod; do
  echo "== $f"
  build/tools/driver/tinylang "$f"
done
```

End-to-end example (record types):

```sh
build/tools/driver/tinylang example/Record.mod -o /tmp/Record.s
cc -c /tmp/Record.s -o /tmp/Record.o
cc -c example/callrecord.c -o /tmp/callrecord.o
cc /tmp/Record.o /tmp/callrecord.o -o /tmp/callrecord
/tmp/callrecord
```

The same pattern is used by the `call*.c` harnesses for `Gcd`, `Fib`, `Arrays`, `TypeAlias`, and `Real`.

## Design Notes

For a chapter-by-chapter walkthrough of the pipeline (lexer → parser → Sema → code generation, including the SSA construction and the record inheritance model), see [`docs/design.md`](docs/design.md).

- **Table-driven `.def` files**: `TokenKinds.def` and `Diagnostic.def` are included multiple times with different macros to generate enums, name tables, spelling tables, and diagnostic tables.
- **Diagnostics engine**: `DiagnosticsEngine` wraps `llvm::SourceMgr`, tracks source locations (`SMLoc`) and an error count, and supports formatted messages.
- **Lexer**: keywords are matched with an `llvm::StringMap`; numbers support decimal and `H`-suffixed hexadecimal integers plus real literals (`digits.digits[E[+|-]digits]`; the `D` scale factor of `LONGREAL` is diagnosed as unimplemented); comments nest.
- **AST**: LLVM-style polymorphic class hierarchy with `isa`/`cast`-style downcasting via `classof()`.
- **Semantic analysis**: `Scope` implements the symbol table; `EnterDeclScope` uses RAII; `Sema` is decoupled from the parser through `actOn*` callbacks.
- **Code generation**: `CGModule`/`CGProcedure` use `llvm::IRBuilder`. Scalar locals use SSA with phi construction; aggregates (arrays/records) are kept in memory and accessed with GEP; record/array assignment lowers to `memcpy`; aggregate return values are spilled through a temporary alloca; symbols are mangled with `_t<len><name>`. A VAR parameter is passed as a `ptr` to the argument, so a scalar local or value parameter that is used as a VAR argument is moved into a stack slot on demand (the value computed so far is stored there, and later accesses load from it).
- **Object model**: an extended record stores the base fields directly after a hidden type descriptor pointer, so a derived record is prefix-compatible with its base. Methods are declared in the record body and their prototypes receive an implicit receiver; the outside definition with a receiver of the same type supplies the body and replaces the prototype in the method table. `CGModule` emits one descriptor per type listing its method implementations (inherited ones included); a method call loads the implementation from the receiver's descriptor, which is what makes the dispatch dynamic.
- **Debug IR dump**: configure with `-DTINYLANG_ENABLE_IR_DUMP=ON` to write `tinylang-dump-<N>.ll` snapshots around phi-node creation/updates.
- **Robust error handling**: parser error recovery is guarded in Sema; invalid decimal literals reported by the lexer are treated as `0` instead of aborting.

## Roadmap

- [x] Lexer, parser, semantic analysis, driver, CMake build
- [x] Diagnostic regression tests
- [x] Code generation: AST → LLVM IR → target assembly
- [x] Static arrays and records
- [x] Type aliases
- [x] `REAL` (floating point) type
- [x] Record inheritance, type-bound procedures, `IS` type tests
- [x] AST dump tool
- [ ] Module imports (`IMPORT`) and module body statements
- [ ] Variant records, `WITH`, `SET`, pointer types, type guards (`v(T)`)
- [x] Array/record return values
- [ ] More optimization passes

## References

- *Learn LLVM 17* — Kai Nacke; the origin of the tinylang example
- [LLVM Documentation](https://llvm.org/docs/)
- [Modula-2 (Wikipedia)](https://en.wikipedia.org/wiki/Modula-2)

## License

[MIT](LICENSE)
