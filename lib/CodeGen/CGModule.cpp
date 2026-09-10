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
  FloatTy = llvm::Type::getFloatTy(getLLVMCtx());
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
    // Extended records start with the type descriptor pointer so that a
    // derived record can be viewed as its base type.
    if (RecTy->hasTypeTag())
      FieldTypes.push_back(llvm::PointerType::get(getLLVMCtx(), 0));
    for (auto *F : RecTy->getAllFields())
      FieldTypes.push_back(convertType(F->getType()));
    return llvm::StructType::get(getLLVMCtx(), FieldTypes,
                                 /*isPacked=*/false);
  }
  if (Ty->getName() == "INTEGER")
    return Int64Ty;
  if (Ty->getName() == "REAL")
    return FloatTy;
  if (Ty->getName() == "BOOLEAN")
    return Int1Ty;
  llvm::report_fatal_error("Unsupported type");
}

std::string CGModule::mangleName(Decl *D) {
  // Type-bound procedures are distinguished by their receiver type.
  if (auto *Proc = llvm::dyn_cast<ProcedureDeclaration>(D))
    if (auto *Receiver = Proc->getReceiver()) {
      auto *RecTy = llvm::cast<RecordTypeDeclaration>(
          getUnderlyingType(Receiver->getType()));
      return mangleName(Proc->getEnclosingDecl()) + "T" +
             llvm::Twine(getTypeId(RecTy)).str() + Proc->getName().str();
    }
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

unsigned CGModule::getTypeId(TypeDeclaration *Ty) {
  auto It = TypeIds.find(Ty);
  if (It != TypeIds.end())
    return It->second;
  unsigned Id = TypeIds.size();
  TypeIds[Ty] = Id;
  return Id;
}

llvm::GlobalVariable *
CGModule::getTypeDescriptor(RecordTypeDeclaration *Ty) {
  auto It = TypeDescriptors.find(Ty);
  if (It != TypeDescriptors.end())
    return It->second;

  llvm::SmallVector<llvm::Constant *, 8> Impls;
  for (auto *Method : collectMethodSlots(Ty)) {
    llvm::Function *Fn = M->getFunction(mangleName(Method));
    if (!Fn)
      Fn = CGProcedure(*this).declareFunction(Method);
    Impls.push_back(Fn);
  }
  llvm::ArrayType *DescTy = llvm::ArrayType::get(
      llvm::PointerType::get(getLLVMCtx(), 0), Impls.size());
  auto *Desc = new llvm::GlobalVariable(
      *M, DescTy, /*isConstant=*/true, llvm::GlobalValue::PrivateLinkage,
      llvm::ConstantArray::get(DescTy, Impls),
      llvm::Twine(mangleName(Mod)) + "T" + llvm::Twine(getTypeId(Ty)) +
          "desc");
  TypeDescriptors[Ty] = Desc;
  return Desc;
}

llvm::Constant *CGModule::getZeroValue(TypeDeclaration *Ty) {
  llvm::Type *LLVMTy = convertType(Ty);
  TypeDeclaration *Underlying = getUnderlyingType(Ty);
  if (auto *RecTy = llvm::dyn_cast<RecordTypeDeclaration>(Underlying)) {
    if (!RecTy->hasTypeTag())
      return llvm::ConstantAggregateZero::get(LLVMTy);
    llvm::SmallVector<llvm::Constant *, 8> Fields;
    Fields.push_back(getTypeDescriptor(RecTy));
    for (auto *F : RecTy->getAllFields())
      Fields.push_back(getZeroValue(F->getType()));
    return llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(LLVMTy),
                                     Fields);
  }
  if (auto *ArrTy = llvm::dyn_cast<ArrayTypeDeclaration>(Underlying)) {
    auto *ElemTy =
        llvm::dyn_cast<RecordTypeDeclaration>(getUnderlyingType(
            ArrTy->getElementType()));
    if (!ElemTy || !ElemTy->hasTypeTag())
      return llvm::ConstantAggregateZero::get(LLVMTy);
    llvm::SmallVector<llvm::Constant *, 8> Elems(
        static_cast<size_t>(ArrTy->getNumElements()),
        getZeroValue(ArrTy->getElementType()));
    return llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(LLVMTy),
                                    Elems);
  }
  if (LLVMTy->isFloatingPointTy())
    return llvm::ConstantFP::get(LLVMTy, 0.0);
  return llvm::ConstantInt::get(LLVMTy, 0);
}

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
  // First pass: declare all procedures so that calls and type descriptors can
  // reference them regardless of declaration order (this also makes
  // recursive and mutually recursive calls work).
  for (const auto &Decl : Mod->getDecls()) {
    if (auto *Proc = llvm::dyn_cast<ProcedureDeclaration>(Decl.get()))
      // Methods declared inside a record body have no implementation here.
      if (!Proc->isMethodDeclaration())
        CGProcedure(*this).declareFunction(Proc);
  }
  // Second pass: module-level variables.  Records taking part in type
  // extension store their type descriptor in the first component.
  for (const auto &Decl : Mod->getDecls()) {
    if (auto *Var = llvm::dyn_cast<VariableDeclaration>(Decl.get())) {
      llvm::Type *Ty = convertType(Var->getType());
      llvm::GlobalVariable *V = new llvm::GlobalVariable(
          *M, Ty, false,
          llvm::GlobalValue::PrivateLinkage, getZeroValue(Var->getType()),
          mangleName(Var));
      Globals[Var] = V;
    }
  }
  // Third pass: emit the procedure bodies.
  for (const auto &Decl : Mod->getDecls()) {
    if (auto *Proc = llvm::dyn_cast<ProcedureDeclaration>(Decl.get())) {
      if (Proc->isMethodDeclaration())
        continue;
      CGProcedure CGP(*this);
      CGP.run(Proc);
    }
  }
}
