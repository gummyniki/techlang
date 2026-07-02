#include "semantic_analyzer.h"
#include "../ast.h"
#include "../debug/error.h"
#include "llvm/ADT/STLExtras.h"
#include <stdexcept>

void SemanticAnalyzer::analyze(ProgramNode *program) {
  // push the global scope
  symbols.pushScope();

  for (auto &statement : program->statements) {
    if (statement->type == NodeType::FunctionDeclaration) {
      auto *func = static_cast<FunctionDeclarationNode *>(statement.get());

      if (func->name == "main")
        continue;

      Symbol funcSymbol;
      funcSymbol.name = func->name;
      funcSymbol.type = func->returnType;
      funcSymbol.isFunction = true;
      funcSymbol.returnType = func->returnType;
      for (auto &param : func->params) {
        funcSymbol.paramTypes.push_back(param.first);
      }
      symbols.declare(funcSymbol);
    }
  }

  for (auto &statement : program->statements) {
    analyzeStatement(statement.get());
  }

  symbols.popScope();
}

void SemanticAnalyzer::analyzeStatement(ASTNode *node) {
  switch (node->type) {

  case NodeType::Import:
    // dont do anything for now
    break;
  case NodeType::VarDeclaration:
    analyzeVarDeclaration(static_cast<VarDeclarationNode *>(node));
    break;
  case NodeType::FunctionDeclaration:
    analyzeFunctionDeclaration(static_cast<FunctionDeclarationNode *>(node));
    break;
  case NodeType::IfStatement:
    analyzeIfStatement(static_cast<IfStatementNode *>(node));
    break;
  case NodeType::WhileStatement:
    analyzeWhileStatement(static_cast<WhileStatementNode *>(node));
    break;
  case NodeType::ForStatement:
    analyzeForStatement(static_cast<ForStatementNode *>(node));
    break;
  case NodeType::ReturnStatement:
    analyzeReturnStatement(static_cast<ReturnStatementNode *>(node));
    break;
  case NodeType::StructDeclaration:
    analyzeStructDeclaration(static_cast<StructDeclarationNode *>(node));
    break;
  case NodeType::StructInstance:
    analyzeStructInstance(static_cast<StructInstanceNode *>(node));
    break;
  case NodeType::MemberAssignment:
    analyzeMemberAssignment(static_cast<MemberAssignmentNode *>(node));
    break;
  case NodeType::ArrayAssignment:
    analyzeArrayAssignment(static_cast<ArrayAssignmentNode *>(node));
    break;
  case NodeType::EnumDeclaration:
    analyzeEnumDeclaration(static_cast<EnumDeclarationNode *>(node));
    break;
  case NodeType::AssignmentExpression: {
    auto *n = static_cast<AssignmentNode *>(node);
    auto symbol = symbols.lookup(n->name);
    if (!symbol) {
      throw CompileError("undefined variable '" + n->name + "'", n->line);
    }
    if (symbol->isConst) {
      throw CompileError("cannot assign to const variable '" + n->name + "'",
                         n->line);
    }
    std::string valueType = analyzeExpression(n->value.get());
    if (!typesAreCompatible(symbol->type, valueType)) {
      throw CompileError("cannot assign " + valueType +
                             " to variable of type " + symbol->type,
                         n->line);
    }
    break;
  }
  case NodeType::FunctionCall: {
    auto *n = static_cast<FunctionCallNode *>(node);
    if (n->name == "std.print" || n->name == "std.exit") {
      for (auto &arg : n->args) {
        analyzeExpression(arg.get());
      }
      break;
    }
    analyzeExpression(node);
    break;
  }

  default:
    throw std::runtime_error("Unknown statement type");
  }
}

void SemanticAnalyzer::analyzeEnumDeclaration(EnumDeclarationNode *node) {
  if (enumTable.count(node->name)) {
    throw CompileError("enum '" + node->name + "' already defined", node->line);
  }

  EnumDefinition def;
  def.name = node->name;
  for (auto &[name, value] : node->entries) {
    def.values[name] = value;
  }
  enumTable[node->name] = def;
}

void SemanticAnalyzer::analyzeVarDeclaration(VarDeclarationNode *node) {
  std::string valueType = analyzeExpression(node->value.get());

  if (!typesAreCompatible(node->dataType, valueType)) {
    throw CompileError("cannot assign " + valueType + " to variable of type " +
                           node->dataType,
                       node->line);
  }

  Symbol symbol;
  symbol.name = node->name;
  symbol.type = node->dataType;
  symbol.isConst = node->isConst;
  symbols.declare(symbol);
}

