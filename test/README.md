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

- The driver exits with status 1 if at least one error was reported.
- After a syntax error, error recovery may emit additional cascading
  diagnostics; the expected diagnostics are listed at the top of each file.
