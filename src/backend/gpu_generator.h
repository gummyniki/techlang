#pragma once
#include "../ast.h"
#include "../frontend/token.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class GPUGenerator {
public:
  GPUGenerator();
  void generate(ProgramNode *program);
  void savePTX(const std::string &filename);
  void print();

private:
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> module;
  llvm::Function *currentKernel = nullptr;

  // scope management — same as IRGenerator
  std::vector<std::unordered_map<std::string, llvm::AllocaInst *>> scopes;
  std::vector<std::unordered_map<std::string, llvm::Type *>> pointerTypeScopes;
  void pushScope();
  void popScope();
  void declareVariable(const std::string &name, llvm::AllocaInst *alloca);
  llvm::AllocaInst *lookupVariable(const std::string &name);
  void declarePointerType(const std::string &name, llvm::Type *type);
  llvm::Type *getPointeeType(const std::string &name);

  // type conversion — same as IRGenerator
  llvm::Type *getLLVMType(const std::string &type);
  llvm::AllocaInst *createEntryAlloca(llvm::Function *func,
                                      const std::string &name,
                                      llvm::Type *type);

  // generation methods
  void generateStatement(ASTNode *node);
  void generateKernel(KernelDeclarationNode *node);
  void generateVarDeclaration(VarDeclarationNode *node);
  void generateReturnStatement(ReturnStatementNode *node);
  void generateIfStatement(IfStatementNode *node);
  void generateWhileStatement(WhileStatementNode *node);
  void generateForStatement(ForStatementNode *node);
  void generateAssignment(AssignmentNode *node);

  llvm::Value *generateExpression(ASTNode *node);
  llvm::Value *generateBinaryExpression(BinaryExpressionNode *node);
  llvm::Value *generateFunctionCall(FunctionCallNode *node);

  // GPU built-ins
  llvm::Value *generateThreadId();
  llvm::Value *generateThreadCount();
  llvm::Value *generateBlockId();
  llvm::Value *generateBlockDim();
};
