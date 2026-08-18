# tinylang diagnostic tests

Negative test files that trigger the diagnostics implemented in
`lib/Sema/Sema.cpp`, `lib/Parser/Parser.cpp` and `lib/Lexer/Lexer.cpp`.
Each file starts with a comment listing the expected diagnostic(s).

`Gcd.mod` is the positive baseline (copied from `example/Gcd.mod`):
it must produce no diagnostics at all.

Run a single test:

```sh
build/tools/driver/tinylang test/DiagModuleNameMismatch.mod
```

Run all negative tests and the baseline:

```sh
for f in test/*.mod; do
  echo "== $f"
  build/tools/driver/tinylang "$f"
done
```

## Known issues

- `err_hex_digit_in_decimal` in an expression context crashes the compiler:
  after the lexer reports the bad literal, `Sema::actOnIntegerLiteral` still
  constructs an `llvm::APInt` from it, which aborts on the invalid digit
  (LLVM assertion in `APInt.cpp:fromString`). Putting the literal in a type
  position crashes differently: the parser recovers with a null declaration
  and `Sema::actOnVariableDeclaration` asserts in `dyn_cast`.
  `DiagLexerHexDigit.mod` avoids both crashes by placing the literal right
  after `MODULE`, where it is only lexed, never parsed as an expression.
- The VAR parameter check in `Sema::checkFormalAndActualParameters` is
  inverted: it reports `err_var_parameter_requires_var` when the argument
  *is* a variable, instead of when it is not. `DiagVarParamRequiresVar.mod`
  documents the current (buggy) behavior.
- The driver always exits with status 0, even when errors are reported.
- The `WHILE` diagnostic text says "expression of IF statement" (copy-paste
  in `Diagnostic.def`); the diagnostic kind is still correct.
