#pragma once
#include "ast.h"
#include "backend/gpu_runtime_generator.h"
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

  std::string baseDir;
  std::string compilerDir;
  std::unordered_set<std::string> alreadyImported;
  std::vector<std::string> gpuWrapperFiles;
  std::vector<std::string> gpuAliases;
  void resolveVecTecImport(ImportNode *node);

  std::vector<KernelInfo> extractKernelInfo(ProgramNode *ast);
  void resolveImport(ImportNode *node);
  std::vector<std::unique_ptr<ASTNode>> parseFile(const std::string &path);
  void prefixDeclarations(std::vector<std::unique_ptr<ASTNode>> &declarations,
                          const std::string &alias);

  bool isDeclaration(ASTNode *node);
};
