#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

Module* makeLLVMModule();

int main(int argc, char**argv) {
  Module* Mod = makeLLVMModule();

  verifyModule(*Mod, PrintMessageAction);

  PassManager PM;
  PM.add(createPrintModulePass(&outs()));
  PM.run(*Mod);

  delete Mod;
  return 0;
}

Module* makeLLVMModule() {
  auto& context = llvm::getGlobalContext();
  // Module Construction
  Module* mod = new Module("code generation test", context);
  IRBuilder<> main_builder(context);

  // Declare i32 @puts(i8*)
  std::vector<Type*> putsArgs { 
    main_builder.getInt8PtrTy()
  };

  FunctionType *putsType = llvm::FunctionType::get(main_builder.getInt32Ty(), putsArgs, false);
  Function *putsFunc = llvm::Function::Create(putsType, llvm::Function::ExternalLinkage, "puts", mod);

  // Declare i32 @main(i32, i8**)
  std::vector<Type*> mainArgs = {
    main_builder.getInt32Ty(),
    main_builder.getInt8PtrTy()->getPointerTo(),
  };

  FunctionType *funcType = llvm::FunctionType::get(main_builder.getInt32Ty(), mainArgs, false);
  Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", mod);
  BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
  main_builder.SetInsertPoint(entry);
  mainFunc->setCallingConv(CallingConv::C);
  mainFunc->addFnAttr(Attribute::StackProtect);


  Value *hello  = main_builder.CreateGlobalStringPtr("hello world!\n");
  //Value* retval = main_builder.CreateAlloca(Type::getInt32Ty(context), 0, "retval");
  Value* retval = main_builder.CreateCall(putsFunc, {hello});

  main_builder.CreateRet(retval);
  
  return mod;
}
