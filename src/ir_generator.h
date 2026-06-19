// ir_generator.h
#pragma once
#include "ast.h"
#include "symbol_table.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include <memory>
#include <unordered_map>

struct StructInfo {
  llvm::StructType *llvmType;
  std::vector<std::pair<std::string, std::string>> fields; // (type, name)

  int getFieldIndex(const std::string &name) const {
    for (int i = 0; i < fields.size(); i++) {
      if (fields[i].second == name)
        return i;
    }
    return -1;
  }
};

class IRGenerator {
public:
  IRGenerator();

  void generate(ProgramNode *program);
  void print();                                 // print the IR to stdout
  void saveToFile(const std::string &filename); // save to .ll file
  llvm::Module *getModule() { return module.get(); }

private:
  // the three core LLVM objects you always need
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> module;

  // maps variable names to their alloca'd stack slots
  std::vector<std::unordered_map<std::string, llvm::AllocaInst *>> scopes;
  std::unordered_map<std::string, StructInfo> structTypes;

  // current function being generated
  llvm::Function *currentFunction = nullptr;

  // scope management
  void pushScope();
  void popScope();
  void declareVariable(const std::string &name, llvm::AllocaInst *alloca);
  llvm::AllocaInst *lookupVariable(const std::string &name);

  // type conversion
  llvm::Type *getLLVMType(const std::string &type);

  // generation methods
  void generateStatement(ASTNode *node);
  void generateVarDeclaration(VarDeclarationNode *node);
  void generateFunctionDeclaration(FunctionDeclarationNode *node);
  void generateIfStatement(IfStatementNode *node);
  void generateWhileStatement(WhileStatementNode *node);
  void generateForStatement(ForStatementNode *node);
  void generateReturnStatement(ReturnStatementNode *node);
  void generateAssignment(AssignmentNode *node);

  void generateStructInstance(StructInstanceNode *node);
  void generateStructDeclaration(StructDeclarationNode *node);
  void generateMemberAssignment(MemberAssignmentNode *node);

  llvm::Value *generateExpression(ASTNode *node);
  llvm::Value *generateBinaryExpression(BinaryExpressionNode *node);
  llvm::Value *generateFunctionCall(FunctionCallNode *node);

  // helper: alloca always in function entry block (LLVM best practice)
  llvm::AllocaInst *createEntryAlloca(llvm::Function *func,
                                      const std::string &name,
                                      llvm::Type *type);

  // std functions
  void declarePrintf();
  llvm::Value *generatePrint(FunctionCallNode *node);
};
