#include "import_resolver.h"
#include <filesystem>
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
  if (node->filename.size() > 5 &&
      node->filename.substr(node->filename.size() - 5) == ".vtec") {
    resolveVecTecImport(node);
    return;
  }

  std::string fullPath = baseDir + "/" + node->filename;

  if (!std::filesystem::exists(fullPath)) {
    fullPath = compilerDir + "/" + node->filename;
  }

  if (!std::filesystem::exists(fullPath)) {
    fullPath = "/usr/local/lib/" + node->filename;
  }

  if (!std::filesystem::exists(fullPath)) {
    throw std::runtime_error("Cannot find import file: " + node->filename +
                             "\n"
                             "Looked in:\n"
                             "  " +
                             baseDir + "/" + node->filename +
                             "\n"
                             "  " +
                             compilerDir + "/" + node->filename +
                             "\n"
                             "  /usr/local/lib/" +
                             node->filename);
  }

  if (alreadyImported.count(fullPath))
    return;
  alreadyImported.insert(fullPath);

  auto declarations = parseFile(fullPath);
  prefixDeclarations(declarations, node->alias);

  for (auto &decl : declarations) {
    node->declarations.push_back(std::move(decl));
  }
}

void ImportResolver::resolveVecTecImport(ImportNode *node) {
  // find the .vtec file
  std::string vtecPath = baseDir + "/" + node->filename;
  if (!std::filesystem::exists(vtecPath)) {
    vtecPath = compilerDir + "/" + node->filename;
  }
  if (!std::filesystem::exists(vtecPath)) {
    throw std::runtime_error("Cannot find VecTec file: " + node->filename);
  }

  size_t dotPos = vtecPath.rfind(".vtec");
  if (dotPos != std::string::npos) {
    vtecPath = vtecPath.substr(0, dotPos);
  }
  // compile to PTX using vectec compiler
  std::string ptxPath = vtecPath + ".ptx";
  std::string vectecCmd = compilerDir + "/vectec " + vtecPath + ".vtec";
  int result = std::system(vectecCmd.c_str());
  if (result != 0) {
    throw std::runtime_error("Failed to compile VecTec file: " +
                             node->filename);
  }

  // read the PTX
  std::ifstream ptxFile(ptxPath);
  if (!ptxFile.is_open()) {
    throw std::runtime_error("Could not read PTX: " + ptxPath);
  }
  std::stringstream ptxBuffer;
  ptxBuffer << ptxFile.rdbuf();
  std::string ptx = ptxBuffer.str();

  // parse the .vtec file to extract kernel signatures
  std::string vtecPathFull = vtecPath + ".vtec";
  std::ifstream vtecFile(vtecPath + ".vtec");
  std::stringstream vtecBuffer;
  vtecBuffer << vtecFile.rdbuf();
  Lexers::Lexer lexer(vtecBuffer.str());
  auto tokens = lexer.tokenize();
  Parser parser(tokens);
  auto ast = parser.parse();

  // extract kernel info
  std::vector<KernelInfo> kernels = extractKernelInfo(ast.get());

  // generate C wrapper
  GPURuntimeGenerator runtimeGen(node->alias, ptx, kernels);
  std::string wrapperCode = runtimeGen.generate();

  // write wrapper to file
  std::string wrapperPath = ptxPath + "_runtime.c";
  std::ofstream wrapperFile(wrapperPath);
  wrapperFile << wrapperCode;
  wrapperFile.close();

  // compile wrapper with nvcc
  std::string nvccCmd = "nvcc -c " + wrapperPath + " -o " + ptxPath +
                        "_runtime.o" + " -I/opt/cuda/include";
  result = std::system(nvccCmd.c_str());
  if (result != 0) {
    throw std::runtime_error("Failed to compile GPU runtime wrapper");
  }

  gpuWrapperFiles.push_back(ptxPath + "_runtime.o");

  // register kernel functions in the import node
  // so the semantic analyzer and IR generator know about them
  for (auto &kernel : kernels) {
    auto funcNode = std::make_unique<FunctionDeclarationNode>(0);
    funcNode->name = node->alias + "." + kernel.name;
    funcNode->returnType = kernel.returnType;
    funcNode->params = kernel.params;
    funcNode->externSymbol = node->alias + "_" + kernel.name;
    funcNode->body.clear();
    node->declarations.push_back(std::move(funcNode));
  }
}

std::vector<KernelInfo> ImportResolver::extractKernelInfo(ProgramNode *ast) {

  std::vector<KernelInfo> kernels;
  for (auto &statement : ast->statements) {
    if (statement->type == NodeType::KernelDeclaration) {
      auto *k = static_cast<KernelDeclarationNode *>(statement.get());
      KernelInfo info;
      info.name = k->name;
      info.returnType = k->returnType;
      info.params = k->params;
      kernels.push_back(info);
    }
  }
  return kernels;
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
