#include "ir_generator.h"
#include "../ast.h"
#include "../frontend/token.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <stdexcept>

IRGenerator::IRGenerator()
    : builder(context),
      module(std::make_unique<llvm::Module>("techlang", context)) {}

void IRGenerator::pushScope() {
  scopes.push_back({});
  pointerTypeScopes.push_back({});
}
void IRGenerator::popScope() {
  scopes.pop_back();
  pointerTypeScopes.pop_back();
}

void IRGenerator::declareVariable(const std::string &name,
                                  llvm::AllocaInst *alloca) {
  scopes.back()[name] = alloca;
}

llvm::AllocaInst *IRGenerator::lookupVariable(const std::string &name) {
  for (int i = scopes.size() - 1; i >= 0; i--) {
    auto it = scopes[i].find(name);
    if (it != scopes[i].end())
      return it->second;
  }
  throw std::runtime_error("Undefined variable '" + name + "'");
}

// convert Techlang type strings to LLVM types
llvm::Type *IRGenerator::getLLVMType(const std::string &type) {
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
  if (type == "string")
    return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
  if (type == "any")
    return llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);

  if (type.substr(0, 7) == "ArrayOf") {
    std::string innerType = type.substr(8, type.size() - 9);
    return llvm::PointerType::get(getLLVMType(innerType), 0);
  }

  if (type.substr(0, 9) == "PointerOf") {
    std::string innerType = type.substr(10, type.size() - 11);
    return llvm::PointerType::get(getLLVMType(innerType), 0);
  }

  auto it = structTypes.find(type);
  if (it != structTypes.end()) {
    return it->second.llvmType;
  }

  if (type == "none")
    return llvm::Type::getVoidTy(context);
  throw std::runtime_error("Unknown type '" + type + "'");
}

// always allocate variables in the entry block of the function
// this makes LLVM's optimizer much happier
llvm::AllocaInst *IRGenerator::createEntryAlloca(llvm::Function *func,
                                                 const std::string &name,
                                                 llvm::Type *type) {
  llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(),
                               func->getEntryBlock().begin());
  return tmpBuilder.CreateAlloca(type, nullptr, name);
}

void IRGenerator::declarePrintf() {
  llvm::FunctionType *printfType = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(context), // returns int
      {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)}, true);

  module->getOrInsertFunction("printf", printfType);
}

void IRGenerator::print() { module->print(llvm::outs(), nullptr); }

void IRGenerator::saveToFile(const std::string &filename) {
  std::error_code ec;
  llvm::raw_fd_ostream file(filename, ec);
  if (ec)
    throw std::runtime_error("Could not open file: " + filename);
  module->print(file, nullptr);
}

void IRGenerator::generate(ProgramNode *program) {
  declareStdFunctions();
  pushScope();
  for (auto &statement : program->statements) {
    generateStatement(statement.get());
  }
  popScope();

  std::string errors;
  llvm::raw_string_ostream errorStream(errors);
  if (llvm::verifyModule(*module, &errorStream)) {
    throw std::runtime_error("LLVM module verification failed:\n" + errors);
  }
}

void IRGenerator::generateStatement(ASTNode *node) {
  switch (node->type) {
  case NodeType::FunctionDeclaration:
    generateFunctionDeclaration(static_cast<FunctionDeclarationNode *>(node));
    break;
  case NodeType::VarDeclaration:
    generateVarDeclaration(static_cast<VarDeclarationNode *>(node));
    break;
  case NodeType::IfStatement:
    generateIfStatement(static_cast<IfStatementNode *>(node));
    break;
  case NodeType::WhileStatement:
    generateWhileStatement(static_cast<WhileStatementNode *>(node));
    break;
  case NodeType::ReturnStatement:
    generateReturnStatement(static_cast<ReturnStatementNode *>(node));
    break;
  case NodeType::AssignmentExpression:
    generateAssignment(static_cast<AssignmentNode *>(node));
    break;
  case NodeType::FunctionCall:
    generateExpression(node); // call but discard return value
    break;
  case NodeType::StructDeclaration:
    generateStructDeclaration(static_cast<StructDeclarationNode *>(node));
    break;
  case NodeType::EnumDeclaration:
    generateEnumDeclaration(static_cast<EnumDeclarationNode *>(node));
    break;
  case NodeType::StructInstance:
    generateStructInstance(static_cast<StructInstanceNode *>(node));
    break;
  case NodeType::MemberAssignment:
    generateMemberAssignment(static_cast<MemberAssignmentNode *>(node));
    break;
  case NodeType::ForStatement:
    generateForStatement(static_cast<ForStatementNode *>(node));
    break;
  case NodeType::Import:
    // for now empty
    break;
  default:
    throw std::runtime_error("Unknown statement in IR generation");
  }
}

