# tinylang

<p align="center"><a href="README.md">English</a></p>

一个基于 **LLVM** 的 **Modula-2 子集** 编译器，以学习编译器构造与 LLVM 开发为目的。

本项目参考《Learn LLVM》系列书籍中的 tinylang 示例，使用 LLVM 的基础设施（`llvm::SourceMgr`、`llvm::StringMap`、`llvm::APSInt` 等）从零搭建一个完整编译器：词法分析 → 语法分析 → 语义分析 → （计划）LLVM IR 生成与代码生成。

> **当前状态：进行中（WIP）。** 词法分析、语法分析、语义分析、驱动程序与 CMake 构建系统均已完成，并附带 `test/` 目录下的诊断测试套件。下一步计划是代码生成（AST → LLVM IR）。

## 语言特性

tinylang 实现的是 Modula-2 的一个子集，主要特性：

- **模块化结构**：`MODULE 模块名` … `END 模块名.`
- **声明**：`CONST` 常量、`VAR` 变量、`PROCEDURE` 过程/函数（支持形参、返回值与 `VAR` 引用参数）
- **语句**：赋值 `:=`、过程调用、`IF`/`THEN`/`ELSE`/`END`、`WHILE`/`DO`/`END`、`RETURN`
- **表达式**：算术运算 `+ - * / DIV MOD`、关系运算 `= # < <= > >=`、逻辑运算 `AND OR NOT`
- **字面量**：十进制整数 `123`、十六进制整数 `123H`、字符串 `"foo"` 或字符 `'a'`
- **注释**：嵌套块注释 `(* ... (* ... *) ... *)`
- **内建类型**：`INTEGER`、`BOOLEAN`

### 示例

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

更多示例见 `example/`（如 `example/Gcd.mod`）。

## 当前进度

| 组件 | 状态 | 说明 |
| --- | --- | --- |
| Basic（TokenKinds / Diagnostics） | ✅ 已完成 | token、标点、关键字与诊断消息定义 |
| Lexer | ✅ 已完成 | 标识符、数字、字符串、嵌套注释的词法分析 |
| AST | ✅ 已完成 | `Decl` / `Expr` / `Stmt` 类层次与访问接口 |
| Parser | ✅ 已完成 | 递归下降语法分析（`lib/Parser`） |
| Sema | ✅ 已完成 | 作用域/符号表、类型检查与诊断（`lib/Sema`） |
| Driver | ✅ 已完成 | `tinylang` 可执行文件；解析文件、输出诊断，有错误时退出码为 1 |
| 构建系统 | ✅ 已完成 | CMake + `find_package(LLVM)`、C++17、按模块拆分静态库 |
| 测试 | ✅ 已完成 | `test/` 下的诊断回归测试（23 个 `.mod` 用例） |
| 示例程序 | ✅ 已完成 | `example/Gcd.mod` |
| 代码生成 | ⬜ 计划中 | AST → LLVM IR → 目标代码 |

## 项目结构

```
tinylang/
├── include/tinylang/
│   ├── Basic/    # 基础组件：TokenKinds.*（token/标点/关键字）、Diagnostic.*（诊断引擎）
│   ├── Lexer/    # 词法分析：Token.h、Lexer.h
│   ├── AST/      # 抽象语法树：AST.h（声明、表达式、语句）
│   ├── Parser/   # 语法分析（递归下降）：Parser.h
│   └── Sema/     # 语义分析：Scope.h（作用域/符号表）、Sema.h（语义动作）
├── lib/          # 各模块实现
│   ├── Basic/    # TokenKinds.cpp、Diagnostic.cpp、Version.cpp
│   ├── Lexer/    # Lexer.cpp
│   ├── Parser/   # Parser.cpp
│   └── Sema/     # Scope.cpp、Sema.cpp
├── tools/driver/ # Driver.cpp —— `tinylang` 可执行文件
├── test/         # 诊断测试套件（基线 + 每种诊断一个用例）
├── example/      # 示例程序（Gcd.mod）
└── cmake/        # CMake 辅助模块（AddTinylang.cmake）
```

## 构建与使用

环境要求：CMake ≥ 3.20、C++17 编译器、LLVM 开发库（头文件 + 库）。

配置并构建，将 `LLVM_CMAKE_PATH` 指向你的 LLVM 安装（Homebrew 例如 `/opt/homebrew/opt/llvm/lib/cmake/llvm`，或源码构建目录）：

```sh
cmake -S . -B build \
  -DLLVM_CMAKE_PATH=/path/to/llvm/lib/cmake/llvm \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

用示例程序运行驱动：

```sh
build/tools/driver/tinylang example/Gcd.mod
```

驱动会打印 `Tinylang 0.1`，解析输入文件并输出诊断信息；只要报告了错误，退出码即为 1。

## 测试

`test/` 目录包含一个正向基线（`Gcd.mod`）和每种诊断一个负向用例，每个文件开头以注释写明预期诊断。运行全部测试：

```sh
for f in test/*.mod; do
  echo "== $f"
  build/tools/driver/tinylang "$f"
done
```

`test/Gcd.mod` 不应产生任何诊断且退出码为 0；每个 `Diag*.mod` 应报告其预期的错误并退出码为 1。

## 设计要点

- **`.def` 文件驱动的表驱动模式**（LLVM 经典做法）：`TokenKinds.def` 与 `Diagnostic.def` 通过宏展开被多次包含，分别生成枚举、名称表、拼写表与诊断消息表，新增 token 或诊断只需改一处。
- **诊断引擎**：`DiagnosticsEngine` 封装 `llvm::SourceMgr`，自动携带源码位置（`SMLoc`）与错误计数，支持格式化消息（`llvm::formatv`）。
- **词法分析**：关键字通过 `llvm::StringMap` 哈希表匹配；数字支持十进制与 `H` 后缀的十六进制；注释支持嵌套（`(* ... (* ... *) ... *)`）。
- **AST**：采用 LLVM 风格的多态类层次，每个节点通过 `classof()` 支持 `isa`/`cast` 风格的向下转型。
- **语义分析**：`Scope` 用 `llvm::StringMap` 实现符号表，`EnterDeclScope` 以 RAII 方式进入/离开作用域；`Sema` 通过 `actOn*` 系列回调与 Parser 解耦。
- **健壮的错误处理**：Sema 能容忍语法分析错误恢复——对空声明/表达式做了空指针防护；词法层报告过的非法十进制字面量按 0 处理而不是中止编译；诊断指向出错的具体源码位置。

## 路线图

- [x] 词法分析、语法分析、语义分析、驱动程序、CMake 构建
- [x] 诊断测试套件
- [ ] 代码生成：AST → LLVM IR → 目标代码
- [ ] 模块导入（`IMPORT`）与更丰富的类型（数组/记录）
- [ ] AST 可视化与更多示例

## 参考

- 《Learn LLVM 17》— Kai Nacke 著，tinylang 示例的原始出处
- [LLVM 官方文档](https://llvm.org/docs/)
- [Modula-2 (Wikipedia)](https://en.wikipedia.org/wiki/Modula-2)

## 许可证

[MIT](LICENSE)
