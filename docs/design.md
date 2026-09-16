# tinylang 设计与实现

本文档面向想要阅读、修改或扩展 tinylang 的开发者，按「编译流水线」的顺序逐层讲解设计与实现细节，并尽量给出源码位置、关键代码片段与真实生成的 LLVM IR。

阅读时建议对照仓库源码，用 `example/*.mod` 里的示例程序配合 `example/llvmir/*.ll` 的 IR 快照一起看：每个设计点几乎都能在 IR 里找到对应的痕迹。

## 目录

- [1. 项目概览](#1-项目概览)
- [2. 构建系统与目录结构](#2-构建系统与目录结构)
- [3. 基础设施层：Token 与诊断](#3-基础设施层token-与诊断)
- [4. 词法分析器](#4-词法分析器)
- [5. 抽象语法树](#5-抽象语法树)
- [6. 语法分析器](#6-语法分析器)
- [7. 语义分析器](#7-语义分析器)
- [8. 代码生成：模块层](#8-代码生成模块层)
- [9. 代码生成：过程层与 SSA 构造](#9-代码生成过程层与-ssa-构造)
- [10. 面向对象模型：继承与多态](#10-面向对象模型继承与多态)
- [11. 运行时约定与 C ABI](#11-运行时约定与-c-abi)
- [12. ASTDumper 工具](#12-astdumper-工具)
- [13. 测试与验证体系](#13-测试与验证体系)
- [14. 设计取舍与实现要点](#14-设计取舍与实现要点)
- [15. 已知限制与路线图](#15-已知限制与路线图)
- [16. 扩展指南](#16-扩展指南)

---

## 1. 项目概览

tinylang 是一个用 C++17 实现的 **Modula-2 子集编译器**，后端直接构建在 LLVM 之上（`llvm::Module` / `llvm::IRBuilder` / `llvm::TargetMachine`），输入 `.mod` 源文件，输出 LLVM IR（`.ll`）或目标汇编（`.s`）。

### 1.1 编译流水线

```text
源文件 .mod
   │
   ├─ Lexer  ──────► Token 流            lib/Lexer
   │                   ▲
   │                   │ 诊断
   ├─ Parser ──────► actOn* 回调         lib/Parser
   │                   │
   │                   ▼
   ├─ Sema  ───────► AST（带类型信息）+ 诊断   lib/Sema
   │                   │
   │                   ▼
   ├─ CodeGen ─────► llvm::Module         lib/CodeGen
   │                   │
   └─ Driver ──────► .ll / .s / .o        tools/driver
```

与教科书式编译器相比，这里有两个显著特征：

1. **Parser 与 Sema 是解耦的**：Parser 不自己造 AST 节点，而是调用 `Sema::actOn*` 回调；Sema 负责建节点、填类型、报错。Parser 只负责语法。
2. **Sema 与 CodeGen 之间只通过 AST 传递信息**：CodeGen 不重新做类型检查，直接读 AST 上已经算好的类型（`Expr::getType()`）、方法表（`RecordTypeDeclaration::getMethods()`）等。

### 1.2 支持的语言子集

| 类别 | 内容 |
| --- | --- |
| 模块 | `MODULE name;` … `END name.` |
| 声明 | `CONST`、`VAR`（全局/局部）、`TYPE`（别名、静态数组、记录、记录扩展）、`PROCEDURE`（含 `VAR` 形参、返回类型、类型绑定过程） |
| 语句 | 赋值 `:=`、过程调用、`IF/THEN/ELSE/END`、`WHILE/DO/END`、`RETURN` |
| 表达式 | `+ - * / DIV MOD`、`= # < <= > >=`、`AND OR NOT`、`IS` 类型测试 |
| 字面量 | 整数 `123` / `123H`、实数 `12.3` / `4.567E8` / `1.0E-3` |
| 内建类型 | `INTEGER`(i64)、`REAL`(32 位 float)、`BOOLEAN`(i1) |
| 面向对象 | `RECORD (Base)` 继承、类型绑定过程、覆盖、动态分派、`IS` |

### 1.3 规模

```text
include + lib + tools ≈ 5.6k 行 C++
test/    42 个诊断回归用例（.mod）
example/ 11 个端到端示例（.mod + call*.c）
example/llvmir/ 11 份与示例对应的 IR 快照
```

---

## 2. 构建系统与目录结构

### 2.1 目录结构

```text
tinylang/
├── CMakeLists.txt              # 顶层构建脚本、LLVM 查找、调试开关
├── cmake/modules/              # AddTinylang 等自定义 CMake 宏
├── include/tinylang/           # 公共头文件（与 lib/ 一一对应）
│   ├── Basic/                  # TokenKinds.*、Diagnostic.*、Version.*
│   ├── Lexer/                  # Token.h、Lexer.h
│   ├── AST/                    # AST.h（全部 AST 节点）
│   ├── Parser/                 # Parser.h
│   ├── Sema/                   # Scope.h、Sema.h
│   ├── CodeGen/                # CGModule.h、CGProcedure.h、CodeGenerator.h
│   └── ASTDumper/              # ASTDumper.h
├── lib/                        # 各组件实现（每个子目录一个静态库）
├── tools/driver/               # tinylang 可执行文件
│   └── astdump/                # tinylang-astdump 可执行文件
├── test/                       # 诊断回归用例 + 基线
├── example/                    # 示例程序、C 验证程序、IR 快照
└── docs/                       # 本文档
```

### 2.2 顶层 CMake

顶层 `CMakeLists.txt` 做三件事：

1. 声明 C++17，并提供调试开关 `TINYLANG_ENABLE_IR_DUMP`：

```cmake
option(TINYLANG_ENABLE_IR_DUMP "Enable LLVM IR dumps around phi creation" OFF)
if(TINYLANG_ENABLE_IR_DUMP)
  add_compile_definitions(TINYLANG_ENABLE_IR_DUMP)
endif()
```

2. 通过 `find_package(LLVM REQUIRED)` 引入 LLVM，并把 `${LLVM_BINARY_DIR}/include`、`${LLVM_INCLUDE_DIR}` 加入头文件搜索路径。
3. 生成 `Version.inc`（来自 `Version.inc.in`），供 `printVersion()` 使用。

### 2.3 库与工具的划分

每个组件是一个独立静态库，依赖方向严格单向：

```text
tinylangBasic ◄── tinylangLexer ◄── tinylangParser ◄── tinylangSema ◄── tinylangCodeGen
                                                              ▲
                                                  tinylangASTDumper
```

| 库 | 源码 | 依赖 |
| --- | --- | --- |
| `tinylangBasic` | `lib/Basic/*` | LLVM Support |
| `tinylangLexer` | `lib/Lexer/Lexer.cpp` | Basic |
| `tinylangParser` | `lib/Parser/Parser.cpp` | Basic、Lexer（通过头文件使用 Sema；最终由可执行文件链接 `tinylangSema`） |
| `tinylangSema` | `lib/Sema/{Sema,Scope}.cpp` | Basic |
| `tinylangCodeGen` | `lib/CodeGen/*` | Sema、LLVM CodeGen/Core/MC… |
| `tinylangASTDumper` | `lib/ASTDumper/ASTDumper.cpp` | Basic |

可执行文件 `tinylang`（`tools/driver/Driver.cpp`）和 `tinylang-astdump`（`tools/driver/astdump/ASTDump.cpp`）分别链接上述库。

### 2.4 构建与使用

```sh
cmake -S . -B build -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

# 检查语法/语义（有错则打印诊断并返回 1）
build/tools/driver/tinylang example/Gcd.mod

# 生成 LLVM IR / 目标汇编
build/tools/driver/tinylang example/Gcd.mod -o /tmp/Gcd.ll --emit-llvm
build/tools/driver/tinylang example/Gcd.mod -o /tmp/Gcd.s

# 打印 AST
build/tools/driver/astdump/tinylang-astdump example/Gcd.mod
```

---

## 3. 基础设施层：Token 与诊断

### 3.1 `.def` 表驱动设计

`TokenKinds.def` 与 `Diagnostic.def` 都是「用宏参数化、被多次 include」的清单文件，这是 LLVM 项目的惯用手法：一份数据，多处展开，避免枚举、字符串表、分发表三者不同步。

`TokenKinds.def` 由三类条目组成：

```c
TOK(identifier)          // 单个 token
PUNCTUATOR(plus, "+")    // 有拼写的标点
KEYWORD(IF, KEYALL)      // 关键字，实际枚举名是 kw_IF
```

同一份文件被这样展开：

| 展开位置 | 宏定义 | 生成内容 |
| --- | --- | --- |
| `TokenKinds.h` | `TOK(ID)→ID` | `tok::TokenKind` 枚举 |
| `lib/Basic/TokenKinds.cpp` | `TOK(ID)→#ID` | `getTokenName` 名字表 |
| `lib/Basic/TokenKinds.cpp` | `PUNCTUATOR(ID,SP)→case ID: return SP` | `getPunctuatorSpelling` |
| `lib/Basic/TokenKinds.cpp` | `KEYWORD(ID,FLAG)→case kw_##ID: return #ID` | `getKeywordSpelling` |
| `lib/Lexer/Lexer.cpp` | `KEYWORD(NAME,FLAGS)→addKeyword(#NAME, tok::kw_##NAME)` | 关键字散列表 |

注意 `KEYWORD` 的第二个参数 `FLAG`（`KEYALL`）目前并未被使用——它是为「不同方言/不同语言模式下的关键字集合」预留的扩展点。

### 3.2 诊断引擎

`Diagnostic.def` 的条目形如：

```c
DIAG(err_undeclared_name, Error, "undeclared name {0}")
DIAG(note_module_identifier_declaration, Note, "module identifier declared here")
```

三列分别是：枚举名、严重级别（`Error`/`Warning`/`Note`）、消息模板（支持 `llvm::formatv` 的占位符）。
级别只是一个宏参数，需要时写 `Warning` 即可（映射到 `llvm::SourceMgr::DK_Warning`，不计入错误数）；当前清单里只用到 `Error` 与 `Note`。

`lib/Basic/Diagnostic.cpp` 用两次展开生成消息表与级别表：

```cpp
const char *DiagnosticText[] = {
#define DIAG(ID, Level, Msg) Msg,
#include "tinylang/Basic/Diagnostic.def"
};
llvm::SourceMgr::DiagKind DiagnosticKind[] = {
#define DIAG(ID, Level, Msg) llvm::SourceMgr::DK_##Level,
#include "tinylang/Basic/Diagnostic.def"
};
```

报告接口是模板化的，用 `formatv` 完成参数替换，并把错误计数记下来：

```cpp
template <typename... Args>
void report(llvm::SMLoc Loc, unsigned DiagID, Args &&... Arguments) {
  std::string Msg = llvm::formatv(getDiagnosticText(DiagID),
                                  std::forward<Args>(Arguments)...).str();
  llvm::SourceMgr::DiagKind Kind = getDiagnosticKind(DiagID);
  SrcMgr.PrintMessage(Loc, Kind, Msg);
  NumErrors += (Kind == llvm::SourceMgr::DK_Error);
}
```

`NumErrors` 是驱动决定「是否继续做代码生成」的唯一依据：只要有一个 error，`main()` 就跳过 CodeGen，只返回退出码 1。

### 3.3 Token 表示

`Token` 只保存「指针 + 长度 + 类别」：

```cpp
class Token {
  const char *Ptr;
  size_t Length;
  tok::TokenKind Kind;
public:
  llvm::StringRef getIdentifier() { return llvm::StringRef(Ptr, Length); }
  llvm::StringRef getLiteralData() { return llvm::StringRef(Ptr, Length); }
  llvm::SMLoc getLocation() const { return llvm::SMLoc::getFromPointer(Ptr); }
};
```

因为直接指向源缓冲区，token 的文本不需要拷贝，`SMLoc` 也能精确定位到出错列；代价是 AST 里保存的 `StringRef`（标识符名、实数原始文本）要求源缓冲区在编译期间一直存活——驱动正是这么做的。

---

## 4. 词法分析器

实现位于 `lib/Lexer/Lexer.cpp`（约 240 行），对外接口只有两个：

```cpp
void Lexer::next(Token &Result);          // 取下一个 token
llvm::StringRef getBuffer() const;        // 取整个缓冲区（调试用）
```

### 4.1 主分派

`next()` 先跳过空白，然后按首字符分派：

```cpp
if (charinfo::isIdentifierHead(*CurPtr))      identifier(Result);
else if (charinfo::isDigit(*CurPtr))          number(Result);
else if (*CurPtr == '"' || *CurPtr == '\'')   string(Result);
else switch (*CurPtr) { ... }                 // 标点、注释、多字符运算符
```

`charinfo` 是一组 `LLVM_READNONE` 的内联判断函数（`isDigit`、`isHexDigit`、`isIdentifierHead/Body`、`isWhitespace` 等），把「字符分类」的细节独立出来，便于移植。

标点里有两处需要向前看两个字符：

```cpp
case ':':  (*CurPtr+1)=='=' ? colonequal : colon
case '.':  (*CurPtr+1)=='.' ? dotdot     : period
case '<':  (*CurPtr+1)=='=' ? lessequal  : less
case '>':  (*CurPtr+1)=='=' ? greaterequal : greater
case '(':  (*CurPtr+1)=='*' ? 进入注释    : l_paren
```

多字符 token 统一通过 `formToken(Result, End, Kind)` 生成，它会设置 `Ptr/Length/Kind` 并把 `CurPtr` 推到 `End`。

### 4.2 标识符与关键字

```cpp
void Lexer::identifier(Token &Result) {
  const char *Start = CurPtr;
  const char *End = CurPtr + 1;
  while (charinfo::isIdentifierBody(*End)) ++End;
  llvm::StringRef Name(Start, End - Start);
  formToken(Result, End, Keywords.getKeyword(Name, tok::identifier));
}
```

`KeywordFilter` 内部是 `llvm::StringMap<tok::TokenKind>`，在构造 `Lexer` 时通过 `addKeywords()` 把 `TokenKinds.def` 里所有 `KEYWORD` 灌进去。查不到就返回 `tok::identifier`。

### 4.3 数字：整数与实数

`number()` 是词法里最复杂的一个，它需要区分：十进制整数、`H` 后缀十六进制整数、实数（含小数点与指数），还要处理数组范围 `1..2` 的歧义。

```cpp
void Lexer::number(Token &Result) {
  const char *End = CurPtr + 1;
  tok::TokenKind Kind = tok::unknown;
  bool IsHex = false;
  while (*End) {                       // 先吃掉所有十六进制数字
    if (!charinfo::isHexDigit(*End)) break;
    if (!charinfo::isDigit(*End)) IsHex = true;
    ++End;
  }
  switch (*End) {
  case 'H':                            // 123H
    Kind = tok::integer_literal; ++End; break;
  case '.':                            // 实数
    if (*(End + 1) == '.') {           // "1..2" 是整数 + 范围运算符
      Kind = tok::integer_literal; break;
    }
    if (IsHex) Diags.report(getLoc(), diag::err_hex_digit_in_decimal);
    ++End;                             // 吃掉 '.'
    while (charinfo::isDigit(*End)) ++End;
    if (*End == 'E' || *End == 'e')
      End = scaleFactor(End, diag::err_invalid_real_literal);
    else if (*End == 'D' || *End == 'd')   // LONGREAL，未实现
      End = scaleFactor(End, diag::err_invalid_real_literal,
                        diag::err_longreal_literal_not_implemented);
    Kind = tok::real_literal; break;
  default:
    if (IsHex) Diags.report(getLoc(), diag::err_hex_digit_in_decimal);
    Kind = tok::integer_literal; break;
  }
  formToken(Result, End, Kind);
}

const char *Lexer::scaleFactor(const char *End, unsigned NoDigitsDiag,
                               unsigned FeatureDiag) {
  const char *ExpEnd = End + 1;                       // 跳过 'E'/'D'
  if (*ExpEnd == '+' || *ExpEnd == '-') ++ExpEnd;
  if (!charinfo::isDigit(*ExpEnd)) {                  // "1.2E+" 这类
    Diags.report(getLoc(), NoDigitsDiag);
    return ExpEnd;
  }
  if (FeatureDiag) Diags.report(getLoc(), FeatureDiag);
  while (charinfo::isDigit(*ExpEnd)) ++ExpEnd;
  return ExpEnd;
}
```

几个值得注意的设计点：

- **`1..2` 的消歧**：Modula-2 里数组范围写作 `ARRAY [1..2] OF`，而实数语法允许 `1.`。实现通过「`.` 后面还是 `.` 就不当小数点」来保证范围语法不被吃掉，`Arrays.mod` / `TypeAlias.mod` 都覆盖了这条路径。
- **十进制里混入十六进制数字**（如 `12A`）会报 `err_hex_digit_in_decimal`；Sema 收到这个字面量时会把它当作 `0`，避免把非法文本喂给 `APInt` 导致断言（见 §7.6）。
- **`D` 指数**（`1.0D0`，LONGREAL）被识别但报「未实现」，不会静默当成 `E` 处理。

### 4.4 字符串与注释

字符串以 `"` 或 `'` 起始，扫描到同种引号或行尾；遇到行尾报 `err_unterminated_char_or_string`。

注释是嵌套块注释（Modula-2 特色），用计数法实现，支持 `(* 外层 (* 内层 *) 外层 *)`：

```cpp
void Lexer::comment() {
  const char *End = CurPtr + 2;
  unsigned Level = 1;
  while (*End && Level) {
    if (*End == '(' && *(End+1) == '*') { End += 2; Level++; }
    else if (*End == '*' && *(End+1) == ')') { End += 2; Level--; }
    else ++End;
  }
  if (!*End) Diags.report(getLoc(), diag::err_unterminated_block_comment);
  CurPtr = End;
}
```

词法层的错误处理策略是「报告 + 尽可能继续」：`next()` 遇到无法识别的字符返回 `tok::unknown`，由 Parser 的恢复机制处理；注释/字符串未终止时仍然把已扫描的部分作为一个 token 交给上层，避免连锁崩溃。

---

## 5. 抽象语法树

全部 AST 节点集中在 `include/tinylang/AST/AST.h`（约 780 行），没有 `.cpp`——所有方法都是内联的。

### 5.1 所有权模型

节点之间用 `std::unique_ptr` 建立树形所有权，容器统一用类型别名：

```cpp
using DeclList       = std::vector<std::unique_ptr<Decl>>;
using FormalParamList = std::vector<std::unique_ptr<FormalParameterDeclaration>>;
using ExprList       = std::vector<std::unique_ptr<Expr>>;
using StmtList       = std::vector<std::unique_ptr<Stmt>>;
```

Parser 生产这些容器（例如 `DeclList Decls`），Sema 通过 `std::move` 接管并挂到对应的父节点上，因此**AST 不需要垃圾回收，也没有显式 delete**。少数需要跨节点共享的类型（内建类型、类型别名指向的目标、record 的方法表）用裸指针表示非拥有引用。

### 5.2 声明节点

`Decl` 是所有声明的基类，用 `DeclKind` 做运行时判别（LLVM 风格的 `classof` + `isa/dyn_cast/cast`）：

```cpp
class Decl {
public:
  enum DeclKind { DK_Module, DK_Const, DK_Type, DK_ArrayType,
                  DK_TypeAlias, DK_RecordType, DK_Var, DK_Param, DK_Proc };
  DeclKind getKind() const;
  SMLoc getLocation();
  StringRef getName();
  Decl *getEnclosingDecl();     // 用于混淆名、判断全局/局部
};
```

| 节点 | 说明 | 关键成员 |
| --- | --- | --- |
| `ModuleDeclaration` | 模块 | `Decls`、`Stmts` |
| `ConstantDeclaration` | `CONST X = expr;` | `Expr *` |
| `TypeDeclaration` | 内建类型/匿名类型基类 | `setName()`（别名命名用） |
| `TypeAliasDeclaration` | `TYPE T = U;` | `Aliased`（非拥有） |
| `ArrayTypeDeclaration` | `ARRAY [low..high] OF T` | `ElementType`、上下界、`getNumElements()` |
| `RecordTypeDeclaration` | `RECORD ... END` | 字段、基类型、方法、类型标签标志 |
| `VariableDeclaration` | `VAR x: T;` | `TypeDeclaration *` |
| `FormalParameterDeclaration` | 形参 | 类型、`isVar()`、`isReceiver()` |
| `ProcedureDeclaration` | 过程/函数/方法 | 形参、返回类型、局部声明、语句 |

`RecordTypeDeclaration` 是承载面向对象特性的核心：

```cpp
class RecordTypeDeclaration : public TypeDeclaration {
  DeclList Fields;          // 字段（VariableDeclaration）
  DeclList MethodDecls;     // record 体内声明的方法原型（拥有）
  TypeDeclaration *BaseType;
  std::vector<ProcedureDeclaration *> Methods;  // 方法槽：实现或原型
  bool HasTypeTag;
public:
  std::vector<VariableDeclaration *> getAllFields();      // 基类字段在前，平铺
  unsigned getFieldIndex(VariableDeclaration *F);         // 含隐藏标签的偏移
  VariableDeclaration *lookupField(StringRef Name);       // 沿基类链查找
  void addMethod/replaceMethod(...);
  bool hasTypeTag() / setHasTypeTag();
};
```

`ProcedureDeclaration` 上有三个与方法相关的状态：

```cpp
bool IsMethodDeclaration;        // 是否是 record 体内声明的「原型」
ProcedureDeclaration *Definition;// 原型的实现
FormalParameterDeclaration *getReceiver();  // 首参带 isReceiver() 标记
bool isMethod() { return getReceiver() != nullptr; }
```

类型关系的辅助函数也都内联在 `AST.h`，供 Sema 与 CodeGen 共用：

| 函数 | 作用 |
| --- | --- |
| `getUnderlyingType(T)` | 剥掉类型别名链，得到真正的类型 |
| `isExtensionOf(Ty, Base)` | 沿 `BaseType` 链判断扩展关系（Oberon-2 语义） |
| `collectMethodSlots(T)` | 展平虚表：基类槽位在前，同名覆盖原位替换，新方法追加 |
| `lookupMethod(T, name)` | 在方法表中按名字查找 |
| `getMethodSlot(T, name)` | 计算方法在虚表中的下标 |

### 5.3 表达式节点

```cpp
enum ExprKind { EK_Infix, EK_Prefix, EK_Int, EK_Bool, EK_Real, EK_Var,
                EK_Const, EK_Func, EK_MethodCall, EK_TypeTest,
                EK_Indexed, EK_Field };
```

`Expr` 基类保存三样东西：种类、**类型**、是否是编译期常量：

```cpp
class Expr {
  ExprKind Kind;
  TypeDeclaration *Ty;   // Sema 填充，CodeGen 直接使用
  bool IsConst;
};
```

| 节点 | 对应语法 | 备注 |
| --- | --- | --- |
| `InfixExpression` | `a + b`、`a < b`、`a AND b` | 保存 `OperatorInfo`（位置 + token 种类） |
| `PrefixExpression` | `-x`、`NOT b` | |
| `IntegerLiteral` | `123` / `123H` | 值用 `llvm::APSInt` |
| `RealLiteral` | `12.3`、`4.567E8` | 原始文本 `StringRef` + `llvm::APFloat`（IEEEsingle） |
| `BooleanLiteral` | `TRUE` / `FALSE` | |
| `VariableAccess` | 变量/形参引用 | 持有 `Decl *` |
| `ConstantAccess` | 常量引用 | 类型取自常量表达式 |
| `FunctionCallExpr` | `F(a, b)` | 持有 `ProcedureDeclaration *` |
| `MethodCallExpr` | `c.Area()` | 持有接收者表达式 + 静态解析到的方法 |
| `TypeTestExpr` | `s IS Circle` | 持有被测试类型 |
| `IndexedExpression` | `a[i]` | 基址 + 下标 |
| `FieldAccess` | `r.f` | 基址 + `VariableDeclaration *` |

两个 OO 相关节点值得单独说明：

```cpp
class MethodCallExpr : public Expr {
  std::unique_ptr<Expr> Receiver;   // 必须是 designator（有地址）
  ProcedureDeclaration *Method;     // 静态类型上找到的方法
  ExprList Params;
};
class TypeTestExpr : public Expr {
  std::unique_ptr<Expr> E;
  TypeDeclaration *TestedType;
};
```

### 5.4 语句节点

```cpp
enum StmtKind { SK_Assign, SK_ProcCall, SK_MethodCall, SK_If, SK_While, SK_Return };
```

| 节点 | 成员 |
| --- | --- |
| `AssignmentStatement` | 目标（designator 表达式）+ 值表达式 |
| `ProcedureCallStatement` | 过程 + 实参列表 |
| `MethodCallStatement` | 包着一个 `MethodCallExpr`（方法作为语句调用） |
| `IfStatement` | 条件 + then 语句表 + else 语句表 |
| `WhileStatement` | 条件 + 循环体 |
| `ReturnStatement` | 可选返回值 |

---

## 6. 语法分析器

`lib/Parser/Parser.cpp`（约 900 行）是手写递归下降解析器，只有一个 token 的前瞻（`Token Tok`）。

### 6.1 基本约定

```cpp
class Parser {
  Lexer &Lex;
  Sema &Actions;
  Token Tok;

  void advance() { Lex.next(Tok); }
  bool expect(tok::TokenKind T);    // 不匹配则报 err_expected，返回 true 表示出错
  bool consume(tok::TokenKind T);   // 匹配则前进并返回 false
  template <typename... Ts> bool skipUntil(Ts &&...Toks);
};
```

所有解析函数都返回 `bool`，**true 表示失败**。每个函数开头都定义一个错误恢复 lambda：

```cpp
bool Parser::parseIfStatement(StmtList &Stmts) {
  auto _errorhandler = [this] { return skipUntil(tok::kw_ELSE, tok::kw_END); };
  ...
  if (expect(tok::kw_THEN))
    return _errorhandler();      // 跳到同步点，继续解析后续语句
  ...
}
```

同步点选择的是「上层结构的分隔符」：语句级用 `;`/`ELSE`/`END`，声明级用 `;` 或 `BEGIN`/`END`，表达式级用运算符与右括号。这样可以在一处出错后继续解析整个文件，一次报告多个错误。

### 6.2 文法概览

```text
compilation_unit = "MODULE" ident ";" {import} block ident "." .
block            = {CONST|TYPE|VAR|PROCEDURE 声明} ["BEGIN" statement_sequence] "END" .

statement        = [designator ":=" expression]
                 | [ident "(" actuals ")"]          (* 过程调用 *)
                 | if_statement | while_statement | return_statement .
if_statement     = "IF" expression "THEN" statement_sequence
                   ["ELSE" statement_sequence] "END" .
while_statement  = "WHILE" expression "DO" statement_sequence "END" .
return_statement = "RETURN" [expression] .

expression       = simple_expression [ relation simple_expression | "IS" qualident ] .
simple_expression= ["+"|"-"] term { ("+"|"-"|"OR") term } .
term             = factor { ("*"|"/"|"DIV"|"MOD"|"AND") factor } .
factor           = integer | real | ident designator | "(" expression ")" | "NOT" factor .
designator       = { "[" expression "]" | "." ident [ "(" actuals ")" ] } .

type             = qualident | array_type | record_type .
array_type       = "ARRAY" ("[" expression ".." expression "]") {"," ...} "OF" type .
record_type      = "RECORD" ["(" qualident ")"] { field | method_decl } "END" .
field            = ident_list ":" type ";" .
method_decl      = "PROCEDURE" ident [formal_parameters] [":" qualident] ";" .
procedure_decl   = "PROCEDURE" ["(" ident ":" qualident ")"] ident
                   [formal_parameters] [":" qualident] ";" block ident ";" .
```

### 6.3 主要函数一览

| 函数 | 行 | 职责 |
| --- | --- | --- |
| `parse()` / `parseCompilationUnit` | 16 / 22 | 模块名、`IMPORT`、块、结尾模块名与 `.` |
| `parseBlock` | 78 | 声明序列 + 可选 `BEGIN` 语句序列 + `END` |
| `parseDeclaration` | 95 | 分派 CONST/VAR/TYPE/PROCEDURE |
| `parseConstantDeclaration` | 136 | `CONST X = expr` |
| `parseTypeDeclaration` | 154 | `TYPE T = type` |
| `parseVariableDeclaration` | 172 | `VAR a, b: T` |
| `parseType` | 186 | 别名 / 数组 / 记录 |
| `parseRecordType` / `parseRecordBody` / `parseMethodDeclaration` | 240 / 265 / 292 | 记录（含扩展、方法声明） |
| `parseProcedureDeclaration` | 333 | 过程/函数/类型绑定过程（接收者） |
| `parseFormalParameters` 系列 | 395+ | 形参列表与 `VAR` 修饰 |
| `parseStatement*` / `parseIf*` / `parseWhile*` / `parseReturn*` | 446+ | 语句 |
| `parseExpression` → `parseSimpleExpression` → `parseTerm` → `parseFactor` | 599 / 656 / 711 / 760 | 表达式优先级链 |
| `parseQualident` | 818 | 限定名（`M.x`） |
| `parseDesignator` | 842 | 下标、字段、方法调用 |

### 6.4 表达式：优先级与 designator

优先级由函数层次表达（低 → 高）：**关系/`IS` → 加减/`OR` → 乘除/`DIV`/`MOD`/`AND` → 一元 `NOT`/`+`/`-` → 基本因子**。

```cpp
bool Parser::parseExpression(std::unique_ptr<Expr> &E) {
  if (parseSimpleExpression(E)) return _errorhandler();
  if (Tok.isOneOf(tok::hash, tok::less, ..., tok::greaterequal)) {
    OperatorInfo Op; std::unique_ptr<Expr> Right;
    if (parseRelation(Op)) return _errorhandler();
    if (parseSimpleExpression(Right)) return _errorhandler();
    E = Actions.actOnExpression(std::move(E), std::move(Right), Op);
  } else if (Tok.is(tok::kw_IS)) {        // 类型测试
    SMLoc Loc = Tok.getLocation();
    advance();
    Decl *D = nullptr;
    if (parseQualident(D)) return _errorhandler();
    E = Actions.actOnTypeTest(Loc, std::move(E), D);
  }
  return false;
}
```

`parseFactor` 里 `identifier` 分支同时处理了三件事：普通变量、函数调用、以及随后的 designator（`a[i].f`）：

```cpp
} else if (Tok.is(tok::identifier)) {
  Decl *D; ExprList Exprs; SMLoc Loc = Tok.getLocation();
  if (parseQualident(D)) return _errorhandler();
  if (Tok.is(tok::l_paren)) {                     // F(...)
    advance();
    if (Tok.isOneOf(...表达式起始集...)) { if (parseExpList(Exprs)) ... }
    if (expect(tok::r_paren)) return _errorhandler();
    E = Actions.actOnFunctionCall(Loc, D, std::move(Exprs));
    advance();
    if (parseDesignator(E)) return _errorhandler();
  } else {
    E = Actions.actOnVariable(D);
    if (parseDesignator(E)) return _errorhandler();
  }
}
```

**方法调用与字段访问在语法层就能区分**：`parseDesignator` 读到 `.` 后的标识符时，再看下一个 token 是不是 `(`：

```cpp
advance();                            // 吃掉 '.'
if (expect(tok::identifier)) return true;
SMLoc Loc = Tok.getLocation();
StringRef Name = Tok.getIdentifier();
advance();
if (Tok.is(tok::l_paren)) {           // 方法调用：receiver.Name(args)
  ExprList Exprs;
  ...
  E = Actions.actOnMethodCall(Loc, std::move(E), Name, std::move(Exprs));
} else {                              // 字段访问：receiver.Name
  E = Actions.actOnFieldAccess(Loc, std::move(E), Name);
}
```

### 6.5 记录与类型绑定过程的解析

记录体由 `parseRecordBody` 解析，允许**字段与方法声明混排**：

```cpp
while (true) {
  if (Tok.is(tok::identifier)) {        // field: a, b: T;
    ... Actions.actOnFieldDeclaration(Fields, Ids, D);
    if (!Tok.is(tok::semi)) break;      // 最后一个字段可以省略分号
    advance();
  } else if (Tok.is(tok::kw_PROCEDURE)) {
    if (parseMethodDeclaration(Methods)) return _errorhandler();
  } else {
    break;                              // 交给 expect(kw_END)
  }
}
```

`parseMethodDeclaration` 只接受**声明**（没有函数体），并把参数解析放在一个临时作用域里（参数名对函数体不可见，因为函数体写在别处）：

```cpp
SMLoc Loc = Tok.getLocation();
advance();                                     // PROCEDURE
if (expect(tok::identifier)) return _errorhandler();
auto D = Actions.actOnMethodDeclaration(Tok.getLocation(), Tok.getIdentifier());
EnterDeclScope S(Actions, D.get());            // 只为形参名建一个作用域
...
if (Tok.is(tok::kw_BEGIN)) {                   // 函数体写在 record 里 → 报错
  getDiagnostics().report(Loc, diag::err_method_body_inside_record);
  ... 解析整个块后 return true，让上层跳到 record 的 END
}
Actions.actOnMethodDeclaration(D.get(), std::move(Params), RetType, RetTypeLoc);
```

过程/函数/方法的实现统一由 `parseProcedureDeclaration` 解析，差别只在可选的**接收者**：

```cpp
if (Tok.is(tok::l_paren)) {            // PROCEDURE (r: T) Name(...)
  IsMethod = true;
  advance();
  RecvName = Tok.getIdentifier(); advance();
  consume(tok::colon);
  parseQualident(RecvType);            // 在「外层作用域」解析接收者类型
  consume(tok::r_paren);
}
if (expect(tok::identifier)) ...
auto D = Actions.actOnProcedureDeclaration(Tok.getLocation(),
                                           Tok.getIdentifier(), IsMethod);
EnterDeclScope S(Actions, D.get());
if (IsMethod) Actions.actOnReceiverParameter(D.get(), RecvLoc, RecvName, RecvType, Params);
if (Tok.is(tok::l_paren)) parseFormalParameters(Params, RetType, RetTypeLoc);
Actions.actOnProcedureHeading(D.get(), std::move(Params), RetType, RetTypeLoc);
```

接收者会作为**第一个形参**进入 `Params`，因此后续所有「形参」相关的逻辑（作用域、签名、CodeGen 参数映射）都不需要特殊分支——只有「跳过第 0 个形参」的少数判断。

---

## 7. 语义分析器

`lib/Sema/Sema.cpp`（约 870 行）承担四件事：**建符号表、造 AST 节点、做类型检查、报诊断**。

### 7.1 作用域与符号表

`Scope` 是链式作用域，每层是一个 `llvm::StringMap<Decl *>`：

```cpp
class Scope {
  Scope *Parent;
  llvm::StringMap<Decl *> Symbols;
public:
  bool insert(Decl *Declaration);   // 已存在返回 false
  Decl *lookup(StringRef Name);     // 沿 Parent 向上找
};
```

Parser 用 RAII 包装进入/离开作用域：

```cpp
class EnterDeclScope {
  Sema &Semantics;
public:
  EnterDeclScope(Sema &S, Decl *D) : Semantics(S) { S.enterScope(D); }
  ~EnterDeclScope() { Semantics.leaveScope(); }
};
```

`Sema::enterScope/leaveScope` 同时维护 `CurrentScope` 与 `CurrentDecl`（当前所属声明），后者被用来设置新声明节点的 `EnclosingDecl`，进而用于「判断变量是全局还是局部」和符号混淆。

### 7.2 内建类型与预定义常量

`Sema::initialize()` 在全局作用域里注册三个内建类型与两个布尔常量：

```cpp
IntegerType = make_unique<TypeDeclaration>(CurrentDecl, SMLoc(), "INTEGER");
RealType    = make_unique<TypeDeclaration>(..., "REAL");
BooleanType = make_unique<TypeDeclaration>(..., "BOOLEAN");
TrueConst  = make_unique<ConstantDeclaration>(..., "TRUE",  BooleanLiteral(true,  BooleanType.get()));
FalseConst = make_unique<ConstantDeclaration>(..., "FALSE", BooleanLiteral(false, BooleanType.get()));
```

它们是**单例**：`isSameType` 对它们直接做指针比较。

### 7.3 类型系统与兼容规则

```cpp
bool Sema::isSameType(TypeDeclaration *LHS, TypeDeclaration *RHS) {
  if (!LHS || !RHS) return LHS == RHS;
  LHS = getUnderlyingType(LHS);
  RHS = getUnderlyingType(RHS);
  if (LHS == RHS) return true;                       // 标量/记录：名义等价
  if (auto *LArr = dyn_cast<ArrayTypeDeclaration>(LHS))
    if (auto *RArr = dyn_cast<ArrayTypeDeclaration>(RHS))
      return LArr->getLowBound() == RArr->getLowBound() &&
             LArr->getHighBound() == RArr->getHighBound() &&
             isSameType(LArr->getElementType(), RArr->getElementType());  // 数组：结构等价
  return false;
}
```

- **标量**（INTEGER/REAL/BOOLEAN）与**记录**是名义等价：必须是同一个类型对象。`REAL` 与 `INTEGER` 之间既无隐式赋值兼容也无表达式兼容（ISO Modula-2 规则），混用会报 `err_types_for_operator_not_compatible`。
- **数组**是结构等价：上下界与元素类型一致即可互相赋值。类型别名在比较前被 `getUnderlyingType` 剥掉，因此别名是透明的。
- 运算符支持由 `isOperatorForType` 决定：`+ - *` 支持 INTEGER/REAL；`/` 只支持 REAL（整数除法用 `DIV`）；`DIV MOD` 只支持 INTEGER；`AND OR NOT` 只支持 BOOLEAN。

形参兼容性在 `isSameType` 之上增加了继承规则：

```cpp
bool Sema::isCompatibleWithFormal(TypeDeclaration *Actual,
                                  TypeDeclaration *Formal, bool IsVar) {
  if (isSameType(Actual, Formal)) return true;
  if (!IsVar) return false;                       // 值形参要求同类型
  auto *ActualRec = dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Actual));
  auto *FormalRec = dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Formal));
  return ActualRec && FormalRec && isExtensionOf(ActualRec, FormalRec);
}
```

### 7.4 表达式构造与常量折叠

表达式的 `actOn*` 回调按语法层次对应实现：`actOnExpression`（关系，结果类型 BOOLEAN）、`actOnSimpleExpression`（加减/OR）、`actOnTerm`（乘除/DIV/MOD/AND）、`actOnPrefixExpression`（一元）。

除了类型检查，Sema 还做少量**常量折叠**，让常量表达式在 IR 里直接变成字面量：

| 位置 | 折叠内容 |
| --- | --- |
| `actOnSimpleExpression` | `TRUE OR x` 之类两个布尔字面量的 `OR` |
| `actOnTerm` | 两个布尔字面量的 `AND` |
| `actOnPrefixExpression` | `NOT` 布尔字面量 |
| `evalConstInt`（静态函数） | 整数字面量与一元负号，用于数组上下界求值 |

一元负号的处理遵循 Modula-2 报告：`simple_expression = ["+"|"-"] term { add_op term }`，即**符号只作用于第一个 term**。因此 `-a + b` 等价于 `(-a) + b`，而 `-a * b` 等价于 `-(a * b)`（乘除属于 term 内部）。想改变符号的作用范围就加括号：`(-a) + b`、`-(a + b)`。`Parser::parseSimpleExpression` 先读入符号、解析完第一个 term 后立刻套用，再进入加减循环，正是这个规则的直接实现；这三种写法都有 `example/Expr.mod` 的回归用例覆盖。

### 7.5 语句检查

| 回调 | 检查内容 | 诊断 |
| --- | --- | --- |
| `actOnAssignment` | 目标类型与值类型 `isSameType` | `err_types_for_operator_not_compatible(":=")` |
| `actOnProcCall` | 目标必须是过程（不能是函数/变量）、实参匹配 | `err_procedure_call_on_nonprocedure` 等 |
| `actOnIfStatement` / `actOnWhileStatement` | 条件必须是 BOOLEAN | `err_if_expr_must_be_bool` / `err_while_expr_must_be_bool` |
| `actOnReturnStatement` | 函数必须有值、过程不能有值、类型匹配 | `err_function_requires_return` / `err_procedure_requires_empty_return` / `err_function_and_return_type` |
| `checkFormalAndActualParameters` | 实参个数、类型兼容、VAR 实参必须是 designator | `err_wrong_number_of_parameters` 等 |

`actOnAssignment` 的目标允许三种 designator：变量、数组元素、记录字段，与 CodeGen 的 `emitLValue` 支持的范围一致。

### 7.6 字面量处理

```cpp
std::unique_ptr<Expr> Sema::actOnIntegerLiteral(SMLoc Loc, StringRef Literal) {
  uint8_t Radix = 10;
  if (Literal.ends_with("H")) { Literal = Literal.drop_back(); Radix = 16; }
  // 词法已报错的情况下不要把非法文本喂给 APInt，否则会断言
  if (Radix == 10 &&
      !llvm::all_of(Literal, [](char C) { return C >= '0' && C <= '9'; }))
    Literal = "0";
  llvm::APInt Value(64, Literal, Radix);
  return std::make_unique<IntegerLiteral>(Loc, llvm::APSInt(Value, false),
                                          IntegerType.get());
}
```

实数则直接按单精度解析，避免「先转 double 再缩窄」的双重舍入：

```cpp
llvm::APFloat Value(llvm::APFloat::IEEEsingle());
auto ParseResult = Value.convertFromString(Literal, llvm::APFloat::rmNearestTiesToEven);
if (!ParseResult) { llvm::consumeError(ParseResult.takeError());
                    Value = llvm::APFloat(0.0f); }
return std::make_unique<RealLiteral>(Loc, Literal, Value, RealType.get());
```

这也是「词法报错、语义继续」的典型配合：`1.2E+`、`1.0D0` 这类字面量在词法层已经报错，Sema 只保证不崩溃，不重复报错。

### 7.7 记录、继承与方法的语义检查

`actOnRecordType` 负责建类型并做继承相关检查：

```cpp
if (BaseType) {
  Base = dyn_cast<TypeDeclaration>(BaseType);
  if (!Base || !isa<RecordTypeDeclaration>(getUnderlyingType(Base))) {
    Diags.report(Loc, diag::err_extended_record_base_must_be_record);
    Base = nullptr;
  }
}
if (BaseRec) { BaseRec->setHasTypeTag(); RecTy->setHasTypeTag(); }
for (const auto &F : RecTy->getFields())
  if (BaseRec && BaseRec->lookupField(F->getName()))
    Diags.report(F->getLocation(), diag::err_field_conflicts_with_base, F->getName());
```

随后给 record 体内声明的方法补上**隐式接收者**并登记到方法表（同时做重复声明与签名一致性检查）：

```cpp
for (const auto &MD : RecTy->getMethodDecls()) {
  auto *Method = cast<ProcedureDeclaration>(MD.get());
  Method->setReceiver(make_unique<FormalParameterDeclaration>(
      Method, Method->getLocation(), "self", RecTy.get(),
      /*IsVar=*/true, /*IsReceiver=*/true));
  if (已有同名方法) → err_duplicate_method
  else if (基类有同名方法 && !hasSameSignature(Inherited, Method))
       → err_method_signature_mismatch
  else { RecTy->addMethod(Method); DeclaredMethods.push_back(Method); }
}
```

外部的实现（`PROCEDURE (c: Circle) Area(...)`）在 `actOnProcedureHeading` 里与本类型的原型配对：

```cpp
for (auto *Own : RecTy->getMethods()) {
  if (Own->getName() != ProcDecl->getName()) continue;
  if (!Own->isMethodDeclaration())  → err_duplicate_method;
  if (!hasSameSignature(Own, ProcDecl)) → err_method_signature_mismatch;
  Own->setDefinition(ProcDecl);          // 记住实现
  RecTy->replaceMethod(Own, ProcDecl);   // 虚表槽位换成带函数体的实现
  return;
}
// 没有对应声明 → 按“新方法/覆盖基类方法”处理
if (lookupMethod(RecTy, name) 且签名不同) → err_method_signature_mismatch;
RecTy->addMethod(ProcDecl);
```

模块结束时（`actOnModuleDeclaration` 的收尾）统一检查「声明了但没实现」：

```cpp
for (auto *Method : DeclaredMethods)
  if (!Method->getDefinition())
    Diags.report(Method->getLocation(), diag::err_method_not_implemented, Method->getName());
```

### 7.8 方法调用与类型测试的解析

```cpp
std::unique_ptr<Expr> Sema::actOnMethodCall(SMLoc Loc, std::unique_ptr<Expr> Receiver,
                                            StringRef Name, ExprList Params) {
  if (!isDesignator(Receiver.get()))        // 变量/字段/数组元素
    return report(err_method_call_requires_variable);
  auto *RecTy = dyn_cast<RecordTypeDeclaration>(getUnderlyingType(Receiver->getType()));
  if (!RecTy) return report(err_method_call_requires_record);
  ProcedureDeclaration *Method = lookupMethod(RecTy, Name);   // 沿基类链找
  if (!Method) return report(err_undeclared_method, Name);
  RecTy->setHasTypeTag();                   // 需要虚表指针
  checkFormalAndActualParameters(Loc, Method->getFormalParams(), Params);
  return std::make_unique<MethodCallExpr>(std::move(Receiver), Method, std::move(Params));
}
```

`actOnTypeTest` 则要求左操作数是记录 designator，且被测类型是其静态类型的扩展：

```cpp
if (!TestedRec || !StaticRec || !isDesignator(E.get()))
  → err_type_test_requires_record
if (!isExtensionOf(TestedRec, StaticRec))
  → err_type_test_not_extension
StaticRec->setHasTypeTag(); TestedRec->setHasTypeTag();
```

### 7.9 诊断清单

`Diagnostic.def` 目前定义了 50 余条诊断，按来源分类如下（节选）：

| 来源 | 诊断 |
| --- | --- |
| 词法 | `err_unterminated_block_comment`、`err_unterminated_char_or_string`、`err_hex_digit_in_decimal`、`err_invalid_real_literal`、`err_longreal_literal_not_implemented` |
| 语法 | `err_expected`、`err_module_identifier_not_equal`、`err_proc_identifier_not_equal` |
| 名字与类型 | `err_symbold_declared`、`err_undeclared_name`、`err_types_for_operator_not_compatible`、`err_typedecl_requires_type` |
| 数组 | `err_array_bound_not_constant`、`err_array_bound_invalid`、`err_array_param_requires_var` |
| 记录与方法 | `err_extended_record_base_must_be_record`、`err_field_conflicts_with_base`、`err_duplicate_method`、`err_method_signature_mismatch`、`err_undeclared_method`、`err_method_not_implemented`、`err_method_body_inside_record`、`err_type_test_not_extension` |
| 调用 | `err_wrong_number_of_parameters`、`err_type_of_formal_and_actual_parameter_not_compatible`、`err_var_parameter_requires_var` |
| 控制流 | `err_if_expr_must_be_bool`、`err_while_expr_must_be_bool`、`err_function_requires_return`、`err_procedure_requires_empty_return` |
| 未实现 | `err_not_yet_implemented`（`IMPORT`） |

---

## 8. 代码生成：模块层

`lib/CodeGen/CGModule.cpp` 负责**模块级**的一切：类型映射、全局变量、符号名、类型描述符与调试 dump。它是 `CGProcedure` 的上下文对象（`CGProcedure` 持有 `CGModule &CGM`）。

### 8.1 成员与职责

```cpp
class CGModule {
  llvm::Module *M;
  ModuleDeclaration *Mod;
  llvm::DenseMap<Decl *, llvm::GlobalObject *> Globals;      // 全局变量
  llvm::DenseMap<TypeDeclaration *, llvm::GlobalVariable *> TypeDescriptors;
  llvm::DenseMap<TypeDeclaration *, unsigned> TypeIds;       // 方法名/描述符编号
public:
  llvm::Type *VoidTy, *Int1Ty, *Int32Ty, *Int64Ty, *FloatTy;
  llvm::Constant *Int32Zero;
  unsigned IfCounter = 0, WhileCounter = 0;                  // 基本块命名
  ...
};
```

类型描述符是「record 第一个字段指向的类型信息表」（虚表），`TypeIds` 给方法与描述符生成唯一编号，两个计数器保证 `if.body.N`、`while.cond.N` 这类基本块名在模块内唯一。

### 8.2 三趟流程

```cpp
void CGModule::run(ModuleDeclaration *Mod) {
  this->Mod = Mod;
  // 第一趟：声明所有过程（含方法实现），让调用与虚表能前向引用
  for (const auto &Decl : Mod->getDecls())
    if (auto *Proc = dyn_cast<ProcedureDeclaration>(Decl.get()))
      if (!Proc->isMethodDeclaration())          // 只有声明没有函数体的原型跳过
        CGProcedure(*this).declareFunction(Proc);

  // 第二趟：模块级变量（带类型标签的 record 在这里写入描述符指针）
  for (const auto &Decl : Mod->getDecls())
    if (auto *Var = dyn_cast<VariableDeclaration>(Decl.get())) {
      llvm::Type *Ty = convertType(Var->getType());
      auto *V = new llvm::GlobalVariable(*M, Ty, false,
          llvm::GlobalValue::PrivateLinkage, getZeroValue(Var->getType()),
          mangleName(Var));
      Globals[Var] = V;
    }

  // 第三趟：函数体
  for (const auto &Decl : Mod->getDecls())
    if (auto *Proc = dyn_cast<ProcedureDeclaration>(Decl.get())) {
      if (Proc->isMethodDeclaration()) continue;
      CGProcedure CGP(*this);
      CGP.run(Proc);
    }
}
```

三趟的原因：**函数签名必须先全部存在**，才能生成互相调用、递归调用，以及虚表里指向方法函数的常量指针；全局变量的初始化器又可能引用虚表，所以排在函数声明之后。

### 8.3 类型映射

```cpp
llvm::Type *CGModule::convertType(TypeDeclaration *Ty) {
  Ty = getUnderlyingType(Ty);                       // 类型别名透明
  if (auto *ArrTy = dyn_cast<ArrayTypeDeclaration>(Ty))
    return llvm::ArrayType::get(convertType(ArrTy->getElementType()),
                                ArrTy->getNumElements());
  if (auto *RecTy = dyn_cast<RecordTypeDeclaration>(Ty)) {
    llvm::SmallVector<llvm::Type *, 8> FieldTypes;
    if (RecTy->hasTypeTag())
      FieldTypes.push_back(llvm::PointerType::get(getLLVMCtx(), 0));  // 虚表指针
    for (auto *F : RecTy->getAllFields())          // 基类字段在前（平铺）
      FieldTypes.push_back(convertType(F->getType()));
    return llvm::StructType::get(getLLVMCtx(), FieldTypes, false);
  }
  if (Ty->getName() == "INTEGER") return Int64Ty;
  if (Ty->getName() == "REAL")    return FloatTy;
  if (Ty->getName() == "BOOLEAN") return Int1Ty;
  llvm::report_fatal_error("Unsupported type");
}
```

| Modula-2 | LLVM |
| --- | --- |
| `INTEGER` | `i64` |
| `REAL` | `float`（32 位 IEEE 单精度） |
| `BOOLEAN` | `i1` |
| `ARRAY [l..h] OF T` | `[h-l+1 x T]`（下界只在索引计算时参与） |
| `RECORD` | `{ [ptr,] field... }` 具名结构布局（非 packed） |

### 8.4 符号改名

```cpp
std::string CGModule::mangleName(Decl *D) {
  if (auto *Proc = dyn_cast<ProcedureDeclaration>(D))
    if (auto *Receiver = Proc->getReceiver())        // 类型绑定过程
      return mangleName(Proc->getEnclosingDecl()) + "T" +
             llvm::Twine(getTypeId(getUnderlyingType(Receiver->getType()))).str() +
             Proc->getName().str();
  std::string Mangled("_t");                         // 普通符号
  llvm::SmallVector<llvm::StringRef, 4> Parts;
  for (; D; D = D->getEnclosingDecl()) Parts.push_back(D->getName());
  while (!Parts.empty()) {
    llvm::StringRef Name = Parts.pop_back_val();
    Mangled.append(llvm::Twine(Name.size()).concat(Name).str());
  }
  return Mangled;
}
```

| 源码 | 生成的符号 |
| --- | --- |
| `MODULE Gcd` 里的 `GCD` | `_t3Gcd3GCD` |
| `MODULE Shapes` 里的全局变量 `c` | `_t6Shapes1c` |
| `Circle.Area` 的方法实现 | `_t6ShapesT1Area`（`T1` 是类型编号） |
| `Circle` 的类型描述符 | `_t6ShapesT1desc` |

方法分支在源码里的真实写法是（先解析出接收者的记录类型，再取编号）：

```cpp
if (auto *Proc = llvm::dyn_cast<ProcedureDeclaration>(D))
  if (auto *Receiver = Proc->getReceiver()) {
    auto *RecTy = llvm::cast<RecordTypeDeclaration>(
        getUnderlyingType(Receiver->getType()));
    return mangleName(Proc->getEnclosingDecl()) + "T" +
           llvm::Twine(getTypeId(RecTy)).str() + Proc->getName().str();
  }
```

编号由 `getTypeId` 首次请求时分配，因此编号顺序取决于代码生成阶段第一次用到该类型名字的顺序（第一趟声明方法函数时确定），**不依赖类型声明的先后**。

### 8.5 类型描述符（虚表）

```cpp
llvm::GlobalVariable *CGModule::getTypeDescriptor(RecordTypeDeclaration *Ty) {
  if (已有缓存) return 缓存;                       // 每个类型一份，地址唯一
  llvm::SmallVector<llvm::Constant *, 8> Impls;
  for (auto *Method : collectMethodSlots(Ty))      // 基类槽位 + 覆盖 + 新增
    Impls.push_back(方法函数（按 mangleName 查，缺了就 declareFunction）);
  llvm::ArrayType *DescTy = llvm::ArrayType::get(ptr, Impls.size());
  auto *Desc = new llvm::GlobalVariable(*M, DescTy, /*isConstant=*/true,
      llvm::GlobalValue::PrivateLinkage,
      llvm::ConstantArray::get(DescTy, Impls),
      llvm::Twine(mangleName(Mod)) + "T" + llvm::Twine(getTypeId(Ty)) + "desc");
  TypeDescriptors[Ty] = Desc;
  return Desc;
}
```

描述符是**按需创建**的，触发点是：全局/局部变量的初始化（`getZeroValue`）、方法调用（拿数组类型计算槽位地址）、`IS` 类型测试（拿地址做比较）。

### 8.6 零值初始化与类型标签

```cpp
llvm::Constant *CGModule::getZeroValue(TypeDeclaration *Ty) {
  llvm::Type *LLVMTy = convertType(Ty);
  TypeDeclaration *Underlying = getUnderlyingType(Ty);
  if (auto *RecTy = dyn_cast<RecordTypeDeclaration>(Underlying)) {
    if (!RecTy->hasTypeTag()) return llvm::ConstantAggregateZero::get(LLVMTy);
    llvm::SmallVector<llvm::Constant *, 8> Fields;
    Fields.push_back(getTypeDescriptor(RecTy));          // 第 0 个字段：虚表指针
    for (auto *F : RecTy->getAllFields())
      Fields.push_back(getZeroValue(F->getType()));      // 递归：嵌套 record 也要带标签
    return llvm::ConstantStruct::get(cast<llvm::StructType>(LLVMTy), Fields);
  }
  if (auto *ArrTy = dyn_cast<ArrayTypeDeclaration>(Underlying)) {
    ... // 元素是需要标签的 record 时逐元素构造，否则 ConstantAggregateZero
  }
  if (LLVMTy->isFloatingPointTy()) return llvm::ConstantFP::get(LLVMTy, 0.0);
  return llvm::ConstantInt::get(LLVMTy, 0);
}
```

全局变量初始化与局部变量初始化共用这个入口，因此「对象的虚表指针」在整个编译器里只有这一处构造逻辑。

### 8.7 调试：IR dump

配置 `-DTINYLANG_ENABLE_IR_DUMP=ON` 后，`CGModule::dump()` 会把当前模块写到 `tinylang-dump-<N>.ll`（`N` 为递增计数），调用点分布在 phi 节点创建与补全处（见 §9.4）：

```cpp
#ifdef TINYLANG_ENABLE_IR_DUMP
  CGM.dump();     // addEmptyPhi / addPhiOperands 之后各一处
#endif
```

想观察「局部变量如何一步步变成 phi」，这是最直接的手段。

### 8.8 CodeGenerator 与驱动

```cpp
std::unique_ptr<llvm::Module> CodeGenerator::run(ModuleDeclaration *Mod,
                                                 const std::string &FileName) {
  auto M = std::make_unique<llvm::Module>(FileName, Ctx);
  M->setTargetTriple(TM->getTargetTriple());
  M->setDataLayout(TM->createDataLayout());     // 与 C ABI 一致的关键
  CGModule CGM(M.get());
  CGM.run(Mod);
  return M;
}
```

`tools/driver/Driver.cpp` 负责命令行、目标机与输出：`-o` 指定输出文件名，`--emit-llvm` 走 `createPrintModulePass` 打印文本 IR，否则用 `TargetMachine::addPassesToEmitFile` 生成汇编或目标文件。解析/语义阶段只要出现 error，就跳过代码生成并以退出码 1 结束。

---

## 9. 代码生成：过程层与 SSA 构造

`lib/CodeGen/CGProcedure.cpp` 是编译器最复杂的部分，核心是**用 SSA 表示标量局部变量**，并在必要时把变量降级到栈上。

### 9.1 状态

```cpp
class CGProcedure {
  CGModule &CGM;
  llvm::IRBuilder<> Builder;
  llvm::BasicBlock *Curr;                 // 当前插入点
  ProcedureDeclaration *Proc;
  llvm::FunctionType *Fty; llvm::Function *Fn;

  struct BasicBlockDef {
    llvm::DenseMap<Decl *, llvm::TrackingVH<llvm::Value>> Defs;  // 变量在当前块的 SSA 值
    llvm::DenseMap<llvm::PHINode *, Decl *> IncompletePhis;      // 未填 incoming 的 phi
    unsigned Sealed : 1;                                         // 前驱是否已全部确定
  };
  llvm::DenseMap<llvm::BasicBlock *, BasicBlockDef> CurrentDef;
  llvm::DenseMap<FormalParameterDeclaration *, llvm::Argument *> FormalParams;
  llvm::DenseMap<Decl *, llvm::Value *> PromotedLocals;   // 需要内存表示的标量
  llvm::SmallPtrSet<Decl *, 8> ByRefLocals;               // 被当作 VAR 实参的声明
};
```

两个细节值得注意：

- `Defs` 的值类型是 `llvm::TrackingVH<llvm::Value>`：当后续优化（`optimizePhi` 的 RAUW）替换掉某个 phi 时，映射表会自动更新，避免悬空引用。
- `Sealed` 表示「这个基本块的所有前驱都已知」，只有此时才能安全地给 phi 填 incoming 值。

### 9.2 函数创建与参数绑定

```cpp
llvm::FunctionType *CGProcedure::createFunctionType(ProcedureDeclaration *Proc) {
  ResultTy = Proc->getRetType() ? mapType(Proc->getRetType()) : CGM.VoidTy;
  for (const auto &FP : Proc->getFormalParams())
    ParamTypes.push_back(mapType(FP.get()));      // VAR 形参 → ptr
  return llvm::FunctionType::get(ResultTy, ParamTypes, false);
}

llvm::Type *CGProcedure::mapType(Decl *D, bool HonorReference) {
  if (auto *FP = dyn_cast<FormalParameterDeclaration>(D)) {
    if (FP->isVar() && HonorReference) return ptr;      // 引用参数
    return CGM.convertType(FP->getType());
  }
  if (auto *V = dyn_cast<VariableDeclaration>(D)) return CGM.convertType(V->getType());
  return CGM.convertType(cast<TypeDeclaration>(D));
}
```

参数传递规则汇总（对应 IR 见 §11）：

| 形参形式 | LLVM 签名 | 实参侧 | 被调函数内 |
| --- | --- | --- | --- |
| `x: INTEGER/REAL/BOOLEAN` | 标量按值 | `emitExpr()` 取值 | 走 SSA（需要地址时降级到栈） |
| `VAR x: 标量` | `ptr` | `emitLValue()` 取地址 | 通过指针 load/store |
| `x: 记录`（值形参） | 记录按值 | `load` 出整个记录再传 | 拷贝到 `alloca` 后按地址访问（数组值形参被 Sema 拒绝） |
| `VAR x: 记录/数组` | `ptr` | `emitLValue()` 取地址 | 直接操作调用者对象 |
| 方法接收者（隐式） | 首参 `ptr` | 接收者 designator 的地址 | 同 `VAR` 记录形参 |

VAR 形参会额外附加属性，便于优化器推理：

```cpp
llvm::AttrBuilder Attr(CGM.getLLVMCtx());
llvm::TypeSize Sz = DL.getTypeStoreSize(CGM.convertType(FP->getType()));
Attr.addDereferenceableAttr(Sz);
Attr.addCapturesAttr(llvm::CaptureInfo::none());
Arg.addAttrs(Attr);
```

### 9.3 过程体的初始化顺序

```cpp
void CGProcedure::run(ProcedureDeclaration *Proc) {
  this->Proc = Proc;
  Fty = createFunctionType(Proc);
  Fn  = createFunction(Proc, Fty);
  Curr = BasicBlock::Create(ctx, "entry", Fn);

  collectByRefLocals(Proc->getStmts());     // ① 先扫描：哪些标量必须进内存
  for (形参) { ② 建立 FormalParams / alloca / SSA 映射 }
  for (局部声明) { ③ 聚合 alloca + 零值；被 VAR 用到的标量也 alloca }

  emit(Proc->getStmts());                   // ④ 生成语句
  if (!Curr->getTerminatorOrNull()) Builder.CreateRetVoid();
  sealBlock(Curr);
}
```

第 ① 步很关键：**变量用 SSA 还是内存表示，必须在生成函数体之前决定**。`collectByRefLocals` 递归遍历语句与表达式，遇到调用时用 `noteVarArguments` 对照形参表，凡是「实参是当前过程里的标量局部变量/值形参，且对应形参是 VAR」的，就记进 `ByRefLocals`；第 ③ 步为它们分配 `alloca`（局部变量零初始化、值形参存入传入值），此后读写都经内存。

如果改成「真正需要地址时再临时降级」，会踩到一个语义陷阱：在循环体里降级时写入的是当时那份循环不变的 SSA 值，每次迭代都会把累加变量重置。`example/VarParam.mod` 的 `LoopIncrement`（在 `WHILE` 里 `Inc(s)`）就是为这个场景补的回归用例。

### 9.4 SSA 构造算法

核心接口是 `readLocalVariable` / `writeLocalVariable`：前者取变量在某个基本块末尾的值，后者写入。

```cpp
void CGProcedure::writeLocalVariable(BB, D, Val) { CurrentDef[BB].Defs[D] = Val; }

llvm::Value *CGProcedure::readLocalVariable(BB, Decl) {
  auto Val = CurrentDef[BB].Defs.find(Decl);
  if (Val != CurrentDef[BB].Defs.end()) return Val->second;   // 本块已有定义
  return readLocalVariableRecursive(BB, Decl);
}

llvm::Value *CGProcedure::readLocalVariableRecursive(BB, Decl) {
  if (!CurrentDef[BB].Sealed) {              // 前驱未知：先放一个「空 phi」
    llvm::PHINode *Phi = addEmptyPhi(BB, Decl);
    CurrentDef[BB].IncompletePhis[Phi] = Decl;
    Val = Phi;
  } else if (auto *PredBB = BB->getSinglePredecessor()) {
    Val = readLocalVariable(PredBB, Decl);   // 单前驱：直接向前找
  } else {                                   // 多前驱：建 phi 并填 incoming
    llvm::PHINode *Phi = addEmptyPhi(BB, Decl);
    writeLocalVariable(BB, Decl, Phi);
    Val = addPhiOperands(BB, Decl, Phi);
  }
  writeLocalVariable(BB, Decl, Val);
  return Val;
}
```

三种情况对应「sealed block」SSA 构造算法的三个分支：块未封闭时先建空 phi 并登记待补；单前驱时无需 phi；多前驱且已封闭时建 phi 并对每个前驱递归取值。

```cpp
void CGProcedure::sealBlock(llvm::BasicBlock *BB) {
  for (auto PhiDecl : CurrentDef[BB].IncompletePhis)
    addPhiOperands(BB, PhiDecl.second, PhiDecl.first);
  CurrentDef[BB].IncompletePhis.clear();
  CurrentDef[BB].Sealed = true;
}

llvm::Value *CGProcedure::addPhiOperands(BB, D, Phi) {
  for (auto *PredBB : predecessors(BB))
    Phi->addIncoming(readLocalVariable(PredBB, D), PredBB);
  return optimizePhi(Phi);
}
```

`optimizePhi` 做一次简单化简：若所有 incoming（除自己）都是同一个值，就直接用该值替换 phi 并删除它，再递归检查用到它的其他 phi。

以 `example/Gcd.mod` 为例，`a`、`b` 在循环中被反复赋值，最终生成两个 phi；而只在循环体内用一次的 `t` 完全没有出现在 IR 里：

```llvm
while.cond.0:                                     ; preds = %while.body.0, %after.if.0
  %1 = phi i64 [ %4, %while.body.0 ], [ %b, %after.if.0 ]   ; b
  %2 = phi i64 [ %1, %while.body.0 ], [ %a, %after.if.0 ]   ; a
  %3 = icmp ne i64 %1, 0
  br i1 %3, label %while.body.0, label %after.while.0
while.body.0:
  %4 = srem i64 %2, %1                                       ; t := a MOD b
  br label %while.cond.0
after.while.0:
  ret i64 %2
```

### 9.5 表达式生成

`emitExpr` 是一张分派表：

| AST 节点 | 生成方式 |
| --- | --- |
| `IntegerLiteral` / `RealLiteral` / `BooleanLiteral` | `ConstantInt` / `ConstantFP` / `ConstantInt(i1)` |
| `VariableAccess` | `readVariable`（标量返回值，聚合返回地址） |
| `ConstantAccess` | 递归生成常量表达式 |
| `InfixExpression` | 整数 `add/sub/mul/sdiv/srem/and/or/icmp`；REAL 用 `fadd/fsub/fmul/fdiv/fcmp` |
| `PrefixExpression` | `neg` / `fneg` / `not`；一元 `+` 无操作 |
| `FunctionCallExpr` | 处理实参 + `CreateCall`；聚合返回值 spill 到 `alloca` 后返回地址 |
| `MethodCallExpr` | 虚表间接调用（§10.3） |
| `TypeTestExpr` | 描述符指针比较（§10.4） |
| `IndexedExpression` | `emitLValue` 算地址，元素是聚合就返回地址，否则 `load` |
| `FieldAccess` | 同上 |

算术与比较按操作数类型选择指令（REAL 与 INTEGER 属于两个类型族，不会混用）：

```cpp
bool IsReal = getUnderlyingType(E->getLeft()->getType())->getName() == "REAL";
case tok::plus:  IsReal ? Builder.CreateFAdd(L, R)  : Builder.CreateNSWAdd(L, R);
case tok::star:  IsReal ? Builder.CreateFMul(L, R)  : Builder.CreateNSWMul(L, R);
case tok::slash: Builder.CreateFDiv(L, R);                       // 仅 REAL 合法
case tok::equal: IsReal ? Builder.CreateFCmpOEQ(L, R) : Builder.CreateICmpEQ(L, R);
```

### 9.6 左值与数组下标

```cpp
llvm::Value *CGProcedure::emitLValue(Expr *E) {
  // VariableAccess:
  //   局部变量 → 已在内存（聚合，或被 VAR 用到而提前降级）→ 直接返回地址
  //   全局变量 → CGM.getGlobal(D)
  //   VAR 形参 → FormalParams[FP]
  // IndexedExpression → 基址 + GEP
  // FieldAccess       → 基址 + GEP（下标来自 getFieldIndex）
}
```

Modula-2 数组的上下界是任意的，索引前要减去下界归一化：

```cpp
llvm::Value *Base = emitExpr(E->getBase());
llvm::Value *Index = emitExpr(E->getIndex());
Index = Builder.CreateSub(Index, Builder.getInt64(ArrTy->getLowBound()));
return Builder.CreateGEP(ArrTyLLVM, Base, {getInt64(0), Index});
```

### 9.7 语句生成

| 语句 | 生成方式 |
| --- | --- |
| `x := expr`（标量） | `writeVariable`：全局 store、VAR 形参 store、局部标量更新 SSA 值 |
| `r := expr`（聚合） | `emitMemCpy(dst, src, Ty)`，用 DataLayout 取 ABI 对齐与存储大小 |
| 过程调用 | 处理实参后 `CreateCall` |
| 方法调用 | 虚表间接调用 |
| `IF` | 建 `if.body.N`/`else.body.N`/`after.if.N`，条件 `CreateCondBr`，分支结束补 `br` |
| `WHILE` | 建 `while.cond.N`/`while.body.N`/`after.while.N`，条件块在循环体之后才 `sealBlock` |
| `RETURN` | 有值 `CreateRet`（聚合先 `load`），否则 `CreateRetVoid` |

`IF`/`WHILE` 的基本块名带全局递增编号，便于阅读与调试：

```cpp
unsigned IfNo = CGM.IfCounter++;
auto *IfBB = llvm::BasicBlock::Create(ctx, Twine("if.body.") + Twine(IfNo), Fn);
```

---

## 10. 面向对象模型：继承与多态

Modula-2 本身没有继承，这里采用其后继语言 **Oberon-2 的类型扩展模型**：`RECORD (Base)` 扩展记录、类型绑定过程、通过类型描述符做动态分派、`v IS T` 动态类型测试。

### 10.1 内存布局：前缀兼容

派生记录的字段**平铺**存储：隐藏的描述符指针 + 基类字段（递归）+ 自身字段。

```text
Shape  = { ptr, i64, i64 }              // desc | x | y
Circle = { ptr, i64, i64, i64 }         // desc | x | y | radius
Blob   = { ptr, i64, i64, i64 }         // desc | x | y | weight
```

因此 `Circle*` 可以直接当作 `Shape*` 使用（`AreaOf(VAR sh: Shape)` 的形参只承诺 `dereferenceable(24)`），这正是多态参数能够工作的基础。字段下标由 `getFieldIndex` 统一计算，`ci.radius` 在 `Circle` 中是第 3 个字段：

```llvm
define i64 @_t6ShapesT1Area(ptr captures(none) dereferenceable(32) %ci) {
  %0 = getelementptr { ptr, i64, i64, i64 }, ptr %ci, i32 0, i32 3
  %1 = load i64, ptr %0, align 8
  ...
}
```

### 10.2 虚表与槽位分配

`collectMethodSlots` 把继承链展平成一维槽位序列：

```text
Shape : [Area→Shape.Area,  Kind→Shape.Kind]
Circle: [Area→Circle.Area, Kind→Circle.Kind]     // 两个都覆盖
Blob  : [Area→Blob.Area,   Kind→Shape.Kind]      // 只覆盖 Area，Kind 复用基类实现
```

对应的描述符全局变量（注意 `T3desc` 的第二个槽位直接指向 `Shape` 的实现）：

```llvm
@_t6ShapesT1desc = private constant [2 x ptr] [ptr @_t6ShapesT1Area, ptr @_t6ShapesT1Kind]
@_t6ShapesT3desc = private constant [2 x ptr] [ptr @_t6ShapesT3Area, ptr @_t6ShapesT0Kind]
```

槽位在**代码生成阶段**计算（而不是解析时），因此「先声明派生类型、之后才写基类方法」也能得到正确布局。

### 10.3 动态分派

```llvm
define i64 @_t6Shapes6AreaOf(ptr captures(none) dereferenceable(24) %sh) {
  %0 = load ptr, ptr %sh                       ; ① 从对象第 0 个字段取运行期描述符
  %1 = getelementptr [2 x ptr], ptr %0, i32 0, i32 0
  %2 = load ptr, ptr %1                        ; ② 取槽位里的实现
  %3 = call i64 %2(ptr %sh)                    ; ③ 接收者按引用传入
  ret i64 %3
}
```

三个关键点：

- 槽位号由**静态类型**决定（`Shape` 上 `Area` 是 0 号槽）；
- 实现来自**动态类型**的描述符（运行期才知道对象是 `Circle` 还是 `Blob`）；
- 接收者始终按引用传递，所以方法内对字段的修改对调用者可见，`VAR` 形参上的调用也能正确分派。

### 10.4 类型测试 `IS`

```llvm
%0 = load ptr, ptr %sh, align 8            ; 取对象描述符
%1 = icmp eq ptr %0, @_t6ShapesT1desc      ; 与 Circle 的描述符比较
```

每个类型只有一份描述符，指针相等即类型相同，因此测试是 O(1)，不需要额外运行时支持，也不需要遍历继承链。

### 10.5 编译期规则小结

| 规则 | 违反时的诊断 |
| --- | --- |
| 扩展的基类型必须是 record | `err_extended_record_base_must_be_record` |
| 派生记录不能重声明基类字段 | `err_field_conflicts_with_base` |
| 同一类型内不能重复声明同名方法 | `err_duplicate_method` |
| 覆盖必须同名同签名 | `err_method_signature_mismatch` |
| record 内声明的方法必须有实现 | `err_method_not_implemented` |
| 方法体不能写在 record 内 | `err_method_body_inside_record` |
| 接收者类型必须是 record | `err_receiver_requires_record` |
| 方法调用的接收者必须是 record 的 designator | `err_method_call_requires_variable` / `err_method_call_requires_record` |
| 方法必须在接收者静态类型上可见 | `err_undeclared_method` |
| `IS` 的被测类型必须是静态类型的扩展 | `err_type_test_not_extension` |

---

## 11. 运行时约定与 C ABI

### 11.1 符号与类型对照

| Modula-2 | C（`example/call*.c` 中的写法） | 说明 |
| --- | --- | --- |
| `PROCEDURE F(...): INTEGER` | `long _t...F(...)` | `i64` |
| 返回 `REAL` | `float _t...F(...)` | 32 位单精度 |
| 返回 `BOOLEAN` | `_Bool`（当前示例未用到，ABI 上就是 `i1` 零扩展返回） | `i1` |
| `VAR x: INTEGER` 形参 | `long *` | `ptr`，带 `dereferenceable` 属性 |
| 记录值形参 | 按值传结构体 | 布局与 C 结构体一致（非 packed） |

### 11.2 值参数与引用参数的 IR 对比

以 `VarParam.mod` 为例：

```llvm
define void @_t8VarParam3Inc(ptr captures(none) dereferenceable(8) %n) {   ; VAR n: INTEGER
entry:
  %0 = load i64, ptr %n, align 8
  %1 = add nsw i64 %0, 1
  store i64 %1, ptr %n, align 8
  ret void
}

define i64 @_t8VarParam7BumpVal(i64 %n) {                                 ; n: INTEGER
entry:
  %0 = add nsw i64 %n, 1
  ret i64 %0
}
```

同一个局部变量既按值又被引用传递时，它会整体落到栈上：值调用前先 `load`，引用调用传地址。

```llvm
define i64 @_t8VarParam10MixedCalls() {
entry:
  %k = alloca i64, align 8
  store i64 0, ptr %k, align 8
  store i64 10, ptr %k, align 8
  %0 = load i64, ptr %k, align 8            ; 值调用前取值
  %1 = call i64 @_t8VarParam7BumpVal(i64 %0)
  call void @_t8VarParam3Inc(ptr %k)        ; VAR 调用传地址
  %2 = load i64, ptr %k, align 8            ; 读到被修改后的值
  ...
}
```

### 11.3 端到端验证方式

每个示例都配一个 C 验证程序，用 C 里独立实现的参考结果对比 tinylang 生成代码的结果：

```sh
build/tools/driver/tinylang example/Record.mod -o /tmp/Record.s
cc -c /tmp/Record.s -o /tmp/Record.o
cc -c example/callrecord.c -o /tmp/callrecord.o
cc /tmp/Record.o /tmp/callrecord.o -o /tmp/callrecord
/tmp/callrecord
```

这种「Modula-2 实现 + C 参考实现 + 逐项断言」的模式一次验证了三件事：语言语义、ABI 兼容性、真实链接后的端到端行为。

### 11.4 示例程序与覆盖点

| 示例 | 覆盖的语言特性 |
| --- | --- |
| `Gcd.mod` | 循环、`MOD`、`IF`、SSA phi |
| `Fib.mod` | 递归、多过程、全局变量 |
| `Arrays.mod` | 静态数组、多维数组、下界归一化 |
| `TypeAlias.mod` | 类型别名透明解析 |
| `Record.mod` | 嵌套记录、`VAR`/值形参、整体赋值（memcpy） |
| `Real.mod` | 32 位浮点、科学计数法、局部 REAL 的 phi |
| `ReturnRecord.mod` / `ReturnArray.mod` | 聚合类型作为返回值 |
| `Shapes.mod` | 继承、方法覆盖、动态分派、`IS` |
| `VarParam.mod` | 值/引用参数、标量降级到内存、循环内传引用 |
| `Expr.mod` | 运算符优先级、括号、一元负号的作用范围、关系与逻辑运算 |

---

## 12. ASTDumper 工具

`lib/ASTDumper/ASTDumper.cpp` 用缩进风格打印 AST，直观体现语句与表达式之间的嵌套：

```text
ModuleDeclaration 'Shapes'
  Declarations:
    TypeAliasDeclaration 'Circle' = RECORD (extends Shape) (radius: INTEGER) [PROCEDURE Area(): INTEGER]
    VariableDeclaration 'c' : Circle
    ProcedureDeclaration 'Area' : INTEGER
      Receiver Circle
      Statements:
        ReturnStatement
          InfixExpression '*' : INTEGER
            ...
```

实现要点：

- 状态只有 `OS` 与 `Indent`，`printIndent()` 输出 `2*Indent` 个空格；
- 每个节点一个 `dump*` 方法，`dumpDecl`/`dumpStmt`/`dumpExpr` 按 `Kind` 分派；
- 类型打印分成两个入口：`dumpTypeName`（**引用**位置，优先打印类型名，如 `Circle`）与 `dumpTypeDefinition`（**定义**位置，展开成 `RECORD (extends Shape) (radius: INTEGER) [...]`）；
- 匿名类型在有别名时会被 Sema 补上名字（`TypeDeclaration::setName`），因此 dump 里能看到可读的类型名。

命令行工具 `tools/driver/astdump/ASTDump.cpp` 复用同一套 Lexer/Parser/Sema 流程，只是把输出换成 `llvm::outs()`；有诊断时返回 1。

---

## 13. 测试与验证体系

### 13.1 诊断回归（`test/`）

约定：每个负向用例是一个 `.mod` 文件，文件名形如 `Diag<Feature>.mod`，**文件头部注释列出期望诊断**；`Gcd.mod` 是正向基线，必须零诊断。

```modula2
(* Expected diagnostic:
   err_types_for_operator_not_compatible (operator +)
   Mixing INTEGER and BOOLEAN operands. *)
MODULE Gcd;
VAR x : INTEGER; ok : BOOLEAN;
BEGIN
  x := 1 + ok
END Gcd.
```

批量运行（正向基线要求返回 0，其余要求返回 1）：

```sh
for f in test/*.mod; do
  build/tools/driver/tinylang "$f" -o /tmp/out.s
  echo "$? $f"
done
```

当前共 42 个用例（1 个正向基线 + 41 个负向用例），覆盖：词法（未终止注释/字符串、非法实数字面量、十六进制数字混入十进制）、语法（模块名/过程名不匹配、`err_expected`）、类型与运算符、数组边界、实参匹配、`VAR` 实参必须是变量、`RETURN` 规则，以及继承/方法/`IS` 的全部错误分支。

### 13.2 端到端示例（`example/`）

见 §11.3 与 §11.4。每个 `call*.c` 逐项打印结果，任一不符即以非零码退出，可以直接放进 CI。

### 13.3 IR 快照与合法性校验

`example/llvmir/*.ll` 是随仓库提交的 IR 快照，方便不重新编译也能阅读生成结果（本文档中的 IR 片段大多取自这里）。用 `llvm-as`（位于 LLVM 安装目录的 `bin/`）校验它们：

```sh
LLVM_BIN=/path/to/llvm/bin
for f in example/llvmir/*.ll; do "$LLVM_BIN/llvm-as" "$f" -o /dev/null && echo "$f ok"; done
```

修改代码生成后重新生成快照：

```sh
for f in example/*.mod; do
  build/tools/driver/tinylang "$f" -o "example/llvmir/$(basename "${f%.mod}").ll" --emit-llvm
done
```

---

## 14. 设计取舍与实现要点

1. **Parser/Sema 解耦**：Parser 只做语法，节点构造与类型检查都在 Sema。代价是 `actOn*` 接口较多（40 余个），收益是职责边界清晰，诊断逻辑集中在一处。
2. **表驱动而不是硬编码**：token、关键字、诊断都由 `.def` 文件生成，新增一条诊断只需加一行。
3. **AST 上直接携带类型**：`Expr::getType()` 是 Sema 的产物，CodeGen 只读不查，避免把类型检查做两遍。
4. **标量走 SSA、聚合走内存**：标量局部变量用 SSA + phi，聚合（数组/记录）用 `alloca` + GEP。这让 Gcd/Fib 这类示例的 IR 非常干净——只在循环体内用一次的临时变量会被完全消除。
5. **按需降级**：只有当标量变量的地址真的会传出去（作为 VAR 实参）时，才把它改成内存表示；降级决策在生成函数体前完成，避免循环中的语义错误。
6. **虚表指针内嵌于对象**：类型描述符指针放在 record 第 0 个字段，派生记录天然前缀兼容基记录，动态分派与 `IS` 都退化成一次 load + 间接调用/比较。
7. **描述符按需创建并缓存**：`TypeDescriptors` 保证每个类型一份、地址唯一，这既是 `IS` 正确性的前提，也避免了初始化顺序问题（函数声明在第一趟全部完成后才建描述符）。

---

## 15. 已知限制与路线图

**当前不支持**

- `IMPORT` 与多模块（报 `err_not_yet_implemented`）；
- 模块体语句会被解析但不生成代码（示例统一通过过程 + C 驱动调用）；
- 嵌套过程（CodeGen 遇到会 `report_fatal_error`）；
- 类型保护 `v(T)`、通过类型名调用 `T.M(v)`、抽象方法（声明了就必须实现）；
- 指针类型、变体记录、`WITH`、`SET`、`CASE`、`FOR`；
- 整数与浮点之间的显式转换函数（`FLOAT`/`TRUNC`）、`LONGREAL`；
- 优化 pass（命令行可解析 `-O` 等参数，但尚未接入优化管线）。

**可以预期的下一步**：`CASE`/`FOR` 语句、`IMPORT` 与多模块（配合符号导出约定）、类型保护 `v(T)`、把示例验证脚本化并接入 CI。

---

## 16. 扩展指南

新增一个语言特性的推荐顺序（可以拿 `IS` 类型测试当模板）：

1. **词法**（若需要新关键字/字面量）：在 `TokenKinds.def` 加 `KEYWORD`/`TOK`，必要时在 `Lexer::next` 或 `number`/`string` 里加分支。
2. **诊断**：在 `Diagnostic.def` 加条目（错误用 `err_*`，警告用 `warn_*`）。
3. **AST**：在 `AST.h` 添加节点类与新的 `DeclKind`/`ExprKind`/`StmtKind` 枚举值；若新节点需要类型关系判断，补一个辅助函数（参考 `isExtensionOf`）。
4. **语法**：在 `Parser.cpp` 对应函数加分支，同时更新错误恢复用的 `skipUntil(...)` 同步点集合——否则新语法出错时会引发连锁诊断。
5. **语义**：在 `Sema.h/.cpp` 增加 `actOn*` 回调（建节点 + 类型检查 + 报错），必要时扩展 `isSameType`/`isOperatorForType`/`isCompatibleWithFormal` 等类型规则。
6. **代码生成**：新类型改 `CGModule::convertType`；新表达式/语句改 `CGProcedure::emitExpr`/`emitLValue`/`emitStmt`，注意区分「值语义」与「地址语义」。
7. **ASTDumper**：加 `dump*` 方法并在分派处登记。
8. **测试**：用 `test/Diag<Feature>.mod` 覆盖每个新诊断；在 `example/` 加正向示例 + `call*.c` 断言，并重新生成 `example/llvmir/*.ll`。
9. **文档**：同步 `README.md`、`README_zh.md`（特性、示例、用例数量）与本文档。

以「新增一条语句」为例，最小改动集是：`AST.h`（新 `StmtKind` + 节点类）→ `Parser.cpp`（`parseStatement` 分支）→ `Sema.cpp`（`actOn*` 检查）→ `CGProcedure::emitStmt` 与 `emit()` 分派 → `ASTDumper::dumpStmt` → 测试与示例。

---

## 附：快速索引

| 想了解 | 去看 |
| --- | --- |
| token / 关键字定义 | `include/tinylang/Basic/TokenKinds.def` |
| 诊断定义 | `include/tinylang/Basic/Diagnostic.def` |
| 词法细节（数字、注释） | `lib/Lexer/Lexer.cpp` |
| AST 节点与类型辅助函数 | `include/tinylang/AST/AST.h` |
| 语法与错误恢复 | `lib/Parser/Parser.cpp` |
| 类型规则、继承与方法检查 | `lib/Sema/Sema.cpp` |
| 类型映射、虚表、符号名 | `lib/CodeGen/CGModule.cpp` |
| SSA 构造、表达式/语句生成 | `lib/CodeGen/CGProcedure.cpp` |
| 命令行与输出 | `tools/driver/Driver.cpp` |
| AST 打印 | `lib/ASTDumper/ASTDumper.cpp` |
| 示例与 IR 快照 | `example/`、`example/llvmir/` |
| 回归用例 | `test/` |
