#include "semantic_analyzer.h"
#include "ast.h"
#include <stdexcept>

void SemanticAnalyzer::analyze(ProgramNode *program) {
  // push the global scope
  symbols.pushScope();

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
  case NodeType::AssignmentExpression: {
    auto *n = static_cast<AssignmentNode *>(node);
    // check the variable exists
    auto symbol = symbols.lookup(n->name);
    if (!symbol) {
      throw std::runtime_error("Line " + std::to_string(n->line) +
                               ": undefined variable '" + n->name + "'");
    }
    // check it's not const
    if (symbol->isConst) {
      throw std::runtime_error("Line " + std::to_string(n->line) +
                               ": cannot assign to const variable '" + n->name +
                               "'");
    }
    // check type of value matches
    std::string valueType = analyzeExpression(n->value.get());
    if (!typesAreCompatible(symbol->type, valueType)) {
      throw std::runtime_error("Line " + std::to_string(n->line) +
                               ": cannot assign " + valueType +
                               " to variable of type " + symbol->type);
    }
    break;
  }
  case NodeType::FunctionCall: {
    auto *n = static_cast<FunctionCallNode *>(node);
    // for std.print and std.exit, just check args exist
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

void SemanticAnalyzer::analyzeVarDeclaration(VarDeclarationNode *node) {
  // analyze the value expression and get its type
  std::string valueType = analyzeExpression(node->value.get());

  // check type compatibility
  if (!typesAreCompatible(node->dataType, valueType)) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": cannot assign " + valueType +
                             " to variable of type " + node->dataType);
  }

  // declare in current scope
  Symbol symbol;
  symbol.name = node->name;
  symbol.type = node->dataType;
  symbol.isConst = node->isConst;
  symbols.declare(symbol);
}

void SemanticAnalyzer::analyzeFunctionDeclaration(
    FunctionDeclarationNode *node) {
  // declare the function in current scope BEFORE analyzing body
  // so recursive functions can call themselves
  Symbol funcSymbol;
  funcSymbol.name = node->name;
  funcSymbol.type = node->returnType;
  funcSymbol.isFunction = true;
  funcSymbol.returnType = node->returnType;
  for (auto &param : node->params) {
    funcSymbol.paramTypes.push_back(param.first);
  }
  symbols.declare(funcSymbol);

  // push a new scope for the function body
  symbols.pushScope();

  // declare parameters in function scope
  for (auto &[type, name] : node->params) {
    Symbol paramSymbol;
    paramSymbol.name = name;
    paramSymbol.type = type;
    symbols.declare(paramSymbol);
  }

  // track return type so return statements can check against it
  std::string previousReturnType = currentFunctionReturnType;
  currentFunctionReturnType = node->returnType;

  // analyze body
  analyzeBlock(node->body);

  // restore previous return type (handles nested functions later)
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

  case NodeType::Identifier: {
    auto *n = static_cast<IdentifierNode *>(node);
    auto symbol = symbols.lookup(n->name);
    if (!symbol) {
      throw std::runtime_error("Line " + std::to_string(n->line) +
                               ": undefined variable '" + n->name + "'");
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

    if (!structTable.count(objectType)) {
      throw std::runtime_error("Line " + std::to_string(n->line) + ": '" +
                               objectType + "' is not a struct type");
    }

    auto fieldType = structTable[objectType].getFieldType(n->member);
    if (!fieldType) {
      throw std::runtime_error("Line " + std::to_string(n->line) +
                               ": struct '" + objectType + "' has no field '" +
                               n->member + "'");
    }

    return *fieldType;
  }

  case NodeType::ArrayLiteral: {
    auto *n = static_cast<ArrayLiteralNode *>(node);

    // empty array, can't infer type
    if (n->elements.empty()) {
      return "ArrayOf(unknown)";
    }

    // infer type from first element
    std::string elementType = analyzeExpression(n->elements[0].get());

    // check all elements are the same type
    for (size_t i = 1; i < n->elements.size(); i++) {
      std::string t = analyzeExpression(n->elements[i].get());
      if (!typesAreCompatible(elementType, t)) {
        throw std::runtime_error(
            "Line " + std::to_string(n->line) +
            ": array elements must be the same type, got " + elementType +
            " and " + t);
      }
    }

    return "ArrayOf(" + elementType + ")";
  }

  case NodeType::ArrayAccess: {
    auto *n = static_cast<ArrayAccessNode *>(node);
    std::string arrayType = analyzeExpression(n->array.get());
    std::string indexType = analyzeExpression(n->index.get());
    if (indexType != "int") {
      throw std::runtime_error("Line " + std::to_string(n->line) +
                               ": array index must be int, got " + indexType);
    }
    // strip "ArrayOf(int)" -> "int"
    if (arrayType.substr(0, 7) == "ArrayOf") {
      return arrayType.substr(8, arrayType.size() - 9);
    }
    return "unknown";
  }

  default:
    throw std::runtime_error("Unknown expression type");
  }
}

