#pragma once
#include "../ast.h"
#include "../symbol_table.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include <memory>
#include <unordered_map>

struct StructInfo {
  llvm::StructType *llvmType;
  std::vector<std::pair<std::string, std::string>> fields;

  int getFieldIndex(const std::string &name) const {
    for (int i = 0; i < fields.size(); i++) {
      if (fields[i].second == name)
        return i;
    }
    return -1;
  }
};

// address of an lvalue expression, plus enough type info to keep resolving
// a member-access chain through it (structs are concrete LLVM types, but
// pointer/array fields are opaque `ptr` at the LLVM level, so pointeeType
// carries what a `.value` on this slot would point to, if applicable)
struct LValueResult {
  llvm::Value *addr;
  llvm::Type *type;
  llvm::Type *pointeeType = nullptr;
};

class IRGenerator {
public:
  IRGenerator();

  void generate(ProgramNode *program);
  void print();
  void saveToFile(const std::string &filename);
  llvm::Module *getModule() { return module.get(); }
  std::vector<std::string> gpuAliases;
  void registerGPUAlias(const std::string &alias);

private:
  llvm::LLVMContext context;
  llvm::IRBuilder<> builder;
  std::unique_ptr<llvm::Module> module;

  std::vector<std::unordered_map<std::string, llvm::AllocaInst *>> scopes;
  std::vector<std::unordered_map<std::string, llvm::Type *>> pointerTypeScopes;
  std::unordered_map<std::string, StructInfo> structTypes;
  std::unordered_map<std::string, llvm::ConstantInt *> enumConstants;
  std::unordered_map<std::string, llvm::Function *> externFunctions;

  llvm::Function *currentFunction = nullptr;

  // scope management
  void pushScope();
  void popScope();
  void declareVariable(const std::string &name, llvm::AllocaInst *alloca);
  llvm::AllocaInst *lookupVariable(const std::string &name);

  // type conversion
  llvm::Type *getLLVMType(const std::string &type);
  llvm::Type *getPointeeType(const std::string &name);
  llvm::Type *tryGetPointeeType(const std::string &name);

  void declarePointerType(const std::string &name, llvm::Type *type);

  // resolves the storage address of an (possibly chained) lvalue expression,
  // e.g. `p.value.width` or `a.b.c` — walks Identifier/MemberAccess nodes
  LValueResult resolveLValue(ASTNode *node);

  // generation methods
  void generateStatement(ASTNode *node);
  void generateVarDeclaration(VarDeclarationNode *node);
  void generateFunctionDeclaration(FunctionDeclarationNode *node);
  void generateIfStatement(IfStatementNode *node);
  void generateWhileStatement(WhileStatementNode *node);
  void generateForStatement(ForStatementNode *node);
  void generateReturnStatement(ReturnStatementNode *node);
  void generateAssignment(AssignmentNode *node);
  void generateEnumDeclaration(EnumDeclarationNode *node);

  void generateStructInstance(StructInstanceNode *node);
  void generateStructDeclaration(StructDeclarationNode *node);
  void generateMemberAssignment(MemberAssignmentNode *node);

  llvm::Value *generateExpression(ASTNode *node);
  llvm::Value *generateBinaryExpression(BinaryExpressionNode *node);
  llvm::Value *generateFunctionCall(FunctionCallNode *node);

  llvm::AllocaInst *createEntryAlloca(llvm::Function *func,
                                      const std::string &name,
                                      llvm::Type *type);

  // widen/truncate an integer value to match a differently-sized integer
  // target type (e.g. an int32 literal passed where int64 is expected) —
  // leaves non-integer or already-matching types untouched
  llvm::Value *coerceIntWidth(llvm::Value *val, llvm::Type *targetType);

  void generateArrayAssignment(ArrayAssignmentNode *node);

  // this is unused im pretty sure
  void declarePrintf();
  llvm::Value *generatePrint(FunctionCallNode *node);
  void declareStdFunctions();
  llvm::Value *generateExit(FunctionCallNode *node);
  llvm::Value *generateReadInt(FunctionCallNode *node);
  llvm::Value *generateSqrt(FunctionCallNode *node);
};
