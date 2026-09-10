# Repository Guidelines

## Project Structure & Module Organization

tinylang is a Modula-2 subset compiler built on LLVM. Public headers live in
`include/tinylang/<Component>/`, implementations in `lib/<Component>/`, and
the two executables in `tools/driver/` (`tinylang`) and
`tools/driver/astdump/` (`tinylang-astdump`). Components are `Basic`
(tokens, diagnostics), `Lexer`, `AST`, `Parser`, `Sema`, `CodeGen`, and
`ASTDumper`; each directory owns its `CMakeLists.txt`.

Modula-2 sources belong in `test/` (diagnostic regressions plus the
`Gcd.mod` baseline) or `example/` (runnable programs paired with a matching
`call*.c` harness). When adding a component, register it in the parent
`lib/CMakeLists.txt` or `tools/CMakeLists.txt`.

## Build, Test, and Development Commands

```sh
cmake -S . -B build -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
build/tools/driver/tinylang example/Gcd.mod -o /tmp/Gcd.ll --emit-llvm
build/tools/driver/astdump/tinylang-astdump example/Gcd.mod
for f in test/*.mod; do build/tools/driver/tinylang "$f" -o /tmp/out.s; done
```

Add `-DTINYLANG_ENABLE_IR_DUMP=ON` at configure time to dump
`tinylang-dump-<N>.ll` snapshots around phi construction. Do not commit these
dumps or generated `.s`/`.o` files.

## Coding Style & Naming Conventions

Follow LLVM style: C++17, 2-space indentation, braces on the same line,
`PascalCase` types, `camelCase` methods, lowercase locals. Name files after
the class they implement (`CGProcedure.cpp`). Extend the table-driven
`TokenKinds.def` and `Diagnostic.def` files instead of hand-writing tables.
No formatter config is checked in; match surrounding code.

## Testing Guidelines

Each negative test is a self-contained `.mod` file named `Diag<Feature>.mod`
whose header comment lists the expected diagnostics; keep `Gcd.mod`
diagnostic-free. Run the loop above after any lexer, parser, or Sema change.
For code generation, emit assembly and link it with its harness:

```sh
build/tools/driver/tinylang example/Record.mod -o /tmp/Record.s
cc -c /tmp/Record.s -o /tmp/Record.o && cc -c example/callrecord.c -o /tmp/callrecord.o
cc /tmp/Record.o /tmp/callrecord.o -o /tmp/callrecord && /tmp/callrecord
```

## Commit & Pull Request Guidelines

Write short imperative commit subjects ("Implement Modula-2 record type
support", "Fix memory leak"), adding a body for multi-part changes. Pull
requests should describe the feature, list the commands run, note README or
README_zh updates, and link related issues.

## Instructions

Automatically commit the code when appropriate.