std::string
SemanticAnalyzer::analyzeBinaryExpression(BinaryExpressionNode *node) {
  std::string leftType = analyzeExpression(node->left.get());
  std::string rightType = analyzeExpression(node->right.get());

  // comparison operators always return bool
  if (node->op == TokenType::EQUALS_EQUALS ||
      node->op == TokenType::BANG_EQUALS || node->op == TokenType::LESS ||
      node->op == TokenType::GREATER || node->op == TokenType::LESS_EQUALS ||
      node->op == TokenType::GREATER_EQUALS || node->op == TokenType::AND_AND ||
      node->op == TokenType::PIPE_PIPE) {
    return "bool";
  }

  // arithmetic operators — check compatibility and return result type
  return binaryResultType(leftType, rightType, node->op);
}

std::string
SemanticAnalyzer::analyzeUnaryExpression(UnaryExpressionNode *node) {
  std::string operandType = analyzeExpression(node->operand.get());

  if (node->op == TokenType::BANG) {
    if (operandType != "bool") {
      throw std::runtime_error("Line " + std::to_string(node->line) +
                               ": '!' operator requires bool, got " +
                               operandType);
    }
    return "bool";
  }

  if (node->op == TokenType::MINUS) {
    if (operandType != "int" && operandType != "float" &&
        operandType != "double") {
      throw std::runtime_error("Line " + std::to_string(node->line) +
                               ": unary '-' requires numeric type, got " +
                               operandType);
    }
    return operandType;
  }

  return operandType;
}

std::string SemanticAnalyzer::analyzeFunctionCall(FunctionCallNode *node) {
  auto symbol = symbols.lookup(node->name);
  if (!symbol) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": undefined function '" + node->name + "'");
  }
  if (!symbol->isFunction) {
    throw std::runtime_error("Line " + std::to_string(node->line) + ": '" +
                             node->name + "' is not a function");
  }

  // check argument count
  if (node->args.size() != symbol->paramTypes.size()) {
    throw std::runtime_error(
        "Line " + std::to_string(node->line) + ": function '" + node->name +
        "' expects " + std::to_string(symbol->paramTypes.size()) +
        " arguments, got " + std::to_string(node->args.size()));
  }

  // check argument types
  for (size_t i = 0; i < node->args.size(); i++) {
    std::string argType = analyzeExpression(node->args[i].get());
    if (!typesAreCompatible(symbol->paramTypes[i], argType)) {
      throw std::runtime_error("Line " + std::to_string(node->line) +
                               ": argument " + std::to_string(i + 1) + " of '" +
                               node->name + "' expects " +
                               symbol->paramTypes[i] + ", got " + argType);
    }
  }

  return symbol->returnType;
}

bool SemanticAnalyzer::typesAreCompatible(const std::string &a,
                                          const std::string &b) {
  if (a == b)
    return true;

  // numeric promotions: int is compatible with float and double
  bool aIsNumeric = (a == "int" || a == "float" || a == "double");
  bool bIsNumeric = (b == "int" || b == "float" || b == "double");
  if (aIsNumeric && bIsNumeric)
    return true;

  // array type compatibility: ArrayOf(int) vs ArrayOf(int)
  // also handles numeric promotion inside arrays
  if (a.substr(0, 7) == "ArrayOf" && b.substr(0, 7) == "ArrayOf") {
    std::string innerA =
        a.substr(8, a.size() - 9); // extract "int" from "ArrayOf(int)"
    std::string innerB = b.substr(8, b.size() - 9);
    return typesAreCompatible(innerA, innerB);
  }

  return false;
}

