// src/backend/gpu_generator.cpp
#include "gpu_generator.h"
#include "../ast.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <llvm/IR/IntrinsicEnums.inc>
#include <llvm/IR/IntrinsicsNVPTX.h>
#include <stdexcept>

GPUGenerator::GPUGenerator()
    : builder(context),
      module(std::make_unique<llvm::Module>("vectec", context)) {

  // initialize NVPTX target
  LLVMInitializeNVPTXTarget();
  LLVMInitializeNVPTXTargetInfo();
  LLVMInitializeNVPTXTargetMC();
  LLVMInitializeNVPTXAsmPrinter();

  // set target triple for NVIDIA GPU

  llvm::Triple triple("nvptx64-nvidia-cuda");
  module->setTargetTriple(triple);
}

void GPUGenerator::pushScope() {
  scopes.push_back({});
  pointerTypeScopes.push_back({});
}
void GPUGenerator::popScope() {
  scopes.pop_back();
  pointerTypeScopes.pop_back();
}

void GPUGenerator::declarePointerType(const std::string &name,
                                      llvm::Type *type) {
  pointerTypeScopes.back()[name] = type;
}

llvm::Type *GPUGenerator::getPointeeType(const std::string &name) {
  for (int i = pointerTypeScopes.size() - 1; i >= 0; i--) {
    auto it = pointerTypeScopes[i].find(name);
    if (it != pointerTypeScopes[i].end())
      return it->second;
  }
  throw std::runtime_error("Cannot determine pointee type for '" + name + "'");
}

void GPUGenerator::declareVariable(const std::string &name,
                                   llvm::AllocaInst *alloca) {
  scopes.back()[name] = alloca;
}

llvm::AllocaInst *GPUGenerator::lookupVariable(const std::string &name) {
  for (int i = scopes.size() - 1; i >= 0; i--) {
    auto it = scopes[i].find(name);
    if (it != scopes[i].end())
      return it->second;
  }
  throw std::runtime_error("Undefined variable '" + name + "'");
}

llvm::Type *GPUGenerator::getLLVMType(const std::string &type) {
  if (type == "int")
    return llvm::Type::getInt32Ty(context);
  if (type == "float")
    return llvm::Type::getFloatTy(context);
  if (type == "double")
    return llvm::Type::getDoubleTy(context);
  if (type == "bool")
    return llvm::Type::getInt1Ty(context);
  if (type == "char")
    return llvm::Type::getInt8Ty(context);
  if (type == "none")
    return llvm::Type::getVoidTy(context);

  if (type.substr(0, 7) == "ArrayOf") {
    std::string innerType = type.substr(8, type.size() - 9);
    return llvm::PointerType::get(getLLVMType(innerType), 0);
  }

  throw std::runtime_error("Unknown type '" + type + "'");
}

llvm::AllocaInst *GPUGenerator::createEntryAlloca(llvm::Function *func,
                                                  const std::string &name,
                                                  llvm::Type *type) {
  llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                               func->getEntryBlock().begin());
  return tmpBuilder.CreateAlloca(type, nullptr, name);
}

void GPUGenerator::generate(ProgramNode *program) {
  pushScope();
  for (auto &statement : program->statements) {
    generateStatement(statement.get());
  }
  popScope();

  std::string errors;
  llvm::raw_string_ostream errorStream(errors);
  if (llvm::verifyModule(*module, &errorStream)) {
    throw std::runtime_error("GPU module verification failed:\n" + errors);
  }
}

void GPUGenerator::generateStatement(ASTNode *node) {
  switch (node->type) {
  case NodeType::KernelDeclaration:
    generateKernel(static_cast<KernelDeclarationNode *>(node));
    break;
  case NodeType::VarDeclaration:
    generateVarDeclaration(static_cast<VarDeclarationNode *>(node));
    break;
  case NodeType::ReturnStatement:
    generateReturnStatement(static_cast<ReturnStatementNode *>(node));
    break;
  case NodeType::IfStatement:
    generateIfStatement(static_cast<IfStatementNode *>(node));
    break;
  case NodeType::WhileStatement:
    generateWhileStatement(static_cast<WhileStatementNode *>(node));
    break;
  case NodeType::ForStatement:
    generateForStatement(static_cast<ForStatementNode *>(node));
    break;
  case NodeType::AssignmentExpression:
    generateAssignment(static_cast<AssignmentNode *>(node));
    break;
  case NodeType::FunctionCall:
    generateExpression(node);
    break;
  default:
    throw std::runtime_error("Unknown statement in GPU IR generation");
  }
}