void IRGenerator::generateStructDeclaration(StructDeclarationNode *node) {
  std::vector<llvm::Type *> fieldTypes;
  for (auto &[type, name] : node->fields) {
    fieldTypes.push_back(getLLVMType(type));
  }

  llvm::StructType *structType =
      llvm::StructType::create(context, fieldTypes, node->name);

  StructInfo info;
  info.llvmType = structType;
  info.fields = node->fields;
  structTypes[node->name] = info;
}

void IRGenerator::generateStructInstance(StructInstanceNode *node) {
  if (!structTypes.count(node->structType)) {
    throw std::runtime_error("Unknown struct type '" + node->structType + "'");
  }

  auto &info = structTypes[node->structType];

  // alloca the whole struct on the stack
  llvm::AllocaInst *alloca =
      createEntryAlloca(currentFunction, node->name, info.llvmType);

  // zero-initialize all fields to their base values
  for (int i = 0; i < info.fields.size(); i++) {
    llvm::Value *gep = builder.CreateGEP(
        info.llvmType, alloca,
        {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
         llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), i)},
        "fieldptr");
    llvm::Type *fieldType = info.llvmType->getElementType(i);
    builder.CreateStore(llvm::Constant::getNullValue(fieldType), gep);
  }

  declareVariable(node->name, alloca);
}

void IRGenerator::generateMemberAssignment(MemberAssignmentNode *node) {
  llvm::AllocaInst *alloca = lookupVariable(node->objectName);
  llvm::StructType *structType =
      static_cast<llvm::StructType *>(alloca->getAllocatedType());

  std::string structTypeName = structType->getName().str();
  auto &info = structTypes[structTypeName];
  int fieldIndex = info.getFieldIndex(node->memberName);

  if (fieldIndex < 0) {
    throw std::runtime_error("Unknown field '" + node->memberName + "'");
  }

  llvm::Value *fieldPtr = builder.CreateGEP(
      structType, alloca,
      {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
       llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), fieldIndex)},
      "fieldptr");

  llvm::Value *value = generateExpression(node->value.get());
  builder.CreateStore(value, fieldPtr);
}

void IRGenerator::declarePointerType(const std::string &name,
                                     llvm::Type *type) {
  pointerTypeScopes.back()[name] = type;
}

llvm::Type *IRGenerator::getPointeeType(const std::string &name) {
  for (int i = pointerTypeScopes.size() - 1; i >= 0; i--) {
    auto it = pointerTypeScopes[i].find(name);
    if (it != pointerTypeScopes[i].end())
      return it->second;
  }
  throw std::runtime_error("Cannot determine pointee type for '" + name + "'");
}

void IRGenerator::generateFunctionDeclaration(FunctionDeclarationNode *node) {
  std::vector<llvm::Type *> paramTypes;
  for (auto &[type, name] : node->params) {
    paramTypes.push_back(getLLVMType(type));
  }

  llvm::Type *returnType = getLLVMType(node->returnType);
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);

  if (node->body.empty() && !node->externSymbol.empty()) {
    auto callee = module->getOrInsertFunction(node->externSymbol, funcType);
    externFunctions[node->name] =
        llvm::cast<llvm::Function>(callee.getCallee());
    return;
  }

  llvm::Function *func = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, node->name, module.get());

  if (node->body.empty()) {
    module->getOrInsertFunction(node->name, funcType);
    return;
  }

  size_t i = 0;
  for (auto &arg : func->args()) {
    arg.setName(node->params[i++].second);
  }

  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", func);
  builder.SetInsertPoint(entry);

  currentFunction = func;
  pushScope();

  i = 0;
  for (auto &arg : func->args()) {
    auto &[type, name] = node->params[i++];
    llvm::AllocaInst *alloca = createEntryAlloca(func, name, getLLVMType(type));
    builder.CreateStore(&arg, alloca);
    declareVariable(name, alloca);
    if (type.substr(0, 9) == "PointerOf") {
      std::string innerTypeStr = type.substr(10, type.size() - 11);
      declarePointerType(name, getLLVMType(innerTypeStr));
    }
    if (type.substr(0, 7) == "ArrayOf") {
      std::string innerTypeStr = type.substr(8, type.size() - 9);
      declarePointerType(name, getLLVMType(innerTypeStr));
    }
  }

  for (auto &statement : node->body) {
    generateStatement(statement.get());
  }

  if (node->returnType == "none") {
    builder.CreateRetVoid();
  }

  popScope();

  if (!builder.GetInsertBlock()->getTerminator()) {
    if (node->returnType == "none") {
      builder.CreateRetVoid();
    } else {
      builder.CreateRet(
          llvm::Constant::getNullValue(getLLVMType(node->returnType)));
    }
  }

  std::string errors;
  llvm::raw_string_ostream errorStream(errors);
  if (llvm::verifyFunction(*func, &errorStream)) {
    throw std::runtime_error("LLVM function verification failed for '" +
                             node->name + "':\n" + errors);
  }
}

