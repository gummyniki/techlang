#pragma once
#include "ast.h"
#include <iostream>
#include <string>

class ASTPrinter {
public:
  void print(ASTNode *node, std::string prefix = "", bool isLast = true);

private:
  void printNode(ASTNode *node, std::string prefix, bool isLast);
  std::string getIndent(std::string prefix, bool isLast);
  std::string getChildPrefix(std::string prefix, bool isLast);
};
