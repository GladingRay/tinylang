# tinylang

<p align="center"><a href="README.md">English</a></p>

一个基于 **LLVM** 的 **Modula-2 子集** 编译器，以学习编译器构造与 LLVM 开发为目的。

本项目参考《Learn LLVM》系列书籍中的 tinylang 示例，使用 LLVM 的基础设施（`llvm::SourceMgr`、`llvm::StringMap`、`llvm::APSInt`、`llvm::IRBuilder` 等）从零搭建完整编译流水线：词法分析 → 语法分析 → 语义分析 → LLVM IR 生成 → 目标汇编。

## 语言特性

tinylang 实现的是 Modula-2 的一个实用子集：

- **模块化结构**：`MODULE 模块名` … `END 模块名.`
- **声明**
  - `CONST` 常量
  - `VAR` 变量（模块级全局变量和过程内局部变量）
  - `TYPE` 类型别名（`TYPE MyInt = INTEGER;`）、静态数组（`ARRAY [下界..上界] OF T`，支持多维数组）与记录（`RECORD ... END`）
  - 扩展记录（`Circle = RECORD (Shape) ... END`），自动继承基记录的字段，并可在 record 体内声明方法
  - `PROCEDURE` 过程/函数，支持形参、`VAR` 引用参数和返回类型（包括数组与记录）；类型绑定过程（`PROCEDURE (c: Circle) Area(): INTEGER`）支持覆盖
- **语句**：赋值 `:=`、过程调用、`IF`/`THEN`/`ELSE`/`END`、`WHILE`/`DO`/`END`、`RETURN`
- **表达式**：算术运算 `+ - * / DIV MOD`、关系运算 `= # < <= > >=`、动态类型测试 `IS`、逻辑运算 `AND OR NOT`
- **字面量**：十进制整数 `123`、十六进制整数 `123H`、带小数点与可选 `E` 指数的实数（`12.3`、`4.567E8`、`1.0E-3`）；字符串/字符字面量目前仅由词法分析器识别
- **注释**：嵌套块注释 `(* ... (* ... *) ... *)`
- **内建类型**：`INTEGER`、`REAL`、`BOOLEAN`

记录字段支持嵌套，可与数组下标组合（`a[i].f`、`r.f[k]`），记录支持整体赋值；类型别名会被透明地解析为底层类型。

记录同时构成了 Oberon-2 风格的对象模型（Modula-2 后继语言的方向）：`RECORD (Base)` 扩展一个记录并继承其字段，方法可以直接声明在 record 体内（`PROCEDURE Area(): INTEGER;`，与字段并列）。每个声明都在 record 外部用接收者实现——`PROCEDURE (c: Circle) Area(): INTEGER`——派生类型可以用同名同签名的方法覆盖基类型的方法。凡是参与扩展的记录都带一个类型描述符指针，因此对 designator 的方法调用是动态分派的，基记录类型的 `VAR` 形参可以接受它的任意派生类型，`v IS T` 可测试动态类型；而值赋值仍然要求类型完全相同，声明了却未实现的方法会报错。

`REAL` 遵循 Modula-2 语义：它是 32 位 IEEE 单精度（single）浮点类型族，`/` 表示实数除法，`+ - *` 可作用于 `REAL` 操作数（`DIV`、`MOD` 与逻辑运算仍只属于 `INTEGER`/`BOOLEAN`）。与（ISO）Modula-2 一致，`REAL` 与 `INTEGER` 之间既不存在赋值兼容也不存在表达式兼容，混用会在编译期报错；在实现 `FLOAT`/`TRUNC` 等显式转换函数之前，需要先转换再参与运算。

### 示例

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

更多示例见 `example/`：`Gcd.mod`、`Fib.mod`、`Arrays.mod`、`TypeAlias.mod`、`Record.mod`、`Real.mod`、`Shapes.mod`、`ReturnRecord.mod`、`ReturnArray.mod`，每个都配有对应的 `call*.c` 验证程序。

## 当前进度

| 组件 | 状态 | 说明 |
| --- | --- | --- |
| Basic（TokenKinds / Diagnostics） | ✅ 已完成 | token、标点、关键字与诊断消息定义 |
| Lexer | ✅ 已完成 | 标识符、数字、字符串、嵌套注释的词法分析 |
| AST | ✅ 已完成 | `Decl` / `Expr` / `Stmt` 类层次、类型声明与字段访问节点 |
| Parser | ✅ 已完成 | 递归下降语法分析（`lib/Parser`） |
| Sema | ✅ 已完成 | 作用域/符号表、类型检查与诊断（`lib/Sema`） |
| Driver | ✅ 已完成 | `tinylang` 可执行文件：解析、诊断、输出 IR/汇编 |
| ASTDumper | ✅ 已完成 | `tinylang-astdump` 工具，打印 AST |
| 构建系统 | ✅ 已完成 | CMake + `find_package(LLVM)`、C++17、按模块拆分静态库 |
| 测试 | ✅ 已完成 | `test/` 下的诊断回归测试（42 个 `.mod` 用例）以及端到端示例 |
| 代码生成 | ✅ 已完成 | 函数、控制流、全局变量、数组、记录、类型别名、`REAL`（32 位 float）算术、聚合类型返回值，以及带虚方法分派的类型扩展 |

已知限制：`IMPORT` 尚未实现；模块体的语句会被解析但暂不生成代码。

## 项目结构