void IRGenerator::generateVarDeclaration(VarDeclarationNode *node) {
  if (node->dataType.substr(0, 9) == "PointerOf") {
    std::string innerTypeStr =
        node->dataType.substr(10, node->dataType.size() - 11);
    llvm::Type *pointeeType = getLLVMType(innerTypeStr);
    llvm::Type *ptrType = llvm::PointerType::get(pointeeType, 0);

    llvm::AllocaInst *alloca =
        createEntryAlloca(currentFunction, node->name, ptrType);
    llvm::Value *value = generateExpression(node->value.get());
    builder.CreateStore(value, alloca);
    declareVariable(node->name, alloca);

    //  remember what this pointer points to
    declarePointerType(node->name, pointeeType);
    return;
  }

  if (node->dataType.substr(0, 7) == "ArrayOf") {

    auto *literalNode = static_cast<ArrayLiteralNode *>(node->value.get());
    if (literalNode->elements.empty()) {
      throw std::runtime_error("Cannot declare empty array");
    }

    llvm::Value *firstVal = generateExpression(literalNode->elements[0].get());
    llvm::Type *elementType = firstVal->getType();
    int size = literalNode->elements.size();

    llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, size);
    llvm::AllocaInst *arrayAlloca =
        createEntryAlloca(currentFunction, node->name, arrayType);

    llvm::Value *gep0 = builder.CreateGEP(
        arrayType, arrayAlloca,
        {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
         llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)},
        "elemptr");
    builder.CreateStore(firstVal, gep0);

    for (int i = 1; i < size; i++) {
      llvm::Value *val = generateExpression(literalNode->elements[i].get());
      llvm::Value *gep = builder.CreateGEP(
          arrayType, arrayAlloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), i)},
          "elemptr");
      builder.CreateStore(val, gep);
    }

    declareVariable(node->name, arrayAlloca);
    return;
  }

  llvm::Type *type = getLLVMType(node->dataType);
  llvm::AllocaInst *alloca =
      createEntryAlloca(currentFunction, node->name, type);
  llvm::Value *value = generateExpression(node->value.get());
  builder.CreateStore(value, alloca);
  declareVariable(node->name, alloca);
}

