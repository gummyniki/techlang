#include "import_resolver.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

void ImportResolver::resolve(ProgramNode *program) {
  // first pass: resolve all imports
  for (auto &statement : program->statements) {
    if (statement->type == NodeType::Import) {
      resolveImport(static_cast<ImportNode *>(statement.get()));
    }
  }

  // second pass: inject declarations at the top of the program
  std::vector<std::unique_ptr<ASTNode>> injected;
  for (auto &statement : program->statements) {
    if (statement->type == NodeType::Import) {
      auto *importNode = static_cast<ImportNode *>(statement.get());
      for (auto &decl : importNode->declarations) {
        injected.push_back(std::move(decl));
      }
    }
  }

  // prepend injected declarations before everything else
  for (int i = injected.size() - 1; i >= 0; i--) {
    program->statements.insert(program->statements.begin(),
                               std::move(injected[i]));
  }
}

void ImportResolver::prefixDeclarations(
    std::vector<std::unique_ptr<ASTNode>> &declarations,
    const std::string &alias) {

  for (auto &decl : declarations) {
    if (decl->type == NodeType::FunctionDeclaration) {
      auto *func = static_cast<FunctionDeclarationNode *>(decl.get());
      // "myFunc" becomes "math.myFunc"
      func->name = alias + "." + func->name;
    }
    // structs and enums don't get prefixed — they're used by type name
  }
}

void ImportResolver::resolveImport(ImportNode *node) {
  // check local directory first, then compiler directory
  std::string fullPath = baseDir + "/" + node->filename;

  if (!std::filesystem::exists(fullPath)) {
    // fall back to compiler directory
    fullPath = compilerDir + "/" + node->filename;
  }

  if (!std::filesystem::exists(fullPath)) {
    throw std::runtime_error("Cannot find import file: " + node->filename);
  }

  // prevent importing the same file twice
  if (alreadyImported.count(fullPath))
    return;
  alreadyImported.insert(fullPath);

  // parse the file
  auto declarations = parseFile(fullPath);
  prefixDeclarations(declarations, node->alias);

  for (auto &decl : declarations) {
    node->declarations.push_back(std::move(decl));
  }
}

std::vector<std::unique_ptr<ASTNode>>
ImportResolver::parseFile(const std::string &path) {

  // read the file
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open import file: " + path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string source = buffer.str();

  // lex and parse it
  Lexers::Lexer lexer(source);
  auto tokens = lexer.tokenize();
  Parser parser(tokens);
  auto ast = parser.parse();

  // recursively resolve imports in this file too
  resolve(ast.get());

  // extract only declarations (functions, structs, enums)
  // skip imports and top-level statements
  std::vector<std::unique_ptr<ASTNode>> declarations;
  for (auto &statement : ast->statements) {
    if (isDeclaration(statement.get())) {
      declarations.push_back(std::move(statement));
    }
  }

  return declarations;
}

bool ImportResolver::isDeclaration(ASTNode *node) {
  return node->type == NodeType::FunctionDeclaration ||
         node->type == NodeType::StructDeclaration ||
         node->type == NodeType::EnumDeclaration;
}