void SemanticAnalyzer::analyzeFunctionDeclaration(
    FunctionDeclarationNode *node) {

  if (!symbols.lookup(node->name)) {
    Symbol funcSymbol;
    funcSymbol.name = node->name;
    funcSymbol.type = node->returnType;
    funcSymbol.isFunction = true;
    funcSymbol.returnType = node->returnType;
    for (auto &param : node->params) {
      funcSymbol.paramTypes.push_back(param.first);
    }
    symbols.declare(funcSymbol);
  }

  // only analyze body if it exists
  if (node->body.empty())
    return;

  symbols.pushScope();

  for (auto &[type, name] : node->params) {
    Symbol paramSymbol;
    paramSymbol.name = name;
    paramSymbol.type = type;
    symbols.declare(paramSymbol);
  }

  std::string previousReturnType = currentFunctionReturnType;
  currentFunctionReturnType = node->returnType;

  analyzeBlock(node->body);

  currentFunctionReturnType = previousReturnType;

  symbols.popScope();
}
std::string SemanticAnalyzer::analyzeExpression(ASTNode *node) {
  switch (node->type) {
  case NodeType::IntLiteral:
    return "int";
  case NodeType::FloatLiteral:
    return "float";
  case NodeType::StringLiteral:
    return "string";
  case NodeType::BoolLiteral:
    return "bool";
  case NodeType::CharLiteral:
    return "char";

  case NodeType::Identifier: {
    auto *n = static_cast<IdentifierNode *>(node);
    auto symbol = symbols.lookup(n->name);
    if (!symbol) {
      for (auto &[enumName, def] : enumTable) {
        if (def.getValue(n->name)) {
          return "int";
        }
      }
      throw CompileError("undefined variable '" + n->name + "'", n->line);
    }
    return symbol->type;
  }

  case NodeType::BinaryExpression:
    return analyzeBinaryExpression(static_cast<BinaryExpressionNode *>(node));

  case NodeType::UnaryExpression:
    return analyzeUnaryExpression(static_cast<UnaryExpressionNode *>(node));

  case NodeType::FunctionCall:
    return analyzeFunctionCall(static_cast<FunctionCallNode *>(node));

  case NodeType::MemberAccess: {
    auto *n = static_cast<MemberAccessNode *>(node);
    std::string objectType = analyzeExpression(n->object.get());

    if (n->member == "length" && objectType.substr(0, 7) == "ArrayOf") {
      return "int";
    }

    if (n->member == "length") {
      if (objectType == "string")
        return "int";
      if (objectType.substr(0, 7) == "ArrayOf")
        return "int";
    }

    if (n->member == "value") {
      if (objectType.substr(0, 9) != "PointerOf") {
        throw std::runtime_error("Line " + std::to_string(n->line) +
                                 ": '.value' is only valid on pointers, got " +
                                 objectType);
      }
      return objectType.substr(10, objectType.size() - 11); // inner type
    }

    if (n->member == "address") {
      return "PointerOf(" + objectType + ")";
    }

    if (!structTable.count(objectType)) {
      throw CompileError("'" + objectType + "' is not a struct type", n->line);
    }

    auto fieldType = structTable[objectType].getFieldType(n->member);
    if (!fieldType) {
      throw CompileError("struct '" + objectType + "' has no field '" +
                             n->member + "'",
                         n->line);
    }

    return *fieldType;
  }

  case NodeType::ArrayLiteral: {
    auto *n = static_cast<ArrayLiteralNode *>(node);

    if (n->elements.empty()) {
      return "ArrayOf(unknown)";
    }

    std::string elementType = analyzeExpression(n->elements[0].get());

    for (size_t i = 1; i < n->elements.size(); i++) {
      std::string t = analyzeExpression(n->elements[i].get());
      if (!typesAreCompatible(elementType, t)) {
        throw CompileError("array elements must be the same type, got " +
                               elementType + " and " + t,
                           n->line);
      }
    }

    return "ArrayOf(" + elementType + ")";
  }

  case NodeType::ArrayAccess: {
    auto *n = static_cast<ArrayAccessNode *>(node);
    std::string arrayType = analyzeExpression(n->array.get());
    std::string indexType = analyzeExpression(n->index.get());
    bool indexIsInt = indexType == "int" || indexType == "int8" ||
                      indexType == "int16" || indexType == "int32" ||
                      indexType == "int64" || indexType == "uint8" ||
                      indexType == "uint16" || indexType == "uint32" ||
                      indexType == "uint64";
    if (!indexIsInt) {
      throw CompileError("array index must be int, got " + indexType, n->line);
    }
    if (arrayType.substr(0, 7) == "ArrayOf") {
      return arrayType.substr(8, arrayType.size() - 9);
    }
    // indexing through a raw pointer (e.g. a malloc'd buffer) behaves like
    // indexing an array of its pointee type - matches the IR generator,
    // which already walks pointer-typed variables the same way as arrays
    if (arrayType.substr(0, 9) == "PointerOf") {
      return arrayType.substr(10, arrayType.size() - 11);
    }
    return "unknown";
  }

  case NodeType::Cast: {
    auto *n = static_cast<CastNode *>(node);
    std::string fromType = analyzeExpression(n->value.get());

    bool fromNumeric = (fromType == "int" || fromType == "float" ||
                        fromType == "double" || fromType == "char");
    bool toNumeric = (n->targetType == "int" || n->targetType == "float" ||
                      n->targetType == "double" || n->targetType == "char");

    bool isAny = (fromType == "any");

    if (!isAny) {

      if (!fromNumeric || !toNumeric) {
        throw std::runtime_error("Line " + std::to_string(n->line) +
                                 ": cannot cast " + fromType + " to " +
                                 n->targetType);
      }
    }

    return n->targetType;
  }

  default:
    throw std::runtime_error("Unknown expression type");
  }
}

