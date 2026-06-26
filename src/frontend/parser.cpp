#include "parser.h"
#include "../ast.h"
#include "../debug/error.h"
#include "token.h"
#include "llvm/ADT/STLExtras.h"
#include <memory>
#include <stdexcept>
#include <string>

std::unique_ptr<ProgramNode> Parser::parse() {
  auto program = std::make_unique<ProgramNode>();

  while (!isAtEnd() && current().type != TokenType::END_OF_FILE) {
    program->statements.push_back(parseStatement());
  }

  return program;
}

Token Parser::current() { return tokens[pos]; }
Token Parser::advance() {
  Token token = current();
  ++pos;

  return token;
}
Token Parser::peek(int offset) {
  int target = pos + offset;

  if (target > tokens.size()) {
    throw CompileError("Parser tried to peek past end of token stream",
                       current().line, current().column);
  }

  return tokens[target];
}
bool Parser::isAtEnd() { return pos > tokens.size(); }

// expects current token to be of a certain type, throws if not
Token Parser::expect(TokenType type, std::string errorMsg) {
  if (current().type != type) {
    throw CompileError(errorMsg + " (got '" + current().value + "')",
                       current().line, current().column);
  }
  return advance();
}

std::string Parser::parseType() {
  // handle ArrayOf and PointerOf
  if (current().type == TokenType::KW_ARRAYOF ||
      current().type == TokenType::KW_POINTEROF) {
    std::string typeName = advance().value;
    expect(TokenType::LPAREN, "expected '(' after " + typeName);
    std::string innerType = advance().value;
    expect(TokenType::RPAREN, "expected ')' after inner type");
    return typeName + "(" + innerType + ")";
  }

  // simple type: int, float, string, etc
  return advance().value;
}

// parsing methods - one per language construct
std::unique_ptr<ASTNode> Parser::parseStatement() {
  if (current().type == TokenType::EXCLAIM) {
    return parseImport();
  }

  if (current().type == TokenType::KW_STRUCT) {
    return parseStructDeclaration();
  }

  if (current().type == TokenType::KW_ENUM) {
    return parseEnumDeclaration();
  }

  if (current().type == TokenType::IDENTIFIER) {
    std::string name = advance().value;

    if (current().type == TokenType::DOT) {
      advance(); // consume .
      std::string member =
          expect(TokenType::IDENTIFIER, "expected member name").value;

      if (current().type == TokenType::EQUALS ||
          current().type == TokenType::PLUS_EQUALS ||
          current().type == TokenType::MINUS_EQUALS ||
          current().type == TokenType::STAR_EQUALS ||
          current().type == TokenType::SLASH_EQUALS) {

        TokenType op = advance().type;
        auto value = parseExpression();
        expect(TokenType::SEMICOLON, "expected ';'");

        auto node = std::make_unique<MemberAssignmentNode>(current().line);
        node->objectName = name;
        node->memberName = member;
        node->op = op;
        node->value = std::move(value);
        return node;
      }

      if (current().type == TokenType::LPAREN) {
        advance(); // consume (
        std::string fullName = name + "." + member;
        auto node = std::make_unique<FunctionCallNode>(current().line);
        node->name = fullName;

        if (current().type != TokenType::RPAREN) {
          node->args.push_back(parseExpression());
          while (current().type == TokenType::COMMA) {
            advance();
            node->args.push_back(parseExpression());
          }
        }

        expect(TokenType::RPAREN, "expected ')'");
        expect(TokenType::SEMICOLON, "expected ';'");
        return node;
      }

      throw CompileError("unexpected token after member access '" +
                             current().value + "'",
                         current().line, current().column);
    }

    // regular assignment: x = 5;
    if (current().type == TokenType::EQUALS ||
        current().type == TokenType::PLUS_EQUALS ||
        current().type == TokenType::MINUS_EQUALS ||
        current().type == TokenType::STAR_EQUALS ||
        current().type == TokenType::SLASH_EQUALS) {

      TokenType op = advance().type;
      auto value = parseExpression();
      expect(TokenType::SEMICOLON, "expected ';'");

      auto node = std::make_unique<AssignmentNode>(current().line);
      node->name = name;
      node->op = op;
      node->value = std::move(value);
      return node;
    }

    if (current().type == TokenType::LPAREN) {
      advance(); // consume (
      auto node = std::make_unique<FunctionCallNode>(current().line);
      node->name = name;

      if (current().type != TokenType::RPAREN) {
        node->args.push_back(parseExpression());
        while (current().type == TokenType::COMMA) {
          advance();
          node->args.push_back(parseExpression());
        }
      }

      expect(TokenType::RPAREN, "expected ')'");
      expect(TokenType::SEMICOLON, "expected ';'");
      return node;
    }

    if (current().type == TokenType::IDENTIFIER) {
      auto node = std::make_unique<StructInstanceNode>(current().line);
      node->structType = name;
      node->name = advance().value;
      expect(TokenType::SEMICOLON, "expected ';'");
      return node;
    }

    throw CompileError("unexpected token '" + current().value + "'",
                       current().line, current().column);
  }

  // function declaration
  if (current().type == TokenType::KW_FUNCTION) {
    return parseFunctionDeclaration();
  }

  // if statement
  if (current().type == TokenType::KW_IF) {
    return parseIfStatement();
  }

  // while statement
  if (current().type == TokenType::KW_WHILE) {
    return parseWhileStatement();
  }

  // for statement
  if (current().type == TokenType::KW_FOR) {
    return parseForStatement();
  }

  // return statement
  if (current().type == TokenType::KW_RETURN) {
    return parseReturnStatement();
  }

  // variable declaration
  if (current().type == TokenType::KW_INT ||
      current().type == TokenType::KW_FLOAT ||
      current().type == TokenType::KW_DOUBLE ||
      current().type == TokenType::KW_STRING ||
      current().type == TokenType::KW_BOOL ||
      current().type == TokenType::KW_CHAR ||
      current().type == TokenType::KW_ARRAYOF ||
      current().type == TokenType::KW_POINTEROF) {
    std::string dataType = parseType();
    return parseVarDeclaration(dataType, TokenType::SEMICOLON);
  }

  throw CompileError("unexpected token '" + current().value + "'",
                     current().line, current().column);
}

