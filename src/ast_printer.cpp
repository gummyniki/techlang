#include "ast_printer.h"

// the connector symbols for the tree branches
// └── for last child, ├── for others
std::string ASTPrinter::getIndent(std::string prefix, bool isLast) {
  return prefix + (isLast ? "└── " : "├── ");
}

// what to pass down to children
std::string ASTPrinter::getChildPrefix(std::string prefix, bool isLast) {
  return prefix + (isLast ? "    " : "│   ");
}

void ASTPrinter::print(ASTNode *node, std::string prefix, bool isLast) {
  if (!node) {
    std::cout << getIndent(prefix, isLast) << "(null)\n";
    return;
  }
  printNode(node, prefix, isLast);
}

std::string tokenTypeToString(TokenType type) {
  switch (type) {
  case TokenType::PLUS:
    return "+";
  case TokenType::MINUS:
    return "-";
  case TokenType::STAR:
    return "*";
  case TokenType::SLASH:
    return "/";
  case TokenType::PERCENT:
    return "%";
  case TokenType::EQUALS:
    return "=";
  case TokenType::EQUALS_EQUALS:
    return "==";
  case TokenType::BANG_EQUALS:
    return "!=";
  case TokenType::LESS:
    return "<";
  case TokenType::GREATER:
    return ">";
  case TokenType::LESS_EQUALS:
    return "<=";
  case TokenType::GREATER_EQUALS:
    return ">=";
  case TokenType::AND_AND:
    return "&&";
  case TokenType::PIPE_PIPE:
    return "||";
  case TokenType::BANG:
    return "!";
  case TokenType::PLUS_EQUALS:
    return "+=";
  case TokenType::MINUS_EQUALS:
    return "-=";
  case TokenType::STAR_EQUALS:
    return "*=";
  case TokenType::SLASH_EQUALS:
    return "/=";
  default:
    return "?";
  }
}

