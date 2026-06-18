#include "parser.h"
#include <stdexcept>

// in parse()
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
    throw std::runtime_error(
        "Parser tried to peek at index bigger than token size!");
  }

  return tokens[target];
}
bool Parser::isAtEnd() { return pos > tokens.size(); }

// expects current token to be of a certain type, throws if not
Token Parser::expect(TokenType type, std::string errorMsg) {
  if (current().type != type) {
    throw std::runtime_error("Line " + std::to_string(current().line) + ": " +
                             errorMsg + " (got '" + current().value + "')");
  }
  return advance();
}

// parsing methods - one per language construct
std::unique_ptr<ASTNode> Parser::parseStatement() {
  if (current().type == TokenType::IDENTIFIER) {
    // could be assignment or function call as a statement
    std::string name = advance().value;

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
    // otherwise it's a function call statement like std.print(x);
    // ...
  }

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

  // variable declaration (starts with a type keyword)
  if (current().type == TokenType::KW_INT ||
      current().type == TokenType::KW_FLOAT ||
      current().type == TokenType::KW_DOUBLE ||
      current().type == TokenType::KW_STRING ||
      current().type == TokenType::KW_BOOL ||
      current().type == TokenType::KW_CHAR ||
      current().type == TokenType::KW_ARRAYOF) {
    std::string dataType = advance().value; // consume the type
    return parseVarDeclaration(dataType, TokenType::SEMICOLON);
  }

  throw std::runtime_error("Line " + std::to_string(current().line) +
                           ": unexpected token '" + current().value + "'");
}

std::vector<std::pair<std::string, std::string>> Parser::parseParameterList() {
  std::vector<std::pair<std::string, std::string>> params;

  // empty parameter list — function foo() returns none
  if (current().type == TokenType::RPAREN) {
    return params; // return empty list, don't consume the )
  }

  // parse first parameter
  params.push_back(parseParameter());

  // parse remaining parameters, each preceded by a comma
  while (current().type == TokenType::COMMA) {
    advance(); // consume the comma

    // trailing comma guard: function foo(int a,) is invalid
    if (current().type == TokenType::RPAREN) {
      throw std::runtime_error("Line " + std::to_string(current().line) +
                               ": trailing comma in parameter list");
    }

    params.push_back(parseParameter());
  }

  return params;
}

std::pair<std::string, std::string> Parser::parseParameter() {
  std::string type;

  // handle complex types like ArrayOf(int) or PointerOf(int)
  if (current().type == TokenType::KW_ARRAYOF ||
      current().type == TokenType::KW_POINTEROF) {
    type += advance().value; // "ArrayOf" or "PointerOf"
    expect(TokenType::LPAREN, "expected '(' after ArrayOf/PointerOf");
    type += "(" + advance().value + ")"; // the inner type
    expect(TokenType::RPAREN, "expected ')' after inner type");
  } else {
    // simple type: int, float, string, etc
    type = advance().value;
  }

  std::string name =
      expect(TokenType::IDENTIFIER, "expected parameter name").value;

  return {type, name};
}

std::unique_ptr<ASTNode>
Parser::parseVarDeclaration(std::string dataType,
                            TokenType terminator = TokenType::SEMICOLON) {
  auto node = std::make_unique<VarDeclarationNode>(current().line);
  node->dataType = dataType;

  // get the variable name
  node->name = expect(TokenType::IDENTIFIER, "expected variable name").value;

  // expect =
  expect(TokenType::EQUALS, "expected '=' after variable name");

  // parse the value expression
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

  expect(terminator, "expected 'terminator after variable declaration");
  return node;
}

std::unique_ptr<ASTNode> Parser::parseFunctionDeclaration() {
  auto node = std::make_unique<FunctionDeclarationNode>(current().line);

  advance(); // consume 'function' keyword

  // get function name
  node->name = expect(TokenType::IDENTIFIER, "expected function name").value;

  // opening paren
  expect(TokenType::LPAREN, "expected '(' after function name");

  // parse parameters
  node->params = parseParameterList();

  // closing paren
  expect(TokenType::RPAREN, "expected ')' after parameters");

  // 'returns' keyword
  expect(TokenType::KW_RETURNS, "expected 'returns'");

  // return type
  node->returnType = advance().value; // int, float, string, none, etc

  // parse the function body { ... }
  node->body = parseBlock();

  return node;
}