void IRGenerator::generateAssignment(AssignmentNode *node) {
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

void IRGenerator::generateReturnStatement(ReturnStatementNode *node) {
  if (!node->value) {
    builder.CreateRetVoid();
  } else {
    llvm::Value *value = generateExpression(node->value.get());
    builder.CreateRet(value);
  }
}

void IRGenerator::generateIfStatement(IfStatementNode *node) {
  llvm::Value *condition = generateExpression(node->condition.get());

  llvm::BasicBlock *thenBlock =
      llvm::BasicBlock::Create(context, "then", currentFunction);
  llvm::BasicBlock *elseBlock =
      llvm::BasicBlock::Create(context, "else", currentFunction);
  llvm::BasicBlock *mergeBlock =
      llvm::BasicBlock::Create(context, "merge", currentFunction);

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
void IRGenerator::generateWhileStatement(WhileStatementNode *node) {
  llvm::BasicBlock *condBlock =
      llvm::BasicBlock::Create(context, "whilecond", currentFunction);
  llvm::BasicBlock *bodyBlock =
      llvm::BasicBlock::Create(context, "whilebody", currentFunction);
  llvm::BasicBlock *afterBlock =
      llvm::BasicBlock::Create(context, "whileafter", currentFunction);

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

void IRGenerator::generateForStatement(ForStatementNode *node) {
  llvm::BasicBlock *forCond =
      llvm::BasicBlock::Create(context, "forcond", currentFunction);
  llvm::BasicBlock *forBody =
      llvm::BasicBlock::Create(context, "forbody", currentFunction);
  llvm::BasicBlock *forIncrement =
      llvm::BasicBlock::Create(context, "forincrement", currentFunction);
  llvm::BasicBlock *forafter =
      llvm::BasicBlock::Create(context, "forafter", currentFunction);

  generateStatement(node->declaration.get());

  builder.CreateBr(forCond);

  builder.SetInsertPoint(forCond);
  llvm::Value *condition = generateExpression(node->condition.get());
  builder.CreateCondBr(condition, forBody, forafter);

  builder.SetInsertPoint(forBody);
  pushScope();
  for (auto &s : node->body)
    generateStatement(s.get());
  popScope();

  builder.CreateBr(forIncrement);

  builder.SetInsertPoint(forIncrement);
  generateStatement(node->increment.get());

  builder.CreateBr(forCond);

  builder.SetInsertPoint(forafter);
}

void IRGenerator::generateEnumDeclaration(EnumDeclarationNode *node) {
  for (auto &entry : node->entries) {
    enumConstants[entry.first] = llvm::ConstantInt::get(
        llvm::Type::getInt32Ty(context), entry.second, true);
  }
}

llvm::Value *IRGenerator::generateExpression(ASTNode *node) {
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
  case NodeType::CharLiteral: {
    auto *n = static_cast<CharLiteralNode *>(node);
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(context), n->value);
  }

  case NodeType::ArrayLiteral: {
    auto *n = static_cast<ArrayLiteralNode *>(node);

    if (n->elements.empty()) {
      throw std::runtime_error("Cannot create empty array literal");
    }

    llvm::Value *firstVal = generateExpression(n->elements[0].get());
    llvm::Type *elementType = firstVal->getType();
    int size = n->elements.size();

    llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, size);
    llvm::AllocaInst *arrayAlloca =
        createEntryAlloca(currentFunction, "arraytmp", arrayType);

    auto storeElement = [&](int index, llvm::Value *val) {
      llvm::Value *gep = builder.CreateGEP(
          arrayType, arrayAlloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), index)},
          "elemptr");
      builder.CreateStore(val, gep);
    };

    storeElement(0, firstVal);

    for (int i = 1; i < size; i++) {
      llvm::Value *val = generateExpression(n->elements[i].get());
      storeElement(i, val);
    }

    return builder.CreateGEP(
        arrayType, arrayAlloca,
        {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
         llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)},
        "arrayptr");
  }
  case NodeType::ArrayAccess: {
    auto *n = static_cast<ArrayAccessNode *>(node);
    llvm::Value *index = generateExpression(n->index.get());

    if (n->array->type == NodeType::Identifier) {
      auto *ident = static_cast<IdentifierNode *>(n->array.get());
      llvm::AllocaInst *alloca = lookupVariable(ident->name);

      if (alloca->getAllocatedType()->isArrayTy()) {
        llvm::ArrayType *arrayType =
            static_cast<llvm::ArrayType *>(alloca->getAllocatedType());
        llvm::Type *elementType = arrayType->getElementType();

        llvm::Value *elemPtr = builder.CreateGEP(
            arrayType, alloca,
            {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), index},
            "elemptr");
        return builder.CreateLoad(elementType, elemPtr, "elemval");
      } else {
        llvm::Value *ptr =
            builder.CreateLoad(alloca->getAllocatedType(), alloca, "arrptr");
        llvm::Type *elementType = getPointeeType(ident->name);

        llvm::Value *elemPtr =
            builder.CreateGEP(elementType, ptr, index, "elemptr");
        return builder.CreateLoad(elementType, elemPtr, "elemval");
      }
    }
  }

  case NodeType::Cast: {
    auto *n = static_cast<CastNode *>(node);
    llvm::Value *val = generateExpression(n->value.get());
    llvm::Type *targetType = getLLVMType(n->targetType);

    llvm::Type *srcType = val->getType();

    if (srcType->isPointerTy() && !targetType->isPointerTy()) {
      return builder.CreateLoad(targetType, val, "anycasttmp");
    }

    if (!srcType->isPointerTy() && targetType->isPointerTy()) {
      llvm::AllocaInst *alloca =
          createEntryAlloca(currentFunction, "anybox", srcType);
      builder.CreateStore(val, alloca);
      return builder.CreateBitCast(alloca, targetType, "anyboxtmp");
    }

    if (srcType->isIntegerTy() && targetType->isFloatingPointTy()) {
      return builder.CreateSIToFP(val, targetType, "casttmp");
    }

    if (srcType->isFloatingPointTy() && targetType->isIntegerTy()) {
      return builder.CreateFPToSI(val, targetType, "casttmp");
    }

    if (srcType->isFloatingPointTy() && targetType->isFloatingPointTy()) {
      if (srcType->isFloatTy() && targetType->isDoubleTy()) {
        return builder.CreateFPExt(val, targetType, "casttmp");
      } else {
        return builder.CreateFPTrunc(val, targetType, "casttmp");
      }
    }

    if (srcType->isIntegerTy() && targetType->isIntegerTy()) {
      if (srcType->getIntegerBitWidth() > targetType->getIntegerBitWidth()) {
        return builder.CreateTrunc(val, targetType, "casttmp");
      } else {
        return builder.CreateSExt(val, targetType, "casttmp");
      }
    }

    throw std::runtime_error("Unsupported cast from " + n->targetType);
  }

    throw std::runtime_error("Complex array access not supported yet");

  case NodeType::StringLiteral: {
    auto *n = static_cast<StringLiteralNode *>(node);
    return builder.CreateGlobalStringPtr(n->value, "str");
  }
  case NodeType::Identifier: {
    auto *n = static_cast<IdentifierNode *>(node);

    // check enum constants first
    auto it = enumConstants.find(n->name);
    if (it != enumConstants.end()) {
      return it->second;
    }

    llvm::AllocaInst *alloca = lookupVariable(n->name);

    if (alloca->getAllocatedType()->isArrayTy()) {
      return builder.CreateGEP(
          alloca->getAllocatedType(), alloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)},
          "arrayptr");
    }

    return builder.CreateLoad(alloca->getAllocatedType(), alloca, n->name);
  }
  case NodeType::BinaryExpression:
    return generateBinaryExpression(static_cast<BinaryExpressionNode *>(node));
  case NodeType::FunctionCall:
    return generateFunctionCall(static_cast<FunctionCallNode *>(node));
  case NodeType::MemberAccess: {
    auto *n = static_cast<MemberAccessNode *>(node);

    if (n->object->type == NodeType::Identifier) {
      auto *ident = static_cast<IdentifierNode *>(n->object.get());
      llvm::AllocaInst *alloca = lookupVariable(ident->name);
      llvm::StructType *structType =
          static_cast<llvm::StructType *>(alloca->getAllocatedType());

      std::string structTypeName = structType->getName().str();
      auto &info = structTypes[structTypeName];
      int fieldIndex = info.getFieldIndex(n->member);

      llvm::Value *fieldPtr = builder.CreateGEP(
          structType, alloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), fieldIndex)},
          "fieldptr");

      llvm::Type *fieldType = structType->getElementType(fieldIndex);
      return builder.CreateLoad(fieldType, fieldPtr, "fieldval");
    }

    throw std::runtime_error("Complex member access not supported yet");
  }

  case NodeType::UnaryExpression: {
    auto *n = static_cast<UnaryExpressionNode *>(node);

    llvm::Value *val = generateExpression(n->operand.get());

    bool isInt = val->getType()->isIntegerTy();
    bool isFloat = val->getType()->isFloatingPointTy();

    if (n->op == TokenType::MINUS) {

      if (isInt) {
        return builder.CreateNeg(val);
      } else if (isFloat) {
        return builder.CreateFNeg(val);
      }
    }

    if (n->op == TokenType::BANG) {
      return builder.CreateNot(val);
    }
  }

  default:
    throw std::runtime_error("Unknown expression in IR generation");
  }
}

