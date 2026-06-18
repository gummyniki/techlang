#pragma once

#include "token.h"
#include <iostream>
#include <memory>
#include <string.h>
#include <vector>

using namespace Tokens;

enum class NodeType {
  Program,
  VarDeclaration,
  FunctionDeclaration,
  ReturnStatement,
  IfStatement,
  WhileStatement,
  ForStatement,
  BinaryExpression,
  UnaryExpression,
  AssignmentExpression,
  FunctionCall,
  Identifier,
  IntLiteral,
  FloatLiteral,
  StringLiteral,
  CharLiteral,
  BoolLiteral,
  ArrayLiteral,
  MemberAccess, // for p.age
  ArrayAccess,  // for a[0]
};

// base node - every AST node inherits from this
struct ASTNode {
  NodeType type;
  int line;
  virtual ~ASTNode() = default;
  ASTNode(NodeType t, int line) : type(t), line(line) {}
};

// the root of the whole program
struct ProgramNode : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements;
  ProgramNode() : ASTNode(NodeType::Program, 0) {}
};

// int x = 5 + 3;
struct VarDeclarationNode : ASTNode {
  std::string dataType;
  std::string name;
  std::unique_ptr<ASTNode> value;
  bool isConst = false;
  bool isFixed = false;

  VarDeclarationNode(int line) : ASTNode(NodeType::VarDeclaration, line) {}
};

// 5 + 3, x > 0, etc
struct BinaryExpressionNode : ASTNode {
  std::unique_ptr<ASTNode> left;
  TokenType op;
  std::unique_ptr<ASTNode> right;

  BinaryExpressionNode(int line) : ASTNode(NodeType::BinaryExpression, line) {}
};

struct UnaryExpressionNode : ASTNode {
  TokenType op;
  std::unique_ptr<ASTNode> operand;

  UnaryExpressionNode(int line) : ASTNode(NodeType::UnaryExpression, line) {}
};

struct AssignmentNode : ASTNode {
  std::string name;
  TokenType op;
  std::unique_ptr<ASTNode> value;

  AssignmentNode(int line) : ASTNode(NodeType::AssignmentExpression, line) {}
};

// just a number literal
struct IntLiteralNode : ASTNode {
  int value;
  IntLiteralNode(int v, int line)
      : ASTNode(NodeType::IntLiteral, line), value(v) {}
};

struct FloatLiteralNode : ASTNode {
  float value;
  FloatLiteralNode(float v, int line)
      : ASTNode(NodeType::FloatLiteral, line), value(v) {}
};

struct StringLiteralNode : ASTNode {
  std::string value;
  StringLiteralNode(std::string v, int line)
      : ASTNode(NodeType::StringLiteral, line), value(v) {}
};

struct BoolLiteralNode : ASTNode {
  bool value;
  BoolLiteralNode(bool v, int line)
      : ASTNode(NodeType::BoolLiteral, line), value(v) {}
};

// just a variable name, like x or myVar
struct IdentifierNode : ASTNode {
  std::string name;
  IdentifierNode(std::string n, int line)
      : ASTNode(NodeType::Identifier, line), name(n) {}
};

// function call: foo(a, b)
struct FunctionCallNode : ASTNode {
  std::string name;
  std::vector<std::unique_ptr<ASTNode>> args;
  FunctionCallNode(int line) : ASTNode(NodeType::FunctionCall, line) {}
};

// if (cond) do { ... } else do { ... }
struct IfStatementNode : ASTNode {
  std::unique_ptr<ASTNode> condition;
  std::vector<std::unique_ptr<ASTNode>> thenBlock;
  std::vector<std::unique_ptr<ASTNode>> elseBlock; // empty if no else
  IfStatementNode(int line) : ASTNode(NodeType::IfStatement, line) {}
};

// while (cond) do { ... }
struct WhileStatementNode : ASTNode {
  std::unique_ptr<ASTNode> condition;
  std::vector<std::unique_ptr<ASTNode>> body;
  WhileStatementNode(int line) : ASTNode(NodeType::WhileStatement, line) {}
};

struct ForStatementNode : ASTNode {
  std::unique_ptr<ASTNode> declaration;
  std::unique_ptr<ASTNode> condition;
  std::unique_ptr<ASTNode> increment;
  std::vector<std::unique_ptr<ASTNode>> body;
  ForStatementNode(int line) : ASTNode(NodeType::ForStatement, line) {}
};

// function foo(int x) returns int { ... }
struct FunctionDeclarationNode : ASTNode {
  std::string name;
  std::string returnType;
  std::vector<std::pair<std::string, std::string>> params; // (type, name)
  std::vector<std::unique_ptr<ASTNode>> body;
  FunctionDeclarationNode(int line)
      : ASTNode(NodeType::FunctionDeclaration, line) {}
};

// return x + 1;
struct ReturnStatementNode : ASTNode {
  std::unique_ptr<ASTNode> value; // nullptr if returns none
  ReturnStatementNode(int line) : ASTNode(NodeType::ReturnStatement, line) {}
};

// p.age
struct MemberAccessNode : ASTNode {
  std::unique_ptr<ASTNode> object;
  std::string member;
  MemberAccessNode(int line) : ASTNode(NodeType::MemberAccess, line) {}
};

// a[0]
struct ArrayAccessNode : ASTNode {
  std::unique_ptr<ASTNode> array;
  std::unique_ptr<ASTNode> index;
  ArrayAccessNode(int line) : ASTNode(NodeType::ArrayAccess, line) {}
};