std::vector<std::pair<std::string, std::string>> Parser::parseParameterList() {
  std::vector<std::pair<std::string, std::string>> params;

  // if there are no parameters
  if (current().type == TokenType::RPAREN) {
    return params;
  }

  // parse first parameter
  params.push_back(parseParameter());

  // parse remaining parameters, each preceded by a comma
  while (current().type == TokenType::COMMA) {
    advance(); // consume the comma

    // trailing comma guard: function foo(int a,) is invalid
    if (current().type == TokenType::RPAREN) {
      throw CompileError("trailing comma in parameter list", current().line,
                         current().column);
    }

    params.push_back(parseParameter());
  }

  return params;
}

std::pair<std::string, std::string> Parser::parseParameter() {
  std::string type;

  if (current().type == TokenType::KW_ARRAYOF ||
      current().type == TokenType::KW_POINTEROF) {
    type += advance().value;

    expect(TokenType::LPAREN, "expected '(' after ArrayOf/PointerOf");
    type += "(" + advance().value + ")";
    expect(TokenType::RPAREN, "expected ')' after inner type");
  } else {

    type = advance().value;
  }

  std::string name =
      expect(TokenType::IDENTIFIER, "expected parameter name").value;

  return {type, name};
}

std::unique_ptr<ASTNode> Parser::parseEnumDeclaration() {
  auto node = std::make_unique<EnumDeclarationNode>(current().line);
  advance(); // consume enum
  node->name = expect(TokenType::IDENTIFIER, "expected enum name").value;
  expect(TokenType::EQUALS, "expected '='");
  expect(TokenType::LBRACE, "expected '{'");

  int value = 0;

  while (!isAtEnd() && current().type != TokenType::RBRACE) {
    std::string entryName =
        expect(TokenType::IDENTIFIER, "expected enum entry name").value;

    // check for hard coded value
    if (current().type == TokenType::EQUALS) {
      advance(); // consume =
      value = std::stoi(
          expect(TokenType::INT_LITERAL, "expected integer value").value);
    }

    node->entries.push_back({entryName, value});
    value++;

    if (current().type == TokenType::COMMA) {
      advance();
    }
  }

  expect(TokenType::RBRACE, "expected '}'");
  return node;
}

std::unique_ptr<ASTNode>
Parser::parseVarDeclaration(std::string dataType,
                            TokenType terminator = TokenType::SEMICOLON) {
  auto node = std::make_unique<VarDeclarationNode>(current().line);
  node->dataType = dataType;

  node->name = expect(TokenType::IDENTIFIER, "expected variable name").value;

  expect(TokenType::EQUALS, "expected '=' after variable name");

  node->value = parseExpression();

  // check for optional params like [const] or [fixed]
  if (current().type == TokenType::LBRACKET) {
    advance(); // consume [
    std::string modifier =
        expect(TokenType::IDENTIFIER, "expected modifier").value;
    if (modifier == "const")
      node->isConst = true;
    if (modifier == "fixed")
      node->isFixed = true;
    expect(TokenType::RBRACKET, "expected ']'");
  }

  expect(terminator, "expected terminator after variable declaration");
  return node;
}