std::string
SemanticAnalyzer::analyzeBinaryExpression(BinaryExpressionNode *node) {
  std::string leftType = analyzeExpression(node->left.get());
  std::string rightType = analyzeExpression(node->right.get());

  if (node->op == TokenType::EQUALS_EQUALS ||
      node->op == TokenType::BANG_EQUALS || node->op == TokenType::LESS ||
      node->op == TokenType::GREATER || node->op == TokenType::LESS_EQUALS ||
      node->op == TokenType::GREATER_EQUALS || node->op == TokenType::AND_AND ||
      node->op == TokenType::PIPE_PIPE) {
    return "bool";
  }

  return binaryResultType(leftType, rightType, node->op);
}

std::string
SemanticAnalyzer::analyzeUnaryExpression(UnaryExpressionNode *node) {
  std::string operandType = analyzeExpression(node->operand.get());

  if (node->op == TokenType::BANG) {
    if (operandType != "bool") {
      throw CompileError("'!' operator requires bool, got " + operandType,
                         node->line);
    }
    return "bool";
  }

  if (node->op == TokenType::MINUS) {
    if (operandType != "int" && operandType != "float" &&
        operandType != "double") {
      throw CompileError("unary '-' requires numeric type, got " + operandType,
                         node->line);
    }
    return operandType;
  }

  return operandType;
}

std::string SemanticAnalyzer::analyzeFunctionCall(FunctionCallNode *node) {

  // hardcoded std functions for now :(
  // TODO: make this be an actual tec function that connects to C somehow

  if (node->name == "std.addressOf") {
    // get the type of the argument and wrap it in PointerOf
    std::string innerType = analyzeExpression(node->args[0].get());
    return "PointerOf(" + innerType + ")";
  }
  if (node->name == "std.deref") {
    // unwrap the PointerOf
    std::string ptrType = analyzeExpression(node->args[0].get());
    if (ptrType.substr(0, 9) != "PointerOf") {
      throw std::runtime_error("Line " + std::to_string(node->line) +
                               ": std.deref requires a pointer type, got " +
                               ptrType);
    }
    return ptrType.substr(10, ptrType.size() - 11);
  }
  if (node->name == "std.storeAt") {
    analyzeExpression(node->args[0].get());
    analyzeExpression(node->args[1].get());
    return "none";
  }

  auto symbol = symbols.lookup(node->name);
  std::cout << "Name: " << node->name << "\n";
  if (!symbol) {
    throw CompileError("undefined function '" + node->name + "'", node->line);
  }
  if (!symbol->isFunction) {
    throw CompileError("'" + node->name + "' is not a function", node->line);
  }

  if (node->args.size() != symbol->paramTypes.size()) {
    throw CompileError("function '" + node->name + "' expects " +
                           std::to_string(symbol->paramTypes.size()) +
                           " arguments, got " +
                           std::to_string(node->args.size()),
                       node->line);
  }

  for (size_t i = 0; i < node->args.size(); i++) {
    std::string argType = analyzeExpression(node->args[i].get());
    if (!typesAreCompatible(symbol->paramTypes[i], argType)) {
      throw CompileError("argument " + std::to_string(i + 1) + " of '" +
                             node->name + "' expects " + symbol->paramTypes[i] +
                             ", got " + argType,
                         node->line);
    }
  }

  return symbol->returnType;
}

