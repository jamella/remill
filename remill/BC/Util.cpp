/* Copyright 2015 Peter Goodman (peter@trailofbits.com), all rights reserved. */

#include <glog/logging.h>

#include <sstream>
#include <system_error>

#include <sys/stat.h>
#include <unistd.h>

#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/ToolOutputFile.h>

#include "remill/BC/Util.h"

namespace remill {

llvm::Function *&BlockMap::operator[](uintptr_t pc) {
  return this->std::unordered_map<uintptr_t, llvm::Function *>::operator[](pc);
}

llvm::Function *BlockMap::operator[](uintptr_t pc) const {
  const auto block_it = this->find(pc);
  if (this->end() == block_it) {
    LOG(WARNING) << "No block associated with PC " << pc;
    return nullptr;
  } else {
    return block_it->second;
  }
}

// Initialize the attributes for a lifted function.
void InitFunctionAttributes(llvm::Function *function) {

  // Make sure functions are treated as if they return. LLVM doesn't like
  // mixing must-tail-calls with no-return.
  function->removeFnAttr(llvm::Attribute::NoReturn);

  // Don't use any exception stuff.
  function->addFnAttr(llvm::Attribute::NoUnwind);
  function->removeFnAttr(llvm::Attribute::UWTable);

  // To use must-tail-calls everywhere we need to use the `fast` calling
  // convention, where it's up the LLVM to decide how to pass arguments.
  function->setCallingConv(llvm::CallingConv::Fast);

  // Mark everything for inlining, but don't require it.
  function->addFnAttr(llvm::Attribute::InlineHint);

  // Mark everything as naked, even though we don't really want this down the
  // line (because it changes codegen), and these functions are assumed to only
  // use assembly. This inhibits certain buggy optimizations (e.g. deadargelim).
  function->addFnAttr(llvm::Attribute::Naked);
}

// Create a tail-call from one lifted function to another.
void AddTerminatingTailCall(llvm::Function *source_func,
                            llvm::Function *dest_func,
                            uintptr_t addr) {
  if (source_func->isDeclaration()) {
    std::stringstream ss;
    ss << "0x" << std::hex << addr;
    llvm::BasicBlock::Create(source_func->getContext(), ss.str(), source_func);
  }
  AddTerminatingTailCall(&(source_func->back()), dest_func, addr);
}

void AddTerminatingTailCall(llvm::BasicBlock *source_block,
                            llvm::Function *target_func,
                            uintptr_t addr) {
  LOG_IF(FATAL, !target_func)
      << "Target function/block does not exist!";

  auto target_func_type = target_func->getFunctionType();
  LOG_IF(FATAL, 2 != target_func_type->getNumParams())
      << "Expected one argument for call to: "
      << (target_func ? target_func->getName().str() : "<unreachable>");

  auto addr_arg = target_func_type->getParamType(1);
  if (auto addr_type = llvm::dyn_cast<llvm::IntegerType>(addr_arg)) {
    AddTerminatingTailCall(source_block, target_func,
                           llvm::ConstantInt::get(addr_type, addr, false));

  } else {
    LOG(FATAL)
        << "Expected second parameter to function "
        << target_func->getName().str() << " to be an integral type.";
  }
}

void AddTerminatingTailCall(llvm::BasicBlock *source_block,
                            llvm::Function *target_func,
                            llvm::Value *addr) {
  LOG_IF(FATAL, !target_func)
      << "Target function/block does not exist!";

  LOG_IF(ERROR, source_block->getTerminator() ||
                source_block->getTerminatingMustTailCall())
      << "Block already has a terminator; not adding fall-through call to: "
      << (target_func ? target_func->getName().str() : "<unreachable>");

  LOG_IF(FATAL, 2 != target_func->getFunctionType()->getNumParams())
      << "Expected two arguments for call to: "
      << (target_func ? target_func->getName().str() : "<unreachable>");

  llvm::IRBuilder<> ir(source_block);
  llvm::Function *source_func = source_block->getParent();
  std::vector<llvm::Value *> args;
  args.push_back(FindStatePointer(source_func));
  args.push_back(addr);
  llvm::CallInst *call_target_instr = ir.CreateCall(target_func, args);
  call_target_instr->setAttributes(target_func->getAttributes());

  // Make sure we tail-call from one block method to another.
  call_target_instr->setTailCallKind(llvm::CallInst::TCK_MustTail);
  call_target_instr->setCallingConv(llvm::CallingConv::Fast);
  ir.CreateRetVoid();
}

// Find a local variable defined in the entry block of the function. We use
// this to find register variables.
llvm::Value *FindVarInFunction(llvm::Function *function, std::string name,
                               bool allow_failure) {
  for (auto &instr : function->getEntryBlock()) {
    if (instr.getName() == name) {
      return &instr;
    }
  }
  LOG_IF(FATAL, !allow_failure)
    << "Could not find variable " << name << " in function "
    << function->getName().str();
  return nullptr;
}

// Find the machine state pointer.
llvm::Value *FindStatePointer(llvm::Function *function) {
  if (2 != function->arg_size()) {
    LOG(FATAL)
        << "Invalid block-like function. Expected two arguments: state "
        << "pointer and program counter in function "
        << function->getName().str();
  }
  return &*function->getArgumentList().begin();
}

// Find a function with name `name` in the module `M`.
llvm::Function *FindFunction(const llvm::Module *module, std::string name) {
  return module->getFunction(name);
}

// Find a global variable with name `name` in the module `M`.
llvm::GlobalVariable *FindGlobaVariable(const llvm::Module *module,
                                        std::string name) {
  return module->getGlobalVariable(name);
}

// Reads an LLVM module from a file.
llvm::Module *LoadModuleFromFile(std::string file_name) {
  llvm::SMDiagnostic err;
  auto mod_ptr = llvm::parseIRFile(file_name, err, llvm::getGlobalContext());
  auto module = mod_ptr.get();
  mod_ptr.release();

  CHECK(nullptr != module)
      << "Unable to parse module file: " << file_name;

  module->materializeAll();  // Just in case.
  return module;
}

// Store an LLVM module into a file.
void StoreModuleToFile(llvm::Module *module, std::string file_name) {
  std::string error;
  llvm::raw_string_ostream error_stream(error);

  if (llvm::verifyModule(*module, &error_stream)) {
    LOG(FATAL)
        << "Error writing module to file " << file_name << ". " << error;
  }

  std::error_code ec;
  llvm::tool_output_file bc(file_name.c_str(), ec, llvm::sys::fs::F_RW);

  CHECK(!ec)
      << "Unable to open output bitcode file for writing: " << file_name;

  llvm::WriteBitcodeToFile(module, bc.os());
  bc.keep();

  CHECK(!ec)
      << "Error writing bitcode to file: " << file_name;
}

}  // namespace remill
