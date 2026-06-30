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
  EnumDeclaration,
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
  StructDeclaration,
  KernelDeclaration,
  StructInstance,
  MemberAssignment,
  CharLiteral,
  BoolLiteral,
  ArrayLiteral,
  SharedDeclaration,
  MemberAccess,
  ArrayAccess,
  ArrayAssignment,
  Cast,
  Import,
};

// base node, every AST node inherits from this
struct ASTNode {
  NodeType type;
  int line;
  virtual ~ASTNode() = default;
  ASTNode(NodeType t, int line) : type(t), line(line) {}
};

struct CastNode : ASTNode {
  std::unique_ptr<ASTNode> value;
  std::string targetType;
  CastNode(int line) : ASTNode(NodeType::Cast, line) {}
};

struct SharedDeclarationNode : ASTNode {
  std::string elementType;
  std::string name;
  int size; // fixed size, must be a compile-time constant
  SharedDeclarationNode(int line)
      : ASTNode(NodeType::SharedDeclaration, line) {}
};

// the root of the whole program
struct ProgramNode : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> statements;
  ProgramNode() : ASTNode(NodeType::Program, 0) {}
};

struct ArrayLiteralNode : ASTNode {
  std::vector<std::unique_ptr<ASTNode>> elements;
  ArrayLiteralNode(int line) : ASTNode(NodeType::ArrayLiteral, line) {}
};

struct VarDeclarationNode : ASTNode {
  std::string dataType;
  std::string name;
  std::unique_ptr<ASTNode> value;
  bool isConst = false;
  bool isFixed = false;

  VarDeclarationNode(int line) : ASTNode(NodeType::VarDeclaration, line) {}
};

struct KernelDeclarationNode : ASTNode {
  std::string name;
  std::string returnType;
  std::vector<std::pair<std::string, std::string>> params;
  std::vector<std::unique_ptr<ASTNode>> body;
  KernelDeclarationNode(int line)
      : ASTNode(NodeType::KernelDeclaration, line) {}
};

struct EnumDeclarationNode : ASTNode {
  std::string name;
  std::vector<std::pair<std::string, int>> entries;

  EnumDeclarationNode(int line) : ASTNode(NodeType::EnumDeclaration, line) {}
};

struct ImportNode : ASTNode {
  std::string filename;
  std::string alias;
  std::vector<std::unique_ptr<ASTNode>> declarations;
  ImportNode(int line) : ASTNode(NodeType::Import, line) {}
};

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

struct CharLiteralNode : ASTNode {
  char value;
  CharLiteralNode(char v, int line)
      : ASTNode(NodeType::CharLiteral, line), value(v) {}
};

struct BoolLiteralNode : ASTNode {
  bool value;
  BoolLiteralNode(bool v, int line)
      : ASTNode(NodeType::BoolLiteral, line), value(v) {}
};

struct StructDeclarationNode : ASTNode {
  std::string name;
  std::vector<std::pair<std::string, std::string>> fields; // (type, name)
  StructDeclarationNode(int line)
      : ASTNode(NodeType::StructDeclaration, line) {}
};

struct StructInstanceNode : ASTNode {
  std::string structType;
  std::string name;
  StructInstanceNode(int line) : ASTNode(NodeType::StructInstance, line) {}
};

struct MemberAssignmentNode : ASTNode {
  std::string objectName;
  std::string memberName;
  TokenType op;
  std::unique_ptr<ASTNode> value;
  MemberAssignmentNode(int line) : ASTNode(NodeType::MemberAssignment, line) {}
};

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

struct FunctionDeclarationNode : ASTNode {
  std::string name;
  std::string returnType;
  std::string externSymbol;
  std::vector<std::pair<std::string, std::string>> params;
  std::vector<std::unique_ptr<ASTNode>> body;
  FunctionDeclarationNode(int line)
      : ASTNode(NodeType::FunctionDeclaration, line) {}
};

struct ReturnStatementNode : ASTNode {
  std::unique_ptr<ASTNode> value; // nullptr if returns none
  ReturnStatementNode(int line) : ASTNode(NodeType::ReturnStatement, line) {}
};

struct MemberAccessNode : ASTNode {
  std::unique_ptr<ASTNode> object;
  std::string member;
  MemberAccessNode(int line) : ASTNode(NodeType::MemberAccess, line) {}
};

struct ArrayAssignmentNode : ASTNode {
  std::string arrayName;
  std::string memberName;
  std::unique_ptr<ASTNode> index;
  std::unique_ptr<ASTNode> value;
  TokenType op;

  ArrayAssignmentNode(int line) : ASTNode(NodeType::ArrayAssignment, line) {}
};

struct ArrayAccessNode : ASTNode {
  std::unique_ptr<ASTNode> array;
  std::unique_ptr<ASTNode> index;
  ArrayAccessNode(int line) : ASTNode(NodeType::ArrayAccess, line) {}
};
