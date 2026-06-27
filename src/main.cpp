#include "analysis/semantic_analyzer.h"
#include "backend/ir_generator.h"
#include "debug/error.h"
#include "frontend/import_resolver.h"
#include "frontend/lexer.h"
#include "frontend/parser.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Triple.h"
#include <llvm/TargetParser/Host.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

std::string readFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string findStdLib(const std::string &compilerDir,
                       const std::string &filename) {
  std::string localPath = compilerDir + "/" + filename;
  if (std::filesystem::exists(localPath))
    return localPath;

  std::string systemPath = "/usr/local/lib/" + filename;
  if (std::filesystem::exists(systemPath))
    return systemPath;

  std::string cwdPath = "./" + filename;
  if (std::filesystem::exists(cwdPath))
    return cwdPath;

  throw std::runtime_error("Could not find " + filename +
                           "\n"
                           "Looked in:\n"
                           "  " +
                           localPath +
                           "\n"
                           "  " +
                           systemPath +
                           "\n"
                           "  " +
                           cwdPath);
}

void linkToExecutable(const std::string &objPath, const std::string &exePath,
                      const std::string &compilerDir,
                      const std::vector<std::string> &gpuWrappers) {

  std::string stdLib = findStdLib(compilerDir, "stdlib.o");

  std::string cmd = "gcc " + objPath + " " + stdLib;

  for (auto &wrapper : gpuWrappers) {
    cmd += " " + wrapper;
  }

  if (!gpuWrappers.empty()) {
    cmd += " -L/opt/cuda/lib64 -lcuda";
  }

  cmd += " -o " + exePath + " -lm";

  int result = std::system(cmd.c_str());
  if (result != 0) {
    throw std::runtime_error("Linking failed!");
  }

  std::remove(objPath.c_str());
}

std::string getOutputName(const std::string &inputPath) {
  // strip the .tec extension to get the output name
  size_t dotPos = inputPath.rfind(".tec");
  if (dotPos != std::string::npos) {
    return inputPath.substr(0, dotPos);
  }
  return inputPath + "_out";
}

void compileToObject(llvm::Module *module, const std::string &outputPath) {
  // initialize all LLVM targets
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  // get the target triple for the current machine
  std::string targetTriple = llvm::sys::getDefaultTargetTriple();
  llvm::Triple triple(targetTriple);
  module->setTargetTriple(triple);

  std::string error;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(targetTriple, error);
  if (!target) {
    throw std::runtime_error("Failed to find target: " + error);
  }

  // generic CPU, no special features
  llvm::TargetOptions opt;
  llvm::TargetMachine *targetMachine =
      target->createTargetMachine(triple,
                                  "generic", // cpu
                                  "",        // features
                                  opt, llvm::Reloc::PIC_);

  // set data layout so LLVM knows struct sizes, alignment etc
  module->setDataLayout(targetMachine->createDataLayout());

  // write object file
  std::string objPath = outputPath + ".o";
  std::error_code ec;
  llvm::raw_fd_ostream objFile(objPath, ec, llvm::sys::fs::OF_None);
  if (ec) {
    throw std::runtime_error("Could not open object file: " + ec.message());
  }

  llvm::legacy::PassManager passManager;
  if (targetMachine->addPassesToEmitFile(passManager, objFile, nullptr,
                                         llvm::CodeGenFileType::ObjectFile)) {
    throw std::runtime_error("Target machine can't emit object files");
  }

  passManager.run(*module);
  objFile.flush();

  delete targetMachine;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: techlang <file.tec>\n";
    return 1;
  }

  // get the directory the compiler binary lives in
  std::string compilerDir =
      std::filesystem::canonical(std::filesystem::path(argv[0]).parent_path())
          .string();

  std::string inputPath = argv[1];
  std::string outputPath = getOutputName(inputPath);

  std::string source;
  try {
    // 1. read source file
    std::cout << "Reading " << inputPath << "...\n";
    source = readFile(inputPath);

    // 2. lex
    std::cout << "Lexing...\n";
    Lexers::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    // 3. parse
    std::cout << "Parsing...\n";
    Parser parser(tokens);
    auto ast = parser.parse();

    ImportResolver resolver(
        std::filesystem::path(inputPath).parent_path().string(), compilerDir);
    resolver.resolve(ast.get());

    // 4. semantic analysis
    std::cout << "Analyzing...\n";
    SemanticAnalyzer analyzer;
    analyzer.analyze(ast.get());

    // 5. IR generation
    std::cout << "Generating IR...\n";
    IRGenerator irGen;

    for (auto &statement : ast->statements) {
      if (statement->type == NodeType::Import) {
        auto *importNode = static_cast<ImportNode *>(statement.get());
        if (importNode->filename.size() > 5 &&
            importNode->filename.substr(importNode->filename.size() - 5) ==
                ".vtec") {
          irGen.registerGPUAlias(importNode->alias);
        }
      }
    }

    irGen.generate(ast.get());

    // optional: save the .ll file for debugging
    irGen.saveToFile(outputPath + ".ll");

    // 6. compile to object file
    std::cout << "Compiling...\n";
    compileToObject(irGen.getModule(), outputPath);

    // 7. link to executable
    std::cout << "Linking...\n";
    linkToExecutable(outputPath + ".o", outputPath, compilerDir,
                     resolver.gpuWrapperFiles);

    std::cout << "Done! Binary: ./" << outputPath << "\n";

  } catch (CompileError &e) {
    std::cerr << "Error on line " << e.line << ": " << e.message << "\n";

    // split source into lines and show the offending line with a pointer
    std::vector<std::string> lines;
    std::istringstream stream(source);
    std::string ln;
    while (std::getline(stream, ln))
      lines.push_back(ln);

    if (e.line >= 1 && e.line <= (int)lines.size()) {
      std::cerr << "  " << lines[e.line - 1] << "\n";
      if (e.column >= 1)
        std::cerr << "  " << std::string(e.column - 1, ' ') << "^\n";
    }

    return 1;
  } catch (std::runtime_error &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