```
tinylang/
├── include/tinylang/
│   ├── Basic/    # TokenKinds.*、Diagnostic.*
│   ├── Lexer/    # Token.h、Lexer.h
│   ├── AST/      # AST.h（声明、表达式、语句）
│   ├── Parser/   # Parser.h（递归下降）
│   ├── Sema/     # Scope.h（符号表）、Sema.h（语义动作）
│   ├── CodeGen/  # CGModule、CGProcedure、CodeGenerator
│   └── ASTDumper/# ASTDumper.h
├── lib/          # 各模块实现
│   ├── Basic/    # TokenKinds.cpp、Diagnostic.cpp、Version.cpp
│   ├── Lexer/    # Lexer.cpp
│   ├── Parser/   # Parser.cpp
│   ├── Sema/     # Scope.cpp、Sema.cpp
│   ├── CodeGen/  # CGModule.cpp、CGProcedure.cpp、CodeGenerator.cpp
│   └── ASTDumper/# ASTDumper.cpp
├── tools/driver/
│   ├── Driver.cpp          # `tinylang` 可执行文件
│   └── astdump/ASTDump.cpp # `tinylang-astdump` 可执行文件
├── test/         # 诊断回归测试
├── example/      # 示例程序与 C 验证程序
└── cmake/        # CMake 辅助模块
```

## 构建与使用

环境要求：CMake ≥ 3.20、C++17 编译器、LLVM 开发库（头文件 + 库）。

配置并构建，将 `LLVM_DIR` 指向 LLVM 的 CMake 目录：

```sh
cmake -S . -B build \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

检查源文件：

```sh
build/tools/driver/tinylang example/Gcd.mod
```

生成 LLVM IR 或目标汇编：

```sh
build/tools/driver/tinylang example/Gcd.mod -o /tmp/Gcd.ll --emit-llvm
build/tools/driver/tinylang example/Gcd.mod -o /tmp/Gcd.s
```

打印 AST：

```sh
build/tools/driver/astdump/tinylang-astdump example/Gcd.mod
```

## 测试

`test/` 目录包含一个正向基线（`Gcd.mod`）和覆盖各种诊断的负向用例。运行全部诊断测试：

```sh
for f in test/*.mod; do
  echo "== $f"
  build/tools/driver/tinylang "$f"
done
```

端到端示例（记录类型）：

```sh
build/tools/driver/tinylang example/Record.mod -o /tmp/Record.s
cc -c /tmp/Record.s -o /tmp/Record.o
cc -c example/callrecord.c -o /tmp/callrecord.o
cc /tmp/Record.o /tmp/callrecord.o -o /tmp/callrecord
/tmp/callrecord
```

`Gcd`、`Fib`、`Arrays`、`TypeAlias`、`Real` 的 `call*.c` 验证程序采用同样的流程。

## 设计要点

- **`.def` 文件驱动的表驱动模式**：`TokenKinds.def` 与 `Diagnostic.def` 通过宏展开被多次包含，分别生成枚举、名称表、拼写表与诊断消息表。
- **诊断引擎**：`DiagnosticsEngine` 封装 `llvm::SourceMgr`，携带源码位置（`SMLoc`）与错误计数，支持格式化消息。
- **词法分析**：关键字通过 `llvm::StringMap` 匹配；数字支持十进制与 `H` 后缀十六进制整数，以及实数（`digits.digits[E[+|-]digits]`；`LONGREAL` 的 `D` 指数会报“未实现”诊断）；注释支持嵌套。
- **AST**：采用 LLVM 风格的多态类层次，通过 `classof()` 支持 `isa`/`cast` 风格的向下转型。
- **语义分析**：`Scope` 用 `llvm::StringMap` 实现符号表，`EnterDeclScope` 以 RAII 方式管理作用域，`Sema` 通过 `actOn*` 回调与 Parser 解耦。
- **代码生成**：`CGModule`/`CGProcedure` 使用 `llvm::IRBuilder`。标量局部变量走 SSA 并构造 phi 节点；数组/记录等聚合类型保存在内存中并用 GEP 访问；record/数组整体赋值生成 `memcpy`；聚合类型返回值通过临时 alloca 保存；符号按 `_t<长度><名字>` 规则混淆。
- **对象模型**：扩展记录在隐藏的类型描述符指针之后顺序存放基记录字段，因此派生记录与基记录前缀兼容。方法声明写在 record 体内，其原型会获得一个隐式接收者；外部同名同类型接收者的实现提供函数体，并在方法表中替换掉原型。`CGModule` 为每个类型生成一张描述符表，记录该类型实际使用的方法实现（含继承而来的方法）；方法调用从接收者对象的描述符里取实现，这正是动态分派的来源。
- **调试 IR dump**：配置时加 `-DTINYLANG_ENABLE_IR_DUMP=ON`，会在 phi 节点创建/更新前后写出 `tinylang-dump-<N>.ll` 快照。
- **健壮的错误处理**：Sema 能容忍语法错误恢复；词法层报告过的非法十进制字面量按 0 处理而不是中止编译。

## 路线图

- [x] 词法分析、语法分析、语义分析、驱动程序、CMake 构建
- [x] 诊断回归测试
- [x] 代码生成：AST → LLVM IR → 目标汇编
- [x] 静态数组与记录
- [x] 类型别名
- [x] `REAL`（浮点）类型
- [x] 记录继承、类型绑定过程与 `IS` 类型测试
- [x] AST dump 工具
- [ ] 模块导入（`IMPORT`）与模块体语句
- [ ] 变体记录、`WITH`、`SET`、指针类型、类型保护（`v(T)`）
- [x] 数组/记录返回值
- [ ] 更多优化

## 参考

- 《Learn LLVM 17》— Kai Nacke 著，tinylang 示例的原始出处
- [LLVM 官方文档](https://llvm.org/docs/)
- [Modula-2 (Wikipedia)](https://en.wikipedia.org/wiki/Modula-2)

## 许可证

[MIT](LICENSE)
