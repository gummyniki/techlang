// ir_generator.cpp
#include "ir_generator.h"
#include "ast.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"
#include <stdexcept>

IRGenerator::IRGenerator()
    : builder(context),
      module(std::make_unique<llvm::Module>("techlang", context)) {}

void IRGenerator::pushScope() { scopes.push_back({}); }
void IRGenerator::popScope() { scopes.pop_back(); }

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
    return llvm::ArrayType::getInt8Ty(context);
  if (type == "ArrayOf(int)")
    return llvm::ArrayType::getInt32Ty(context);
  if (type == "ArrayOf(float)")
    return llvm::ArrayType::getFloatTy(context);
  if (type == "ArrayOf(double)")
    return llvm::ArrayType::getDoubleTy(context);
  if (type == "ArrayOf(bool)")
    return llvm::ArrayType::getInt1Ty(context);
  if (type == "ArrayOf(char)")
    return llvm::ArrayType::getInt8Ty(context);
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
      {llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0)}, // char* arg
      true                                                         // variadic!
  );

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
  declarePrintf();
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
  case NodeType::StructInstance:
    generateStructInstance(static_cast<StructInstanceNode *>(node));
    break;
  case NodeType::MemberAssignment:
    generateMemberAssignment(static_cast<MemberAssignmentNode *>(node));
    break;
  case NodeType::ForStatement:
    generateForStatement(static_cast<ForStatementNode *>(node));
  case NodeType::Import:
    // for now empty, std is handled manually
    break;
  default:
    throw std::runtime_error("Unknown statement in IR generation");
  }
}

void IRGenerator::generateStructDeclaration(StructDeclarationNode *node) {
  // build the LLVM struct type from field types
  std::vector<llvm::Type *> fieldTypes;
  for (auto &[type, name] : node->fields) {
    fieldTypes.push_back(getLLVMType(type));
  }

  // create a named struct type in the module
  llvm::StructType *structType =
      llvm::StructType::create(context, fieldTypes, node->name);

  // store for later lookup
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

  // find the field index
  // look up struct info by type name
  std::string structTypeName = structType->getName().str();
  auto &info = structTypes[structTypeName];
  int fieldIndex = info.getFieldIndex(node->memberName);

  if (fieldIndex < 0) {
    throw std::runtime_error("Unknown field '" + node->memberName + "'");
  }

  // GEP to the field
  llvm::Value *fieldPtr = builder.CreateGEP(
      structType, alloca,
      {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
       llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), fieldIndex)},
      "fieldptr");

  llvm::Value *value = generateExpression(node->value.get());
  builder.CreateStore(value, fieldPtr);
}

void IRGenerator::generateFunctionDeclaration(FunctionDeclarationNode *node) {
  // build the function signature
  std::vector<llvm::Type *> paramTypes;
  for (auto &[type, name] : node->params) {
    paramTypes.push_back(getLLVMType(type));
  }

  llvm::Type *returnType = getLLVMType(node->returnType);
  llvm::FunctionType *funcType =
      llvm::FunctionType::get(returnType, paramTypes, false);
  llvm::Function *func = llvm::Function::Create(
      funcType, llvm::Function::ExternalLinkage, node->name, module.get());

  // name the parameters
  size_t i = 0;
  for (auto &arg : func->args()) {
    arg.setName(node->params[i++].second);
  }

  // create the entry basic block
  llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", func);
  builder.SetInsertPoint(entry);

  currentFunction = func;
  pushScope();

  // alloca each parameter so they can be modified locally
  i = 0;
  for (auto &arg : func->args()) {
    auto &[type, name] = node->params[i++];
    llvm::AllocaInst *alloca = createEntryAlloca(func, name, getLLVMType(type));
    builder.CreateStore(&arg, alloca);
    declareVariable(name, alloca);
  }

  // generate body
  for (auto &statement : node->body) {
    generateStatement(statement.get());
  }

  // if function returns none and no explicit return, add one
  if (node->returnType == "none") {
    builder.CreateRetVoid();
  }

  popScope();

  if (!builder.GetInsertBlock()->getTerminator()) {
    if (node->returnType == "none") {
      builder.CreateRetVoid();
    } else {
      // function missing return statement — add a default
      builder.CreateRet(
          llvm::Constant::getNullValue(getLLVMType(node->returnType)));
    }
  }

  // verify the function is well-formed
  std::string errors;
  llvm::raw_string_ostream errorStream(errors);
  if (llvm::verifyFunction(*func, &errorStream)) {
    throw std::runtime_error("LLVM function verification failed for '" +
                             node->name + "':\n" + errors);
  }
}