void GPUGenerator::generateKernel(KernelDeclarationNode *node) {
  std::vector<llvm::Type *> paramTypes;
  for (auto &[type, name] : node->params) {
    paramTypes.push_back(getLLVMType(type));
  }

  llvm::Type *returnType = getLLVMType(node->returnType);
  if (node->returnType != "none") {
    paramTypes.push_back(returnType);
  }

  llvm::FunctionType *funcType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(context), paramTypes, false);

  llvm::Function *func = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, node->name, module.get());
  // PTX_Kernel calling convention causes NVPTX to emit .entry instead of .func
  func->setCallingConv(llvm::CallingConv::PTX_Kernel);

  llvm::NamedMDNode *annotations =
      module->getOrInsertNamedMetadata("nvvm.annotations");

  llvm::Metadata *values[] = {llvm::ValueAsMetadata::get(func),
                              llvm::MDString::get(context, "kernel"),
                              llvm::ValueAsMetadata::get(llvm::ConstantInt::get(
                                  llvm::Type::getInt32Ty(context), 1))};
  annotations->addOperand(llvm::MDNode::get(context, values));

  // name the parameters
  size_t i = 0;
  for (auto &arg : func->args()) {
    if (i < node->params.size()) {
      arg.setName(node->params[i++].second);
    } else {
      arg.setName("out"); // output pointer
    }
  }

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", func);
  builder.SetInsertPoint(entry);

  currentKernel = func;
  pushScope();

  i = 0;
  for (auto &arg : func->args()) {
    if (i < node->params.size()) {
      auto &[type, name] = node->params[i++];
      llvm::AllocaInst *alloca =
          createEntryAlloca(func, name, getLLVMType(type));
      builder.CreateStore(&arg, alloca);
      declareVariable(name, alloca);
      if (type.substr(0, 7) == "ArrayOf") {
        std::string innerTypeStr = type.substr(8, type.size() - 9);
        declarePointerType(name, getLLVMType(innerTypeStr));
      }
    }
  }

  for (auto &statement : node->body) {
    generateStatement(statement.get());
  }

  if (!builder.GetInsertBlock()->getTerminator()) {
    builder.CreateRetVoid();
  }

  popScope();

  std::string errors;
  llvm::raw_string_ostream errorStream(errors);
  if (llvm::verifyFunction(*func, &errorStream)) {
    throw std::runtime_error("GPU kernel verification failed for '" +
                             node->name + "':\n" + errors);
  }
}

llvm::Value *GPUGenerator::generateThreadId() {
  // declare the NVPTX thread ID intrinsic as external function
  std::string intrinsicName = "llvm.nvvm.read.ptx.sreg.tid.x";

  llvm::Function *tidFunc = module->getFunction(intrinsicName);
  if (!tidFunc) {
    llvm::FunctionType *tidType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(context), {}, false);
    tidFunc = llvm::Function::Create(tidType, llvm::Function::ExternalLinkage,
                                     intrinsicName, module.get());
  }

  return builder.CreateCall(tidFunc, {}, "threadId");
}

llvm::Value *GPUGenerator::generateThreadCount() {
  std::string intrinsicName = "llvm.nvvm.read.ptx.sreg.ntid.x";

  llvm::Function *ntidFunc = module->getFunction(intrinsicName);
  if (!ntidFunc) {
    llvm::FunctionType *ntidType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(context), {}, false);
    ntidFunc = llvm::Function::Create(ntidType, llvm::Function::ExternalLinkage,
                                      intrinsicName, module.get());
  }

  return builder.CreateCall(ntidFunc, {}, "threadCount");
}

llvm::Value *GPUGenerator::generateBlockId() {
  std::string intrinsicName = "llvm.nvvm.read.ptx.sreg.ctaid.x";

  llvm::Function *bidFunc = module->getFunction(intrinsicName);
  if (!bidFunc) {
    llvm::FunctionType *bidType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(context), {}, false);
    bidFunc = llvm::Function::Create(bidType, llvm::Function::ExternalLinkage,
                                     intrinsicName, module.get());
  }

  return builder.CreateCall(bidFunc, {}, "blockId");
}