std::unique_ptr<ASTNode> Parser::parseFunctionDeclaration() {
  auto node = std::make_unique<FunctionDeclarationNode>(current().line);

  advance();

  node->name = expect(TokenType::IDENTIFIER, "expected function name").value;

  expect(TokenType::LPAREN, "expected '(' after function name");

  node->params = parseParameterList();

  expect(TokenType::RPAREN, "expected ')' after parameters");

  expect(TokenType::KW_RETURNS, "expected 'returns'");

  node->returnType = advance().value;

  if (current().type == TokenType::KW_EXTERN) {
    advance();
    node->externSymbol = expect(TokenType::STRING_LITERAL,
                                "expected C symbol name after 'extern'")
                             .value;
    expect(TokenType::SEMICOLON, "expected ';' after C function name");
  } else {

    node->body = parseBlock();
  }
  return node;
}

std::unique_ptr<ASTNode> Parser::parseIfStatement() {
  auto node = std::make_unique<IfStatementNode>(current().line);

  expect(TokenType::KW_IF, "expected 'if' keyword");

  expect(TokenType::LPAREN, "expected '(' after if keyword");
  node->condition = parseComparison();
  expect(TokenType::RPAREN, "expected ')' after condition");
  node->thenBlock = parseBlock();

  if (current().type == TokenType::KW_ELSE) {
    advance();
    expect(TokenType::KW_DO, "expected 'do' after else");
    node->elseBlock = parseBlock();
  }

  return node;
}

std::unique_ptr<ASTNode> Parser::parseStructDeclaration() {
  auto node = std::make_unique<StructDeclarationNode>(current().line);

  advance();
  node->name = expect(TokenType::IDENTIFIER, "Expected struct name").value;
  expect(TokenType::EQUALS, "Expected '=' after struct name");
  expect(TokenType::LBRACE, "Expected '{'");

  while (!isAtEnd() && current().type != TokenType::RBRACE) {
    std::string fieldType = parseType();
    std::string fieldName =
        expect(TokenType::IDENTIFIER, "expected field name").value;
    expect(TokenType::SEMICOLON, "expected ';' after field");
    node->fields.push_back({fieldType, fieldName});
  }

  expect(TokenType::RBRACE, "expected '}'");
  return node;
}

std::unique_ptr<ASTNode> Parser::parseWhileStatement() {
  auto node = std::make_unique<WhileStatementNode>(current().line);

  expect(TokenType::KW_WHILE, "expected 'while' keyword");
  expect(TokenType::LBRACE, "expected '(' after loop declaration");
  node->condition = parseComparison();
  expect(TokenType::RBRACE, "expected ')' after condition");
  node->body = parseBlock();
  return node;
}

std::unique_ptr<ASTNode> Parser::parseForStatement() {
  auto node = std::make_unique<ForStatementNode>(current().line);

  expect(TokenType::KW_FOR, "expected 'for'");
  expect(TokenType::LPAREN, "expected '(' after 'for'");

  std::string dataType = advance().value;
  node->declaration = parseVarDeclaration(dataType, TokenType::COMMA);

  node->condition = parseExpression();
  expect(TokenType::COMMA, "expected ',' after for condition");

  std::string name =
      expect(TokenType::IDENTIFIER, "expected identifier in for increment")
          .value;
  TokenType op = advance().type;
  auto value = parseExpression();
  auto increment = std::make_unique<AssignmentNode>(current().line);
  increment->name = name;
  increment->op = op;
  increment->value = std::move(value);
  node->increment = std::move(increment);

  expect(TokenType::RPAREN, "expected ')' after for clauses");
  node->body = parseBlock();

  return node;
}

