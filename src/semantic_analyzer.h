#pragma once
#include "ast.h"
#include "symbol_table.h"
#include <string>

class SemanticAnalyzer {
public:
  void analyze(ProgramNode *program);

  SymbolTable symbols;
  std::string currentFunctionReturnType; // track what we're returning from

  void analyzeStatement(ASTNode *node);
  void analyzeVarDeclaration(VarDeclarationNode *node);
  void analyzeFunctionDeclaration(FunctionDeclarationNode *node);
  void analyzeIfStatement(IfStatementNode *node);
  void analyzeWhileStatement(WhileStatementNode *node);
  void analyzeForStatement(ForStatementNode *node);
  void analyzeReturnStatement(ReturnStatementNode *node);
  void analyzeBlock(std::vector<std::unique_ptr<ASTNode>> &statements);

  // expressions return their type to check mismatches
  std::string analyzeExpression(ASTNode *node);
  std::string analyzeBinaryExpression(BinaryExpressionNode *node);
  std::string analyzeUnaryExpression(UnaryExpressionNode *node);
  std::string analyzeFunctionCall(FunctionCallNode *node);

  // type helpers
  bool typesAreCompatible(const std::string &a, const std::string &b);
  std::string binaryResultType(const std::string &left,
                               const std::string &right, TokenType op);
};