llvm::Value *GPUGenerator::generateBlockDim() {
  std::string intrinsicName = "llvm.nvvm.read.ptx.sreg.nctaid.x";

  llvm::Function *bdimFunc = module->getFunction(intrinsicName);
  if (!bdimFunc) {
    llvm::FunctionType *bdimType =
        llvm::FunctionType::get(llvm::Type::getInt32Ty(context), {}, false);
    bdimFunc = llvm::Function::Create(bdimType, llvm::Function::ExternalLinkage,
                                      intrinsicName, module.get());
  }

  return builder.CreateCall(bdimFunc, {}, "blockDim");
}

llvm::Value *GPUGenerator::generateFunctionCall(FunctionCallNode *node) {
  if (node->name == "threadId")
    return generateThreadId();
  if (node->name == "threadCount")
    return generateThreadCount();
  if (node->name == "blockId")
    return generateBlockId();
  if (node->name == "gridDim")
    return generateBlockDim();

  throw std::runtime_error("Unknown function in GPU kernel: '" + node->name +
                           "'");
}

void GPUGenerator::savePTX(const std::string &filename) {
  std::string targetTriple = "nvptx64-nvidia-cuda";
  std::string error;

  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(targetTriple, error);
  if (!target) {
    throw std::runtime_error("Failed to find NVPTX target: " + error);
  }

  llvm::Triple triple(targetTriple);

  llvm::TargetOptions opt;
  llvm::TargetMachine *targetMachine =
      target->createTargetMachine(triple,
                                  "sm_86", // RTX 4060 is compute capability 8.6
                                  "", opt, llvm::Reloc::PIC_);

  module->setDataLayout(targetMachine->createDataLayout());

  std::error_code ec;
  llvm::raw_fd_ostream ptxFile(filename, ec);
  if (ec) {
    throw std::runtime_error("Could not open PTX file: " + ec.message());
  }

  llvm::legacy::PassManager passManager;
  if (targetMachine->addPassesToEmitFile(passManager, ptxFile, nullptr,
                                         llvm::CodeGenFileType::AssemblyFile)) {
    throw std::runtime_error("Cannot emit PTX assembly");
  }

  passManager.run(*module);
  ptxFile.flush();

  delete targetMachine;
}

void GPUGenerator::generateVarDeclaration(VarDeclarationNode *node) {
  llvm::Type *type = getLLVMType(node->dataType);
  llvm::AllocaInst *alloca = createEntryAlloca(currentKernel, node->name, type);
  llvm::Value *value = generateExpression(node->value.get());
  builder.CreateStore(value, alloca);
  declareVariable(node->name, alloca);
}

void GPUGenerator::generateReturnStatement(ReturnStatementNode *node) {
  if (!node->value) {
    builder.CreateRetVoid();
    return;
  }

  llvm::Value *value = generateExpression(node->value.get());

  llvm::Function *func = builder.GetInsertBlock()->getParent();
  llvm::Argument *outPtr = nullptr;
  for (auto &arg : func->args()) {
    outPtr = &arg;
  }

  if (outPtr) {
    llvm::Value *tid = generateThreadId();
    llvm::Value *elemPtr =
        builder.CreateGEP(value->getType(), outPtr, tid, "outptr");
    builder.CreateStore(value, elemPtr);
  }

  builder.CreateRetVoid();
}

void GPUGenerator::generateAssignment(AssignmentNode *node) {
  llvm::AllocaInst *alloca = lookupVariable(node->name);
  llvm::Value *value = generateExpression(node->value.get());

  if (node->op != TokenType::EQUALS) {
    llvm::Value *current =
        builder.CreateLoad(alloca->getAllocatedType(), alloca, node->name);
    switch (node->op) {
    case TokenType::PLUS_EQUALS:
      value = builder.CreateAdd(current, value, "addtmp");
      break;
    case TokenType::MINUS_EQUALS:
      value = builder.CreateSub(current, value, "subtmp");
      break;
    case TokenType::STAR_EQUALS:
      value = builder.CreateMul(current, value, "multmp");
      break;
    case TokenType::SLASH_EQUALS:
      value = builder.CreateSDiv(current, value, "divtmp");
      break;
    default:
      break;
    }
  }

  builder.CreateStore(value, alloca);
}