llvm::Value *IRGenerator::generateBinaryExpression(BinaryExpressionNode *node) {
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
    throw std::runtime_error("Unknown binary operator in IR generation");
  }
}

llvm::Value *IRGenerator::generateFunctionCall(FunctionCallNode *node) {
  // hardcoded std functions for now
  // TODO: find a way to connect the std.tec functions to C implementations
  // somehow

  if (node->name == "std.print") {
    // generate the argument
    llvm::Value *arg = generateExpression(node->args[0].get());
    llvm::Type *argType = arg->getType();

    // pick the right print function based on type
    std::string printFunc;
    if (argType->isIntegerTy(32))
      printFunc = "tec_print_int";
    else if (argType->isFloatTy())
      printFunc = "tec_print_float";
    else if (argType->isDoubleTy())
      printFunc = "tec_print_double";
    else if (argType->isIntegerTy(1))
      printFunc = "tec_print_bool";
    else if (argType->isIntegerTy(8))
      printFunc = "tec_print_char";
    else
      printFunc = "tec_print_string";

    llvm::Function *func = module->getFunction(printFunc);
    if (!func) {
      throw std::runtime_error("Print function not found: " + printFunc);
    }

    bool isVoid = func->getReturnType()->isVoidTy();
    return builder.CreateCall(func, {arg}, isVoid ? "" : "calltmp");
  }

  if (node->name == "std.addressOf") {
    auto *ident = static_cast<IdentifierNode *>(node->args[0].get());
    llvm::AllocaInst *alloca = lookupVariable(ident->name);

    return alloca;
  }

  if (node->name == "std.deref") {
    auto *ident = static_cast<IdentifierNode *>(node->args[0].get());
    llvm::AllocaInst *alloca = lookupVariable(ident->name);

    llvm::Value *ptr =
        builder.CreateLoad(alloca->getAllocatedType(), alloca, "ptrval");

    llvm::Type *pointeeType = getPointeeType(ident->name);
    return builder.CreateLoad(pointeeType, ptr, "deref");
  }

  if (node->name == "std.storeAt") {
    auto *ident = static_cast<IdentifierNode *>(node->args[0].get());
    llvm::AllocaInst *alloca = lookupVariable(ident->name);

    llvm::Value *ptr =
        builder.CreateLoad(alloca->getAllocatedType(), alloca, "ptrval");

    llvm::Value *val = generateExpression(node->args[1].get());
    builder.CreateStore(val, ptr);
    return llvm::Constant::getNullValue(llvm::Type::getInt32Ty(context));
  }

  llvm::Function *func = module->getFunction(node->name);
  if (!func) {
    auto it = externFunctions.find(node->name);
    if (it != externFunctions.end())
      func = it->second;
  }
  if (!func) {
    throw std::runtime_error("Undefined function '" + node->name + "'");
  }

  std::vector<llvm::Value *> args;
  int i = 0;
  for (auto &arg : node->args) {
    llvm::Value *val = generateExpression(arg.get());

    // auto-box if function expects ptr (any) but got concrete value
    if (i < func->getFunctionType()->getNumParams()) {
      llvm::Type *expectedType = func->getFunctionType()->getParamType(i);
      llvm::Type *actualType = val->getType();

      if (expectedType->isPointerTy() && !actualType->isPointerTy()) {
        llvm::AllocaInst *alloca =
            createEntryAlloca(currentFunction, "anybox", actualType);
        builder.CreateStore(val, alloca);
        val = builder.CreateBitCast(
            alloca, llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0),
            "anyptr");
      }
    }

    args.push_back(val);
    i++;
  }

  bool isVoid = func->getReturnType()->isVoidTy();
  return builder.CreateCall(func, args, isVoid ? "" : "calltmp");
}

void IRGenerator::declareStdFunctions() { declarePrintf(); }

llvm::Value *IRGenerator::generateExit(FunctionCallNode *node) {
  llvm::Function *exitFunc = module->getFunction("exit");
  llvm::Value *code = generateExpression(node->args[0].get());
  return builder.CreateCall(exitFunc, {code});
}

llvm::Value *IRGenerator::generateReadInt(FunctionCallNode *node) {
  llvm::Function *readIntFunc = module->getFunction("tec_readInt");
  return builder.CreateCall(readIntFunc, {}, "readtmp");
}

llvm::Value *IRGenerator::generateSqrt(FunctionCallNode *node) {
  llvm::Function *sqrtFunc = module->getFunction("tec_sqrt");
  llvm::Value *arg = generateExpression(node->args[0].get());
  return builder.CreateCall(sqrtFunc, {arg}, "sqrttmp");
}
