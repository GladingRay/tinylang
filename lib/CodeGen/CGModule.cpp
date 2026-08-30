#include "tinylang/CodeGen/CGModule.h"
#include "tinylang/CodeGen/CGProcedure.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

using namespace tinylang;

#ifdef TINYLANG_ENABLE_IR_DUMP
namespace {
/// Counter used to give each dumped IR snapshot a unique file name.
unsigned DumpFileCounter = 0;
} // namespace
#endif

void CGModule::initialize() {
  VoidTy = llvm::Type::getVoidTy(getLLVMCtx());
  Int1Ty = llvm::Type::getInt1Ty(getLLVMCtx());
  Int32Ty = llvm::Type::getInt32Ty(getLLVMCtx());
  Int64Ty = llvm::Type::getInt64Ty(getLLVMCtx());
  Int32Zero = llvm::ConstantInt::get(Int32Ty, 0, /*isSigned*/ true);
}

llvm::Type *CGModule::convertType(TypeDeclaration *Ty) {
  Ty = getUnderlyingType(Ty);
  if (auto *ArrTy = llvm::dyn_cast<ArrayTypeDeclaration>(Ty))
    return llvm::ArrayType::get(
        convertType(ArrTy->getElementType()),
        static_cast<uint64_t>(ArrTy->getNumElements()));
  if (auto *RecTy = llvm::dyn_cast<RecordTypeDeclaration>(Ty)) {
    llvm::SmallVector<llvm::Type *, 8> FieldTypes;
    for (const auto &F : RecTy->getFields())
      FieldTypes.push_back(
          convertType(llvm::cast<VariableDeclaration>(F.get())->getType()));
    return llvm::StructType::get(getLLVMCtx(), FieldTypes,
                                 /*isPacked=*/false);
  }
  if (Ty->getName() == "INTEGER")
    return Int64Ty;
  if (Ty->getName() == "BOOLEAN")
    return Int1Ty;
  llvm::report_fatal_error("Unsupported type");
}

std::string CGModule::mangleName(Decl *D) {
  std::string Mangled("_t");
  llvm::SmallVector<llvm::StringRef, 4> Parts;
  for (; D; D = D->getEnclosingDecl())
    Parts.push_back(D->getName());
  while (!Parts.empty()) {
    llvm::StringRef Name = Parts.pop_back_val();
    Mangled.append(llvm::Twine(Name.size()).concat(Name).str());
  }
  return Mangled;
}

llvm::GlobalObject *CGModule::getGlobal(Decl *D) { return Globals[D]; }

#ifdef TINYLANG_ENABLE_IR_DUMP
void CGModule::dump() {
  std::string FileName =
      (llvm::Twine("tinylang-dump-") + llvm::Twine(DumpFileCounter++) +
       ".ll")
          .str();
  std::error_code EC;
  llvm::raw_fd_ostream OS(FileName, EC, llvm::sys::fs::OF_Text);
  if (EC) {
    llvm::errs() << "Error opening " << FileName << ": " << EC.message()
                 << "\n";
    return;
  }
  M->print(OS, nullptr);
  OS.flush();
}
#endif

void CGModule::run(ModuleDeclaration *Mod) {
  this->Mod = Mod;
  // First pass: emit module-level variables and declare all procedures so
  // that calls can reference them regardless of declaration order (this also
  // makes recursive and mutually recursive calls work).
  for (const auto &Decl : Mod->getDecls()) {
    if (auto *Var = llvm::dyn_cast<VariableDeclaration>(Decl.get())) {
      llvm::Type *Ty = convertType(Var->getType());
      llvm::Constant *Init = Ty->isAggregateType()
                                 ? llvm::ConstantAggregateZero::get(Ty)
                                 : llvm::ConstantInt::get(Ty, 0);
      llvm::GlobalVariable *V = new llvm::GlobalVariable(
          *M, Ty, false,
          llvm::GlobalValue::PrivateLinkage, Init, mangleName(Var));
      Globals[Var] = V;
    } else if (auto *Proc = llvm::dyn_cast<ProcedureDeclaration>(Decl.get())) {
      CGProcedure(*this).declareFunction(Proc);
    }
  }
  // Second pass: emit the procedure bodies.
  for (const auto &Decl : Mod->getDecls()) {
    if (auto *Proc = llvm::dyn_cast<ProcedureDeclaration>(Decl.get())) {
      CGProcedure CGP(*this);
      CGP.run(Proc);
    }
  }
}