void IRGenerator::generateVarDeclaration(VarDeclarationNode *node) {
  if (node->dataType.substr(0, 7) == "ArrayOf") {
    // generateExpression for ArrayLiteral already creates an alloca
    // and returns a GEP pointer to it — we need the alloca itself
    // so instead, we generate the elements and build the array here directly

    auto *literalNode = static_cast<ArrayLiteralNode *>(node->value.get());
    if (literalNode->elements.empty()) {
      throw std::runtime_error("Cannot declare empty array");
    }

    // generate first element to get type
    llvm::Value *firstVal = generateExpression(literalNode->elements[0].get());
    llvm::Type *elementType = firstVal->getType();
    int size = literalNode->elements.size();

    // create the array alloca directly
    llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, size);
    llvm::AllocaInst *arrayAlloca =
        createEntryAlloca(currentFunction, node->name, arrayType);

    // store first element
    llvm::Value *gep0 = builder.CreateGEP(
        arrayType, arrayAlloca,
        {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
         llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)},
        "elemptr");
    builder.CreateStore(firstVal, gep0);

    // store remaining elements
    for (int i = 1; i < size; i++) {
      llvm::Value *val = generateExpression(literalNode->elements[i].get());
      llvm::Value *gep = builder.CreateGEP(
          arrayType, arrayAlloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), i)},
          "elemptr");
      builder.CreateStore(val, gep);
    }

    // declare the variable pointing to the actual array alloca
    declareVariable(node->name, arrayAlloca);
    return;
  }

  // normal scalar
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

  // handle += -= *= /=
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

  // create the three blocks
  llvm::BasicBlock *thenBlock =
      llvm::BasicBlock::Create(context, "then", currentFunction);
  llvm::BasicBlock *elseBlock =
      llvm::BasicBlock::Create(context, "else", currentFunction);
  llvm::BasicBlock *mergeBlock =
      llvm::BasicBlock::Create(context, "merge", currentFunction);

  builder.CreateCondBr(condition, thenBlock, elseBlock);

  // then block
  builder.SetInsertPoint(thenBlock);
  pushScope();
  for (auto &s : node->thenBlock)
    generateStatement(s.get());
  popScope();
  builder.CreateBr(mergeBlock); // jump to merge when done

  // else block
  builder.SetInsertPoint(elseBlock);
  pushScope();
  for (auto &s : node->elseBlock)
    generateStatement(s.get());
  popScope();
  builder.CreateBr(mergeBlock);

  // continue after the if
  builder.SetInsertPoint(mergeBlock);
}

