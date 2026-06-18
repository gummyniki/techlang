#pragma once

#include "ast.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

class CodeGen {
public:
  llvm::LLVMContext context;
  llvm::Module module;
  llvm::IRBuilder<> builder;

  CodeGen();

  void generate(ASTNode *root);

  llvm::Value *genExpr(ASTNode *node);
  void genStmt(ASTNode *node);
  llvm::Type *toLLVMType(const std::string &type);
};