void ASTPrinter::printNode(ASTNode *node, std::string prefix, bool isLast) {
  std::string indent = getIndent(prefix, isLast);
  std::string childPrefix = getChildPrefix(prefix, isLast);

  switch (node->type) {

  case NodeType::Program: {
    auto *n = static_cast<ProgramNode *>(node);
    std::cout << "Program\n";
    for (size_t i = 0; i < n->statements.size(); i++) {
      bool last = (i == n->statements.size() - 1);
      print(n->statements[i].get(), childPrefix, last);
    }
    break;
  }

  case NodeType::VarDeclaration: {
    auto *n = static_cast<VarDeclarationNode *>(node);
    std::cout << indent << "VarDeclaration [" << n->dataType << " " << n->name
              << "]";
    if (n->isConst)
      std::cout << " [const]";
    if (n->isFixed)
      std::cout << " [fixed]";
    std::cout << "\n";
    if (n->value) {
      print(n->value.get(), childPrefix, true);
    }
    break;
  }

  case NodeType::FunctionDeclaration: {
    auto *n = static_cast<FunctionDeclarationNode *>(node);
    std::cout << indent << "FunctionDeclaration [" << n->name << " -> "
              << n->returnType << "]\n";

    // print params as a simple list
    std::string paramPrefix = childPrefix;
    for (size_t i = 0; i < n->params.size(); i++) {
      bool last = (i == n->params.size() - 1) && n->body.empty();
      std::cout << getIndent(paramPrefix, last) << "Param ["
                << n->params[i].first << " " << n->params[i].second << "]\n";
    }

    // print body statements
    for (size_t i = 0; i < n->body.size(); i++) {
      bool last = (i == n->body.size() - 1);
      print(n->body[i].get(), childPrefix, last);
    }
    break;
  }

  case NodeType::IfStatement: {
    auto *n = static_cast<IfStatementNode *>(node);
    std::cout << indent << "IfStatement\n";

    // condition
    std::cout << getIndent(childPrefix, false) << "Condition\n";
    print(n->condition.get(), getChildPrefix(childPrefix, false), true);

    // then block
    bool hasElse = !n->elseBlock.empty();
    std::cout << getIndent(childPrefix, !hasElse) << "Then\n";
    std::string thenPrefix = getChildPrefix(childPrefix, !hasElse);
    for (size_t i = 0; i < n->thenBlock.size(); i++) {
      bool last = (i == n->thenBlock.size() - 1);
      print(n->thenBlock[i].get(), thenPrefix, last);
    }

    // else block (optional)
    if (hasElse) {
      std::cout << getIndent(childPrefix, true) << "Else\n";
      std::string elsePrefix = getChildPrefix(childPrefix, true);
      for (size_t i = 0; i < n->elseBlock.size(); i++) {
        bool last = (i == n->elseBlock.size() - 1);
        print(n->elseBlock[i].get(), elsePrefix, last);
      }
    }
    break;
  }

  case NodeType::WhileStatement: {
    auto *n = static_cast<WhileStatementNode *>(node);
    std::cout << indent << "WhileStatement\n";

    std::cout << getIndent(childPrefix, false) << "Condition\n";
    print(n->condition.get(), getChildPrefix(childPrefix, false), true);

    std::cout << getIndent(childPrefix, true) << "Body\n";
    std::string bodyPrefix = getChildPrefix(childPrefix, true);
    for (size_t i = 0; i < n->body.size(); i++) {
      bool last = (i == n->body.size() - 1);
      print(n->body[i].get(), bodyPrefix, last);
    }
    break;
  }

  case NodeType::ForStatement: {
    auto *n = static_cast<ForStatementNode *>(node);
    std::cout << indent << "ForStatement\n";

    std::cout << getIndent(childPrefix, false) << "Declaration\n";
    print(n->declaration.get(), getChildPrefix(childPrefix, false), true);

    std::cout << getIndent(childPrefix, false) << "Condition\n";
    print(n->condition.get(), getChildPrefix(childPrefix, false), true);

    std::cout << getIndent(childPrefix, true) << "Increment\n";
    print(n->increment.get(), getChildPrefix(childPrefix, true), true);

    std::cout << getIndent(childPrefix, true) << "Body\n";
    std::string bodyPrefix = getChildPrefix(childPrefix, true);
    for (size_t i = 0; i < n->body.size(); i++) {
      bool last = (i == n->body.size() - 1);
      print(n->body[i].get(), bodyPrefix, last);
    }
    break;
  }

  case NodeType::ReturnStatement: {
    auto *n = static_cast<ReturnStatementNode *>(node);
    std::cout << indent << "ReturnStatement\n";
    if (n->value) {
      print(n->value.get(), childPrefix, true);
    } else {
      std::cout << getIndent(childPrefix, true) << "(void)\n";
    }
    break;
  }

  case NodeType::BinaryExpression: {
    auto *n = static_cast<BinaryExpressionNode *>(node);
    std::cout << indent << "BinaryExpression [" << tokenTypeToString(n->op)
              << "]\n";
    print(n->left.get(), childPrefix, false);
    print(n->right.get(), childPrefix, true);
    break;
  }

  case NodeType::UnaryExpression: {
    auto *n = static_cast<UnaryExpressionNode *>(node);
    std::cout << indent << "UnaryExpression [" << tokenTypeToString(n->op)
              << "]\n";
    print(n->operand.get(), childPrefix, true);
    break;
  }

  case NodeType::AssignmentExpression: {
    auto *n = static_cast<AssignmentNode *>(node);
    std::cout << indent << "Assignment [" << n->name << " "
              << tokenTypeToString(n->op) << "]\n";
    print(n->value.get(), childPrefix, true);
    break;
  }

  case NodeType::FunctionCall: {
    auto *n = static_cast<FunctionCallNode *>(node);
    std::cout << indent << "FunctionCall [" << n->name << "]\n";
    for (size_t i = 0; i < n->args.size(); i++) {
      bool last = (i == n->args.size() - 1);
      print(n->args[i].get(), childPrefix, last);
    }
    break;
  }

  case NodeType::MemberAccess: {
    auto *n = static_cast<MemberAccessNode *>(node);
    std::cout << indent << "MemberAccess [." << n->member << "]\n";
    print(n->object.get(), childPrefix, true);
    break;
  }

  case NodeType::ArrayAccess: {
    auto *n = static_cast<ArrayAccessNode *>(node);
    std::cout << indent << "ArrayAccess\n";
    print(n->array.get(), childPrefix, false);
    print(n->index.get(), childPrefix, true);
    break;
  }

  // leaf nodes — no children, just print the value
  case NodeType::Identifier: {
    auto *n = static_cast<IdentifierNode *>(node);
    std::cout << indent << "Identifier [" << n->name << "]\n";
    break;
  }

  case NodeType::IntLiteral: {
    auto *n = static_cast<IntLiteralNode *>(node);
    std::cout << indent << "IntLiteral [" << n->value << "]\n";
    break;
  }

  case NodeType::FloatLiteral: {
    auto *n = static_cast<FloatLiteralNode *>(node);
    std::cout << indent << "FloatLiteral [" << n->value << "]\n";
    break;
  }

  case NodeType::StringLiteral: {
    auto *n = static_cast<StringLiteralNode *>(node);
    std::cout << indent << "StringLiteral [\"" << n->value << "\"]\n";
    break;
  }

  case NodeType::BoolLiteral: {
    auto *n = static_cast<BoolLiteralNode *>(node);
    std::cout << indent << "BoolLiteral [" << (n->value ? "true" : "false")
              << "]\n";
    break;
  }

  default:
    std::cout << indent << "UnknownNode\n";
    break;
  }
}
