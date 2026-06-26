#pragma once
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include <filesystem>
#include <string>
#include <unordered_set>

class ImportResolver {
public:
  ImportResolver(const std::string &baseDir, const std::string &compilerDir)
      : baseDir(baseDir), compilerDir(compilerDir) {}

  void resolve(ProgramNode *program);

private:
  std::string baseDir;
  std::string compilerDir;
  std::unordered_set<std::string> alreadyImported;

  void resolveImport(ImportNode *node);
  std::vector<std::unique_ptr<ASTNode>> parseFile(const std::string &path);
  void prefixDeclarations(std::vector<std::unique_ptr<ASTNode>> &declarations,
                          const std::string &alias);

  bool isDeclaration(ASTNode *node);
};