bool SemanticAnalyzer::typesAreCompatible(const std::string &a,
                                          const std::string &b) {
  if (a == b)
    return true;

  if (a == "any" || b == "any")
    return true;

  bool aIsNumeric =
      (a == "int" || a == "int8" || a == "int16" || a == "int32" ||
       a == "int64" || a == "uint8" || a == "uint16" || a == "uint32" ||
       a == "uint64" || a == "float" || a == "double");
  bool bIsNumeric =
      (b == "int" || b == "int8" || b == "int16" || b == "int32" ||
       b == "int64" || b == "uint8" || b == "uint16" || b == "uint32" ||
       b == "uint64" || b == "float" || b == "double");

  if (aIsNumeric && bIsNumeric)
    return true;

  if (a.substr(0, 7) == "ArrayOf" && b.substr(0, 7) == "ArrayOf") {
    std::string innerA = a.substr(8, a.size() - 9);
    std::string innerB = b.substr(8, b.size() - 9);
    return typesAreCompatible(innerA, innerB);
  }

  if (a.substr(0, 9) == "PointerOf" && b.substr(0, 9) == "PointerOf") {
    std::string innerA = a.substr(10, a.size() - 11);
    std::string innerB = a.substr(10, b.size() - 11);

    return typesAreCompatible(innerA, innerB);
  }

  return false;
}

void SemanticAnalyzer::analyzeStructDeclaration(StructDeclarationNode *node) {
  if (structTable.count(node->name)) {
    throw CompileError("struct '" + node->name + "' already defined",
                       node->line);
  }

  StructDefinition def;
  def.name = node->name;
  def.fields = node->fields;
  structTable[node->name] = def;
}

void SemanticAnalyzer::analyzeStructInstance(StructInstanceNode *node) {
  if (!structTable.count(node->structType)) {
    throw CompileError("unknown struct type '" + node->structType + "'",
                       node->line);
  }

  Symbol symbol;
  symbol.name = node->name;
  symbol.type = node->structType;
  symbols.declare(symbol);
}

void SemanticAnalyzer::analyzeMemberAssignment(MemberAssignmentNode *node) {
  std::string objectType = analyzeExpression(node->object.get());

  if (node->memberName == "value") {
    if (objectType.substr(0, 9) != "PointerOf") {
      throw std::runtime_error("Line " + std::to_string(node->line) +
                               ": '.value' is only valid on pointers, got " +
                               objectType);
    }
    std::string innerType = objectType.substr(10, objectType.size() - 11);
    std::string valueType = analyzeExpression(node->value.get());
    if (!typesAreCompatible(innerType, valueType)) {
      throw std::runtime_error("Line " + std::to_string(node->line) +
                               ": cannot assign " + valueType +
                               " to pointer value of type " + innerType);
    }
    return;
  }

  if (!structTable.count(objectType)) {
    throw CompileError("'" + objectType + "' is not a struct type", node->line);
  }

  auto &def = structTable[objectType];
  auto fieldType = def.getFieldType(node->memberName);
  if (!fieldType) {
    throw CompileError("struct '" + objectType + "' has no field '" +
                           node->memberName + "'",
                       node->line);
  }

  std::string valueType = analyzeExpression(node->value.get());
  if (!typesAreCompatible(*fieldType, valueType)) {
    throw CompileError("cannot assign " + valueType + " to field '" +
                           node->memberName + "' of type " + *fieldType,
                       node->line);
  }
}