void GPUGenerator::generateIfStatement(IfStatementNode *node) {
  llvm::Value *condition = generateExpression(node->condition.get());

  llvm::BasicBlock *thenBlock =
      llvm::BasicBlock::Create(context, "then", currentKernel);
  llvm::BasicBlock *elseBlock =
      llvm::BasicBlock::Create(context, "else", currentKernel);
  llvm::BasicBlock *mergeBlock =
      llvm::BasicBlock::Create(context, "merge", currentKernel);

  builder.CreateCondBr(condition, thenBlock, elseBlock);

  builder.SetInsertPoint(thenBlock);
  pushScope();
  for (auto &s : node->thenBlock)
    generateStatement(s.get());
  popScope();
  if (!builder.GetInsertBlock()->getTerminator())
    builder.CreateBr(mergeBlock);

  builder.SetInsertPoint(elseBlock);
  pushScope();
  for (auto &s : node->elseBlock)
    generateStatement(s.get());
  popScope();
  if (!builder.GetInsertBlock()->getTerminator())
    builder.CreateBr(mergeBlock);

  if (!mergeBlock->hasNPredecessors(0))
    builder.SetInsertPoint(mergeBlock);
}

void GPUGenerator::generateWhileStatement(WhileStatementNode *node) {
  llvm::BasicBlock *condBlock =
      llvm::BasicBlock::Create(context, "whilecond", currentKernel);
  llvm::BasicBlock *bodyBlock =
      llvm::BasicBlock::Create(context, "whilebody", currentKernel);
  llvm::BasicBlock *afterBlock =
      llvm::BasicBlock::Create(context, "whileafter", currentKernel);

  builder.CreateBr(condBlock);

  builder.SetInsertPoint(condBlock);
  llvm::Value *condition = generateExpression(node->condition.get());
  builder.CreateCondBr(condition, bodyBlock, afterBlock);

  builder.SetInsertPoint(bodyBlock);
  pushScope();
  for (auto &s : node->body)
    generateStatement(s.get());
  popScope();
  builder.CreateBr(condBlock);

  builder.SetInsertPoint(afterBlock);
}

void GPUGenerator::generateForStatement(ForStatementNode *node) {
  llvm::BasicBlock *forCond =
      llvm::BasicBlock::Create(context, "forcond", currentKernel);
  llvm::BasicBlock *forBody =
      llvm::BasicBlock::Create(context, "forbody", currentKernel);
  llvm::BasicBlock *forIncrement =
      llvm::BasicBlock::Create(context, "forincrement", currentKernel);
  llvm::BasicBlock *forAfter =
      llvm::BasicBlock::Create(context, "forafter", currentKernel);

  generateStatement(node->declaration.get());
  builder.CreateBr(forCond);

  builder.SetInsertPoint(forCond);
  llvm::Value *condition = generateExpression(node->condition.get());
  builder.CreateCondBr(condition, forBody, forAfter);

  builder.SetInsertPoint(forBody);
  pushScope();
  for (auto &s : node->body)
    generateStatement(s.get());
  popScope();
  builder.CreateBr(forIncrement);

  builder.SetInsertPoint(forIncrement);
  generateStatement(node->increment.get());
  builder.CreateBr(forCond);

  builder.SetInsertPoint(forAfter);
}