std::unique_ptr<ASTNode> Parser::parseReturnStatement() {
  auto node = std::make_unique<ReturnStatementNode>(current().line);

  expect(TokenType::KW_RETURN, "expected 'return'");

  // if next token is semicolon, it's a bare return
  if (current().type == TokenType::SEMICOLON) {
    advance(); // consume ;
    node->value = nullptr;
    return node;
  }

  // otherwise parse the return value expression
  node->value = parseExpression();
  expect(TokenType::SEMICOLON, "expected ';' after return value");

  return node;
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseBlock() {
  expect(TokenType::LBRACE, "expected '{'");

  std::vector<std::unique_ptr<ASTNode>> statements;

  while (!isAtEnd() && current().type != TokenType::RBRACE) {
    statements.push_back(parseStatement());
  }

  expect(TokenType::RBRACE, "expected '}'");
  return statements;
}

// expressions - broken into levels for operator precedence
std::unique_ptr<ASTNode> Parser::parseExpression() {
  auto left = parseAnd();

  while (current().type == TokenType::PIPE_PIPE) {
    TokenType op = advance().type;
    auto right = parseAnd();

    auto node = std::make_unique<BinaryExpressionNode>(current().line);
    node->left = std::move(left);
    node->op = op;
    node->right = std::move(right);
    left = std::move(node);
  }

  return left;
}

// &&
std::unique_ptr<ASTNode> Parser::parseAnd() {
  auto left = parseComparison();

  while (current().type == TokenType::AND_AND) {
    TokenType op = advance().type;
    auto right = parseComparison();

    auto node = std::make_unique<BinaryExpressionNode>(current().line);
    node->left = std::move(left);
    node->op = op;
    node->right = std::move(right);
    left = std::move(node);
  }

  return left;
}

// == != < > <= >=
std::unique_ptr<ASTNode> Parser::parseComparison() {
  auto left = parseAddSub();

  while (current().type == TokenType::EQUALS_EQUALS ||
         current().type == TokenType::BANG_EQUALS ||
         current().type == TokenType::LESS ||
         current().type == TokenType::GREATER ||
         current().type == TokenType::LESS_EQUALS ||
         current().type == TokenType::GREATER_EQUALS) {

    TokenType op = advance().type;
    auto right = parseAddSub();

    auto node = std::make_unique<BinaryExpressionNode>(current().line);
    node->left = std::move(left);
    node->op = op;
    node->right = std::move(right);
    left = std::move(node);
  }

  return left;
}

// + -
std::unique_ptr<ASTNode> Parser::parseAddSub() {
  auto left = parseMulDiv();

  while (current().type == TokenType::PLUS ||
         current().type == TokenType::MINUS) {

    TokenType op = advance().type;
    auto right = parseMulDiv();

    auto node = std::make_unique<BinaryExpressionNode>(current().line);
    node->left = std::move(left);
    node->op = op;
    node->right = std::move(right);
    left = std::move(node);
  }

  return left;
}

std::unique_ptr<ASTNode> Parser::parsePostfix(std::unique_ptr<ASTNode> left) {
  while (true) {
    if (current().type == TokenType::DOT) {
      advance();
      std::string member =
          expect(TokenType::IDENTIFIER, "expected member name").value;

      if (current().type == TokenType::LPAREN) {
        advance();
        auto node = std::make_unique<FunctionCallNode>(current().line);

        // get the object name from the left side
        if (left->type == NodeType::Identifier) {
          auto *ident = static_cast<IdentifierNode *>(left.get());
          node->name = ident->name + "." + member;
        } else {
          throw CompileError("complex qualified calls not supported yet",
                             current().line, current().column);
        }

        if (current().type != TokenType::RPAREN) {
          node->args.push_back(parseExpression());
          while (current().type == TokenType::COMMA) {
            advance();
            node->args.push_back(parseExpression());
          }
        }

        expect(TokenType::RPAREN, "expected ')' after arguments");
        left = std::move(node);
        continue;
      }

      auto node = std::make_unique<MemberAccessNode>(current().line);
      node->object = std::move(left);
      node->member = member;
      left = std::move(node);
    } else if (current().type == TokenType::LBRACKET) {
      advance();
      auto index = parseExpression();
      expect(TokenType::RBRACKET, "expected ']'");

      auto node = std::make_unique<ArrayAccessNode>(current().line);
      node->array = std::move(left);
      node->index = std::move(index);
      left = std::move(node);
    } else {
      break;
    }
  }

  return left;
}

// * / %
std::unique_ptr<ASTNode> Parser::parseMulDiv() {
  auto left = parseUnary();

  while (current().type == TokenType::STAR ||
         current().type == TokenType::SLASH ||
         current().type == TokenType::PERCENT) {

    TokenType op = advance().type;
    auto right = parseUnary();

    auto node = std::make_unique<BinaryExpressionNode>(current().line);
    node->left = std::move(left);
    node->op = op;
    node->right = std::move(right);
    left = std::move(node);
  }

  return left;
}

// unary: ! and negative -
std::unique_ptr<ASTNode> Parser::parseUnary() {
  if (current().type == TokenType::BANG || current().type == TokenType::MINUS) {

    TokenType op = advance().type;
    auto operand = parseUnary();

    auto node = std::make_unique<UnaryExpressionNode>(current().line);
    node->op = op;
    node->operand = std::move(operand);
    return node;
  }

  return parsePrimary();
}

std::unique_ptr<ASTNode> Parser::parseImport() {
  auto node = std::make_unique<ImportNode>(current().line);

  expect(TokenType::EXCLAIM, "expected '!'");

  std::string keyword =
      expect(TokenType::IDENTIFIER, "expected 'import'").value;
  if (keyword != "import") {
    throw CompileError("expected 'import' after '!'", current().line,
                       current().column);
  }

  expect(TokenType::LPAREN, "expected '(' after import");

  node->filename = expect(TokenType::IDENTIFIER, "expected filename").value;

  if (current().type == TokenType::DOT) {
    advance(); // consume .
    std::string ext =
        expect(TokenType::IDENTIFIER, "expected file extension").value;
    node->filename += "." + ext;
  }

  expect(TokenType::RPAREN, "expected ')' after filename");
  expect(TokenType::KW_AS, "expected 'as' after filename");
  node->alias = expect(TokenType::IDENTIFIER, "expected alias name").value;
  expect(TokenType::SEMICOLON, "expected ';'");

  return node;
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {

  // array literal
  if (current().type == TokenType::LBRACE) {
    advance();
    auto node = std::make_unique<ArrayLiteralNode>(current().line);
    if (current().type != TokenType::RBRACE) {
      node->elements.push_back(parseExpression());
      while (current().type == TokenType::COMMA) {
        advance();
        node->elements.push_back(parseExpression());
      }
    }

    expect(TokenType::RBRACE, "expected '}' after array literal");
    return node;
  }

  // integer literal
  if (current().type == TokenType::INT_LITERAL) {
    int value = std::stoi(current().value);
    int line = current().line;
    advance();
    return std::make_unique<IntLiteralNode>(value, line);
  }

  // float literal
  if (current().type == TokenType::FLOAT_LITERAL) {
    float value = std::stof(current().value);
    int line = current().line;
    advance();
    return std::make_unique<FloatLiteralNode>(value, line);
  }

  // string literal
  if (current().type == TokenType::STRING_LITERAL) {
    std::string value = current().value;
    int line = current().line;
    advance();
    return std::make_unique<StringLiteralNode>(value, line);
  }

  // char literal
  if (current().type == TokenType::CHAR_LITERAL) {
    char value = current().value[0];
    int line = current().line;
    advance();
    return std::make_unique<CharLiteralNode>(value, line);
  }

  // bool literal
  if (current().type == TokenType::BOOL_LITERAL) {
    bool value = (current().value == "true");
    int line = current().line;
    advance();
    return std::make_unique<BoolLiteralNode>(value, line);
  }

  // identifier, function call, member access, array access
  if (current().type == TokenType::IDENTIFIER) {
    std::string name = advance().value;
    int line = current().line;

    // function call: foo(a, b)
    if (current().type == TokenType::LPAREN) {
      advance(); // consume (
      auto node = std::make_unique<FunctionCallNode>(line);
      node->name = name;

      // parse arguments
      if (current().type != TokenType::RPAREN) {
        node->args.push_back(parseExpression());
        while (current().type == TokenType::COMMA) {
          advance(); // consume comma
          node->args.push_back(parseExpression());
        }
      }

      expect(TokenType::RPAREN, "expected ')' after arguments");

      // could still be followed by .member or [index]
      return parsePostfix(std::move(node));
    }

    // plain identifier, but check for postfix
    auto node = std::make_unique<IdentifierNode>(name, line);
    return parsePostfix(std::move(node));
  }

  if (current().type == TokenType::EXCLAIM) {
    return parseImport();
  }

  if (current().type == TokenType::DOT) {
    int line = current().line;
    std::string name = advance().value;
    advance();
    std::string member =
        expect(TokenType::IDENTIFIER, "expected member name").value;

    if (current().type == TokenType::LPAREN) {
      advance();
      auto node = std::make_unique<FunctionCallNode>(line);
      node->name = name + "." + member;

      if (current().type != TokenType::RPAREN) {
        node->args.push_back(parseExpression());
        while (current().type == TokenType::COMMA) {
          advance();
          node->args.push_back(parseExpression());
        }
      }

      expect(TokenType::RPAREN, "expected ')' after arguments");
      return parsePostfix(std::move(node));
    }

    auto node = std::make_unique<MemberAccessNode>(line);
  }

  if (current().type == TokenType::LPAREN) {
    advance();
    auto expr = parseExpression();
    expect(TokenType::RPAREN, "expected ')'");
    return expr;
  }

  throw CompileError("unexpected token '" + current().value + "'",
                     current().line, current().column);
}
