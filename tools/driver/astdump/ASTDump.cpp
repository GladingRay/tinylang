#include "tinylang/ASTDumper/ASTDumper.h"
#include "tinylang/Basic/Diagnostic.h"
#include "tinylang/Lexer/Lexer.h"
#include "tinylang/Parser/Parser.h"
#include "tinylang/Sema/Sema.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace tinylang;

static cl::opt<std::string> InputFile(cl::Positional,
                                      cl::desc("<input-file>"),
                                      cl::init("-"));

int main(int argc, const char **argv) {
  llvm::InitLLVM X(argc, argv);
  llvm::cl::ParseCommandLineOptions(argc, argv, "tinylang AST dumper\n");

  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> FileOrErr =
      llvm::MemoryBuffer::getFile(InputFile);
  if (std::error_code BufferError = FileOrErr.getError()) {
    llvm::WithColor::error(llvm::errs(), argv[0])
        << "Error reading " << InputFile << ": " << BufferError.message()
        << "\n";
    return 1;
  }

  llvm::SourceMgr SrcMgr;
  DiagnosticsEngine Diags(SrcMgr);
  SrcMgr.AddNewSourceBuffer(std::move(*FileOrErr), llvm::SMLoc());

  Lexer TheLexer(SrcMgr, Diags);
  Sema TheSema(Diags);
  Parser TheParser(TheLexer, TheSema);
  auto Mod = TheParser.parse();

  if (Mod)
    ASTDumper(llvm::outs()).dump(Mod.get());
  return Diags.numErrors() > 0 ? 1 : 0;
}