llvm::Value *GPUGenerator::generateExpression(ASTNode *node) {
  switch (node->type) {
  case NodeType::IntLiteral: {
    auto *n = static_cast<IntLiteralNode *>(node);
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), n->value,
                                  true);
  }
  case NodeType::FloatLiteral: {
    auto *n = static_cast<FloatLiteralNode *>(node);
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(context), n->value);
  }
  case NodeType::BoolLiteral: {
    auto *n = static_cast<BoolLiteralNode *>(node);
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context),
                                  n->value ? 1 : 0);
  }
  case NodeType::Identifier: {
    auto *n = static_cast<IdentifierNode *>(node);
    llvm::AllocaInst *alloca = lookupVariable(n->name);
    return builder.CreateLoad(alloca->getAllocatedType(), alloca, n->name);
  }
  case NodeType::BinaryExpression:
    return generateBinaryExpression(static_cast<BinaryExpressionNode *>(node));
  case NodeType::FunctionCall:
    return generateFunctionCall(static_cast<FunctionCallNode *>(node));
  case NodeType::ArrayAccess: {
    auto *n = static_cast<ArrayAccessNode *>(node);
    llvm::Value *index = generateExpression(n->index.get());

    if (n->array->type == NodeType::Identifier) {
      auto *ident = static_cast<IdentifierNode *>(n->array.get());
      llvm::AllocaInst *alloca = lookupVariable(ident->name);
      llvm::Value *ptr =
          builder.CreateLoad(alloca->getAllocatedType(), alloca, "arrptr");
      if (alloca->getAllocatedType()->isPointerTy()) {
        llvm::Type *elementType = getPointeeType(ident->name);
        llvm::Value *elemPtr =
            builder.CreateGEP(elementType, ptr, index, "elemptr");
        return builder.CreateLoad(elementType, elemPtr, "elemval");
      }
    }
    throw std::runtime_error(
        "Complex array access not supported in GPU kernels");
  }
  case NodeType::UnaryExpression: {
    auto *n = static_cast<UnaryExpressionNode *>(node);
    llvm::Value *val = generateExpression(n->operand.get());
    if (n->op == TokenType::MINUS) {
      if (val->getType()->isFloatingPointTy())
        return builder.CreateFNeg(val);
      return builder.CreateNeg(val);
    }
    if (n->op == TokenType::BANG)
      return builder.CreateNot(val);
    throw std::runtime_error("Unknown unary operator in GPU kernel");
  }
  default:
    throw std::runtime_error("Unknown expression in GPU IR generation");
  }
}

llvm::Value *
GPUGenerator::generateBinaryExpression(BinaryExpressionNode *node) {

  llvm::Value *left = generateExpression(node->left.get());
  llvm::Value *right = generateExpression(node->right.get());
  bool isFloat = left->getType()->isFloatingPointTy();

  switch (node->op) {
  case TokenType::PLUS:
    return isFloat ? builder.CreateFAdd(left, right, "addtmp")
                   : builder.CreateAdd(left, right, "addtmp");
  case TokenType::MINUS:
    return isFloat ? builder.CreateFSub(left, right, "subtmp")
                   : builder.CreateSub(left, right, "subtmp");
  case TokenType::STAR:
    return isFloat ? builder.CreateFMul(left, right, "multmp")
                   : builder.CreateMul(left, right, "multmp");
  case TokenType::SLASH:
    return isFloat ? builder.CreateFDiv(left, right, "divtmp")
                   : builder.CreateSDiv(left, right, "divtmp");
  case TokenType::PERCENT:
    return builder.CreateSRem(left, right, "remtmp");
  case TokenType::EQUALS_EQUALS:
    return isFloat ? builder.CreateFCmpOEQ(left, right, "eqtmp")
                   : builder.CreateICmpEQ(left, right, "eqtmp");
  case TokenType::BANG_EQUALS:
    return isFloat ? builder.CreateFCmpONE(left, right, "netmp")
                   : builder.CreateICmpNE(left, right, "netmp");
  case TokenType::LESS:
    return isFloat ? builder.CreateFCmpOLT(left, right, "lttmp")
                   : builder.CreateICmpSLT(left, right, "lttmp");
  case TokenType::GREATER:
    return isFloat ? builder.CreateFCmpOGT(left, right, "gttmp")
                   : builder.CreateICmpSGT(left, right, "gttmp");
  case TokenType::LESS_EQUALS:
    return isFloat ? builder.CreateFCmpOLE(left, right, "letmp")
                   : builder.CreateICmpSLE(left, right, "letmp");
  case TokenType::GREATER_EQUALS:
    return isFloat ? builder.CreateFCmpOGE(left, right, "getmp")
                   : builder.CreateICmpSGE(left, right, "getmp");
  case TokenType::AND_AND:
    return builder.CreateAnd(left, right, "andtmp");
  case TokenType::PIPE_PIPE:
    return builder.CreateOr(left, right, "ortmp");
  default:
    throw std::runtime_error("Unknown binary operator in GPU IR generation");
  }
}

void GPUGenerator::print() { module->print(llvm::outs(), nullptr); }