void SemanticAnalyzer::analyzeStructDeclaration(StructDeclarationNode *node) {
  if (structTable.count(node->name)) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": struct '" + node->name + "' already defined");
  }

  StructDefinition def;
  def.name = node->name;
  def.fields = node->fields;
  structTable[node->name] = def;
}

void SemanticAnalyzer::analyzeStructInstance(StructInstanceNode *node) {
  if (!structTable.count(node->structType)) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": unknown struct type '" + node->structType +
                             "'");
  }

  // declare the variable with its struct type
  Symbol symbol;
  symbol.name = node->name;
  symbol.type = node->structType;
  symbols.declare(symbol);
}

void SemanticAnalyzer::analyzeMemberAssignment(MemberAssignmentNode *node) {
  // check object exists
  auto symbol = symbols.lookup(node->objectName);
  if (!symbol) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": undefined variable '" + node->objectName + "'");
  }

  // check it's a struct type
  if (!structTable.count(symbol->type)) {
    throw std::runtime_error("Line " + std::to_string(node->line) + ": '" +
                             node->objectName + "' is not a struct");
  }

  // check field exists
  auto &def = structTable[symbol->type];
  auto fieldType = def.getFieldType(node->memberName);
  if (!fieldType) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": struct '" + symbol->type + "' has no field '" +
                             node->memberName + "'");
  }

  // check value type matches field type
  std::string valueType = analyzeExpression(node->value.get());
  if (!typesAreCompatible(*fieldType, valueType)) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": cannot assign " + valueType + " to field '" +
                             node->memberName + "' of type " + *fieldType);
  }
}

std::string SemanticAnalyzer::binaryResultType(const std::string &left,
                                               const std::string &right,
                                               TokenType op) {
  bool leftIsNumeric = (left == "int" || left == "float" || left == "double");
  bool rightIsNumeric =
      (right == "int" || right == "float" || right == "double");

  if (leftIsNumeric && rightIsNumeric) {
    // promote to the "wider" type
    if (left == "double" || right == "double")
      return "double";
    if (left == "float" || right == "float")
      return "float";
    return "int";
  }

  // string concatenation with +
  if (left == "string" && right == "string" && op == TokenType::PLUS) {
    return "string";
  }

  throw std::runtime_error("Type error: cannot apply operator to " + left +
                           " and " + right);
}

void SemanticAnalyzer::analyzeReturnStatement(ReturnStatementNode *node) {
  if (currentFunctionReturnType == "none") {
    if (node->value) {
      throw std::runtime_error(
          "Line " + std::to_string(node->line) +
          ": cannot return a value from a function that returns none");
    }
    return;
  }

  if (!node->value) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": expected a return value of type " +
                             currentFunctionReturnType);
  }

  std::string returnType = analyzeExpression(node->value.get());
  if (!typesAreCompatible(currentFunctionReturnType, returnType)) {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": function returns " + currentFunctionReturnType +
                             " but got " + returnType);
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
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": if condition must be bool, got " + condType);
  }
  analyzeBlock(node->thenBlock);
  if (!node->elseBlock.empty()) {
    analyzeBlock(node->elseBlock);
  }
}

void SemanticAnalyzer::analyzeWhileStatement(WhileStatementNode *node) {
  std::string condType = analyzeExpression(node->condition.get());
  if (condType != "bool") {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": while condition must be bool, got " + condType);
  }
  analyzeBlock(node->body);
}

void SemanticAnalyzer::analyzeForStatement(ForStatementNode *node) {
  symbols.pushScope(); // for loop gets its own scope for the declaration
  analyzeStatement(node->declaration.get());
  std::string condType = analyzeExpression(node->condition.get());
  if (condType != "bool") {
    throw std::runtime_error("Line " + std::to_string(node->line) +
                             ": for condition must be bool, got " + condType);
  }
  analyzeStatement(node->increment.get());
  analyzeBlock(node->body);
  symbols.popScope();
}
