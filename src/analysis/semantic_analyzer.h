#pragma once
#include "ast.h"
#include "symbol_table.h"
#include <string>

struct StructDefinition {
  std::string name;
  std::vector<std::pair<std::string, std::string>> fields; // (type, name)

  // helper to find field type by name
  std::optional<std::string> getFieldType(const std::string &fieldName) const {
    for (auto &[type, name] : fields) {
      if (name == fieldName)
        return type;
    }
    return std::nullopt;
  }
};

struct EnumDefinition {
  std::string name;
  std::unordered_map<std::string, int> values;

  std::optional<int> getValue(const std::string &entryName) const {
    auto it = values.find(entryName);
    if (it != values.end())
      return it->second;
    return std::nullopt;
  }
};

class SemanticAnalyzer {
public:
  void analyze(ProgramNode *program);

  SymbolTable symbols;
  std::string currentFunctionReturnType; // track what we're returning from
  std::unordered_map<std::string, StructDefinition> structTable;
  std::unordered_map<std::string, EnumDefinition> enumTable;

  void analyzeStatement(ASTNode *node);
  void analyzeVarDeclaration(VarDeclarationNode *node);
  void analyzeFunctionDeclaration(FunctionDeclarationNode *node);
  void analyzeIfStatement(IfStatementNode *node);
  void analyzeWhileStatement(WhileStatementNode *node);
  void analyzeForStatement(ForStatementNode *node);
  void analyzeReturnStatement(ReturnStatementNode *node);
  void analyzeStructDeclaration(StructDeclarationNode *node);
  void analyzeStructInstance(StructInstanceNode *node);
  void analyzeMemberAssignment(MemberAssignmentNode *node);
  void analyzeBlock(std::vector<std::unique_ptr<ASTNode>> &statements);
  void analyzeEnumDeclaration(EnumDeclarationNode *node);

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