std::unique_ptr<ASTNode> Parser::parseIfStatement() {
  auto node = std::make_unique<IfStatementNode>(current().line);

  expect(TokenType::KW_IF, "expected 'if' keyword");

  expect(TokenType::LPAREN, "expected '(' after if keyword");
  node->condition = parseComparison();
  expect(TokenType::RPAREN, "expected ')' after condition");
  expect(TokenType::KW_DO, "expected 'do' after condition end");
  node->thenBlock = parseBlock();

  if (current().type == TokenType::KW_ELSE) {
    advance();
    expect(TokenType::KW_DO, "expected 'do' after else");
    node->elseBlock = parseBlock();
  }

  return node;
}

std::unique_ptr<ASTNode> Parser::parseWhileStatement() {
  auto node = std::make_unique<WhileStatementNode>(current().line);

  expect(TokenType::KW_WHILE, "expected 'while' keyword");
  expect(TokenType::LBRACE, "expected '(' after loop declaration");
  node->condition = parseComparison();
  expect(TokenType::RBRACE, "expected ')' after condition");
  expect(TokenType::KW_DO, "expected 'do' after condition end");
  node->body = parseBlock();
  return node;
}

std::unique_ptr<ASTNode> Parser::parseForStatement() {
  auto node = std::make_unique<ForStatementNode>(current().line);

  expect(TokenType::KW_FOR, "expected 'for'");
  expect(TokenType::LPAREN, "expected '(' after 'for'");

  // consume the type keyword first, then parse declaration
  std::string dataType = advance().value;
  node->declaration = parseVarDeclaration(dataType, TokenType::COMMA);
  // note: parseVarDeclaration already consumes the semicolon
  // but in a for loop we use commas, so we need to adjust!

  node->condition = parseExpression();
  expect(TokenType::COMMA, "expected ',' after for condition");

  // this is an assignment: identifier op expression
  std::string name =
      expect(TokenType::IDENTIFIER, "expected identifier in for increment")
          .value;
  TokenType op = advance().type; // consume +=, -=, etc
  auto value = parseExpression();
  auto increment = std::make_unique<AssignmentNode>(current().line);
  increment->name = name;
  increment->op = op;
  increment->value = std::move(value);
  node->increment = std::move(increment);

  expect(TokenType::RPAREN, "expected ')' after for clauses");
  expect(TokenType::KW_DO, "expected 'do' after for header");
  node->body = parseBlock();

  return node;
}

std::unique_ptr<ASTNode> Parser::parseReturnStatement() {
  auto node = std::make_unique<ReturnStatementNode>(current().line);

  expect(TokenType::KW_RETURN, "expected 'return'");

  // if next token is semicolon, it's a bare return (for none functions)
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

} // parses { ... }

// expressions - broken into levels for operator precedence
std::unique_ptr<ASTNode> Parser::parseExpression() {
  auto left = parseAnd();

  while (current().type == TokenType::PIPE_PIPE) {
    TokenType op = advance().type; // consume ||
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
    // member access: p.age
    if (current().type == TokenType::DOT) {
      advance(); // consume .
      std::string member =
          expect(TokenType::IDENTIFIER, "expected member name").value;

      auto node = std::make_unique<MemberAccessNode>(current().line);
      node->object = std::move(left);
      node->member = member;
      left = std::move(node);
    }
    // array access: a[0]
    else if (current().type == TokenType::LBRACKET) {
      advance(); // consume [
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
    auto operand = parseUnary(); // recursive! handles !!x or --x

    auto node = std::make_unique<UnaryExpressionNode>(current().line);
    node->op = op;
    node->operand = std::move(operand);
    return node;
  }

  return parsePrimary();
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
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

  // parenthesized expression: (2 + 3)
  if (current().type == TokenType::LPAREN) {
    advance();                     // consume (
    auto expr = parseExpression(); // recurse!
    expect(TokenType::RPAREN, "expected ')'");
    return expr;
  }

  throw std::runtime_error("Line " + std::to_string(current().line) +
                           ": unexpected token '" + current().value + "'");
}