void SemanticAnalyzer::analyzeArrayAssignment(ArrayAssignmentNode *node) {
  auto symbol = symbols.lookup(node->arrayName);
  if (!symbol) {
    throw CompileError("undefined variable '" + node->arrayName + "'",
                       node->line);
  }

  const std::string &arrayType = symbol->type;
  std::string elementType;
  if (arrayType.substr(0, 7) == "ArrayOf") {
    elementType = arrayType.substr(8, arrayType.size() - 9);
  } else if (arrayType.substr(0, 9) == "PointerOf") {
    elementType = arrayType.substr(10, arrayType.size() - 11);
  } else {
    throw CompileError("'" + node->arrayName + "' is not an array or pointer",
                       node->line);
  }

  std::string indexType = analyzeExpression(node->index.get());
  bool indexIsInt = indexType == "int" || indexType == "int8" ||
                    indexType == "int16" || indexType == "int32" ||
                    indexType == "int64" || indexType == "uint8" ||
                    indexType == "uint16" || indexType == "uint32" ||
                    indexType == "uint64";
  if (!indexIsInt) {
    throw CompileError("array index must be int, got " + indexType,
                       node->line);
  }

  std::string valueType = analyzeExpression(node->value.get());
  if (!typesAreCompatible(elementType, valueType)) {
    throw CompileError("cannot assign " + valueType +
                           " to array element of type " + elementType,
                       node->line);
  }
}

std::string SemanticAnalyzer::binaryResultType(const std::string &left,
                                               const std::string &right,
                                               TokenType op) {
  auto isIntType = [](const std::string &t) {
    return t == "int" || t == "int8" || t == "int16" || t == "int32" ||
           t == "int64" || t == "uint8" || t == "uint16" || t == "uint32" ||
           t == "uint64";
  };
  bool leftIsNumeric = isIntType(left) || left == "float" || left == "double";
  bool rightIsNumeric =
      isIntType(right) || right == "float" || right == "double";

  if (leftIsNumeric && rightIsNumeric) {
    if (left == "double" || right == "double")
      return "double";
    if (left == "float" || right == "float")
      return "float";
    // widest integer type wins (matches the implicit widening IR
    // generation performs when the two sides' bit widths differ)
    if (left == "int64" || left == "uint64")
      return left;
    if (right == "int64" || right == "uint64")
      return right;
    if (left != "int" && isIntType(left))
      return left;
    if (right != "int" && isIntType(right))
      return right;
    return "int";
  }

  if (left == "string" && right == "string" && op == TokenType::PLUS) {
    return "string";
  }

  throw std::runtime_error("Type error: cannot apply operator to " + left +
                           " and " + right);
}

void SemanticAnalyzer::analyzeReturnStatement(ReturnStatementNode *node) {
  if (currentFunctionReturnType == "none") {
    if (node->value) {
      throw CompileError(
          "cannot return a value from a function that returns none",
          node->line);
    }
    return;
  }

  if (!node->value) {
    throw CompileError("expected a return value of type " +
                           currentFunctionReturnType,
                       node->line);
  }

  std::string returnType = analyzeExpression(node->value.get());
  if (!typesAreCompatible(currentFunctionReturnType, returnType)) {
    throw CompileError("function returns " + currentFunctionReturnType +
                           " but got " + returnType,
                       node->line);
  }
}

void SemanticAnalyzer::analyzeBlock(
    std::vector<std::unique_ptr<ASTNode>> &statements) {
  symbols.pushScope();
  for (auto &statement : statements) {
    analyzeStatement(statement.get());
  }
  symbols.popScope();
}

void SemanticAnalyzer::analyzeIfStatement(IfStatementNode *node) {
  std::string condType = analyzeExpression(node->condition.get());
  if (condType != "bool") {
    throw CompileError("if condition must be bool, got " + condType,
                       node->line);
  }
  analyzeBlock(node->thenBlock);
  if (!node->elseBlock.empty()) {
    analyzeBlock(node->elseBlock);
  }
}

void SemanticAnalyzer::analyzeWhileStatement(WhileStatementNode *node) {
  std::string condType = analyzeExpression(node->condition.get());
  if (condType != "bool") {
    throw CompileError("while condition must be bool, got " + condType,
                       node->line);
  }
  analyzeBlock(node->body);
}

void SemanticAnalyzer::analyzeForStatement(ForStatementNode *node) {
  symbols.pushScope();
  analyzeStatement(node->declaration.get());
  std::string condType = analyzeExpression(node->condition.get());
  if (condType != "bool") {
    throw CompileError("for condition must be bool, got " + condType,
                       node->line);
  }
  analyzeStatement(node->increment.get());
  analyzeBlock(node->body);
  symbols.popScope();
}