void IRGenerator::generateWhileStatement(WhileStatementNode *node) {
  llvm::BasicBlock *condBlock =
      llvm::BasicBlock::Create(context, "whilecond", currentFunction);
  llvm::BasicBlock *bodyBlock =
      llvm::BasicBlock::Create(context, "whilebody", currentFunction);
  llvm::BasicBlock *afterBlock =
      llvm::BasicBlock::Create(context, "whileafter", currentFunction);

  builder.CreateBr(condBlock); // jump into condition check

  // condition
  builder.SetInsertPoint(condBlock);
  llvm::Value *condition = generateExpression(node->condition.get());
  builder.CreateCondBr(condition, bodyBlock, afterBlock);

  // body
  builder.SetInsertPoint(bodyBlock);
  pushScope();
  for (auto &s : node->body)
    generateStatement(s.get());
  popScope();
  builder.CreateBr(condBlock); // loop back to condition

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

  builder.CreateBr(forCond); // jump into condition check

  // condition
  builder.SetInsertPoint(forCond);
  llvm::Value *condition = generateExpression(node->condition.get());
  builder.CreateCondBr(condition, forBody, forafter);

  // body
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

  case NodeType::ArrayLiteral: {
    auto *n = static_cast<ArrayLiteralNode *>(node);

    if (n->elements.empty()) {
      throw std::runtime_error("Cannot create empty array literal");
    }

    // generate first element to figure out the type
    llvm::Value *firstVal = generateExpression(n->elements[0].get());
    llvm::Type *elementType = firstVal->getType();
    int size = n->elements.size();

    // allocate stack space for the whole array
    llvm::ArrayType *arrayType = llvm::ArrayType::get(elementType, size);
    llvm::AllocaInst *arrayAlloca =
        createEntryAlloca(currentFunction, "arraytmp", arrayType);

    // store each element at its index
    auto storeElement = [&](int index, llvm::Value *val) {
      llvm::Value *gep = builder.CreateGEP(
          arrayType, arrayAlloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
           llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), index)},
          "elemptr");
      builder.CreateStore(val, gep);
    };

    // store first element (already generated)
    storeElement(0, firstVal);

    // generate and store remaining elements
    for (int i = 1; i < size; i++) {
      llvm::Value *val = generateExpression(n->elements[i].get());
      storeElement(i, val);
    }

    // return a pointer to the start of the array
    return builder.CreateGEP(
        arrayType, arrayAlloca,
        {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0),
         llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0)},
        "arrayptr");
  }
  case NodeType::ArrayAccess: {
    auto *n = static_cast<ArrayAccessNode *>(node);
    llvm::Value *index = generateExpression(n->index.get());

    // get the identifier name to look up the alloca directly
    if (n->array->type == NodeType::Identifier) {
      auto *ident = static_cast<IdentifierNode *>(n->array.get());
      llvm::AllocaInst *alloca = lookupVariable(ident->name);

      // alloca's allocated type is the array type e.g. [3 x i32]
      llvm::ArrayType *arrayType =
          static_cast<llvm::ArrayType *>(alloca->getAllocatedType());
      llvm::Type *elementType = arrayType->getElementType();

      // GEP into the array
      llvm::Value *elemPtr = builder.CreateGEP(
          arrayType, alloca,
          {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), index},
          "elemptr");

      return builder.CreateLoad(elementType, elemPtr, "elemval");
    }

    throw std::runtime_error("Complex array access not supported yet");
  }
  case NodeType::StringLiteral: {
    auto *n = static_cast<StringLiteralNode *>(node);
    return builder.CreateGlobalStringPtr(n->value, "str");
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
  case NodeType::MemberAccess: {
    auto *n = static_cast<MemberAccessNode *>(node);

    // get the object
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

  // comparisons
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

llvm::Value *IRGenerator::generatePrint(FunctionCallNode *node) {
  llvm::Function *printfFunc = module->getFunction("printf");

  if (node->args.empty()) {
    // print a newline if no args
    llvm::Value *fmt = builder.CreateGlobalStringPtr("\n", "fmt");
    return builder.CreateCall(printfFunc, {fmt}, "printtmp");
  }

  llvm::Value *arg = generateExpression(node->args[0].get());
  llvm::Type *argType = arg->getType();

  llvm::Value *fmt;

  // pick format string based on type
  if (argType->isIntegerTy(32)) {
    fmt = builder.CreateGlobalStringPtr("%d\n", "fmt");
  } else if (argType->isFloatTy()) {
    // printf needs double for %f, so promote float to double
    arg = builder.CreateFPExt(arg, llvm::Type::getDoubleTy(context));
    fmt = builder.CreateGlobalStringPtr("%f\n", "fmt");
  } else if (argType->isDoubleTy()) {
    fmt = builder.CreateGlobalStringPtr("%f\n", "fmt");
  } else if (argType->isIntegerTy(1)) {
    // bool: print "true" or "false"
    // we need a conditional here
    llvm::Value *trueStr = builder.CreateGlobalStringPtr("true\n", "truestr");
    llvm::Value *falseStr =
        builder.CreateGlobalStringPtr("false\n", "falsestr");
    llvm::Value *str = builder.CreateSelect(arg, trueStr, falseStr, "boolstr");
    fmt = builder.CreateGlobalStringPtr("%s", "fmt");
    return builder.CreateCall(printfFunc, {fmt, str}, "printtmp");
  } else if (argType->isIntegerTy(8)) {
    // char
    fmt = builder.CreateGlobalStringPtr("%c\n", "fmt");
  } else {
    // string (i8*)
    fmt = builder.CreateGlobalStringPtr("%s\n", "fmt");
  }

  return builder.CreateCall(printfFunc, {fmt, arg}, "printtmp");
}

llvm::Value *IRGenerator::generateFunctionCall(FunctionCallNode *node) {
  // std functions

  if (node->name == "std.print") {
    return generatePrint(node);
  }

  llvm::Function *func = module->getFunction(node->name);
  if (!func) {
    throw std::runtime_error("Undefined function '" + node->name + "'");
  }

  std::vector<llvm::Value *> args;
  for (auto &arg : node->args) {
    args.push_back(generateExpression(arg.get()));
  }

  return builder.CreateCall(func, args, "calltmp");
}
