#pragma once

#include "../ast.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Parser {
public:
  Parser(std::vector<Token> tokens) : tokens(tokens), pos(0) {}
  std::unique_ptr<ProgramNode> parse();

  std::vector<Token> tokens;
  size_t pos;

  Token current();
  Token advance();
  Token peek(int offset = 1);
  bool isAtEnd();

  // expects current token to be of a certain type, throws if not
  Token expect(TokenType type, std::string errorMsg);

  // parsing methods - one per language construct
  std::unique_ptr<ASTNode> parseStatement();
  std::string parseType();
  std::vector<std::pair<std::string, std::string>> parseParameterList();
  std::pair<std::string, std::string> parseParameter();
  std::unique_ptr<ASTNode> parseVarDeclaration(std::string dataType,
                                               TokenType terminator);
  std::unique_ptr<ASTNode> parseFunctionDeclaration();
  std::unique_ptr<ASTNode> parseStructDeclaration();
  std::unique_ptr<ASTNode> parseIfStatement();
  std::unique_ptr<ASTNode> parseWhileStatement();
  std::unique_ptr<ASTNode> parseForStatement();
  std::unique_ptr<ASTNode> parseEnumDeclaration();
  std::unique_ptr<ASTNode> parseReturnStatement();
  std::vector<std::unique_ptr<ASTNode>> parseBlock();
  std::unique_ptr<ASTNode> parseImport();

  std::unique_ptr<ASTNode> parseKernelDeclaration();
  std::unique_ptr<ASTNode> parseSharedDeclaration();

  // expressions - broken into levels for operator precedence
  std::unique_ptr<ASTNode> parseExpression();
  std::unique_ptr<ASTNode> parseComparison();
  std::unique_ptr<ASTNode> parseAddSub();
  std::unique_ptr<ASTNode> parseAnd();
  std::unique_ptr<ASTNode> parsePostfix(std::unique_ptr<ASTNode> left);
  std::unique_ptr<ASTNode> parseMulDiv();
  std::unique_ptr<ASTNode> parseCast();
  std::unique_ptr<ASTNode> parseUnary();
  std::unique_ptr<ASTNode> parsePrimary();
};
