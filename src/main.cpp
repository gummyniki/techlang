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

struct CompilerOptions {
  std::string inputFile;
  std::string outputFile;
  std::vector<std::string> cFiles;
  std::vector<std::string> linkLibs;
  std::vector<std::string> linkDirs;
  bool emitLLVM = false;
  bool verbose = false;
};

std::string getOutputName(const std::string &inputPath) {
  size_t dotPos = inputPath.rfind(".tec");
  if (dotPos != std::string::npos) {
    return inputPath.substr(0, dotPos);
  }
  return inputPath + "_out";
}

CompilerOptions parseArgs(int argc, char *argv[]) {
  CompilerOptions opts;

  if (argc < 2) {
    std::cerr << "Usage: techlang <file.tec> [options]\n";
    std::cerr << "Options:\n";
    std::cerr << "  -o <file>       output binary name\n";
    std::cerr << "  -c <file.c>     compile and link a C file\n";
    std::cerr << "  -l <lib>        link a library (e.g. -l vulkan)\n";
    std::cerr << "  -L <dir>        add library search directory\n";
    std::cerr << "  --emit-llvm     save .ll file alongside binary\n";
    std::cerr << "  --verbose       print compilation steps\n";
    exit(1);
  }

  opts.inputFile = argv[1];

  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-o" && i + 1 < argc) {
      opts.outputFile = argv[++i];
    } else if (arg == "-c" && i + 1 < argc) {
      opts.cFiles.push_back(argv[++i]);
    } else if (arg == "-l" && i + 1 < argc) {
      opts.linkLibs.push_back(argv[++i]);
    } else if (arg == "-L" && i + 1 < argc) {
      opts.linkDirs.push_back(argv[++i]);
    } else if (arg == "--emit-llvm") {
      opts.emitLLVM = true;
    } else if (arg == "--verbose") {
      opts.verbose = true;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      exit(1);
    }
  }

  if (opts.outputFile.empty()) {
    opts.outputFile = getOutputName(opts.inputFile);
  }

  return opts;
}

std::string getCompilerDir(const char *argv0) {
  std::filesystem::path execPath;

  if (std::string(argv0).find('/') != std::string::npos) {
    execPath = std::filesystem::canonical(argv0);
  } else {
    // search PATH for the executable
    const char *pathEnv = std::getenv("PATH");
    if (pathEnv) {
      std::stringstream ss(pathEnv);
      std::string dir;
      while (std::getline(ss, dir, ':')) {
        std::filesystem::path candidate = std::filesystem::path(dir) / argv0;
        if (std::filesystem::exists(candidate)) {
          execPath = std::filesystem::canonical(candidate);
          break;
        }
      }
    }
  }

  if (execPath.empty()) {
    return std::filesystem::current_path().string();
  }

  return execPath.parent_path().string();
}

std::string compileCFile(const std::string &cFile, const std::string &buildDir,
                         bool verbose) {
  std::string objFile =
      buildDir + "/" + std::filesystem::path(cFile).stem().string() + ".o";

  std::string cmd = "gcc -c " + cFile + " -o " + objFile;

  if (verbose)
    std::cout << "Compiling C: " << cmd << "\n";

  int result = std::system(cmd.c_str());
  if (result != 0) {
    throw std::runtime_error("Failed to compile C file: " + cFile);
  }

  return objFile;
}

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
                      const std::vector<std::string> &extraObjs,
                      const std::vector<std::string> &linkLibs,
                      const std::vector<std::string> &linkDirs,
                      const std::vector<std::string> &gpuWrappers,
                      bool verbose) {

  std::string stdLib = compilerDir + "/stdlib.o";
  if (!std::filesystem::exists(stdLib)) {
    stdLib = "/usr/local/lib/stdlib.o";
  }

  std::string cmd = "gcc " + objPath + " " + stdLib;

  for (auto &obj : extraObjs) {
    cmd += " " + obj;
  }

  for (auto &wrapper : gpuWrappers) {
    cmd += " " + wrapper;
  }

  for (auto &dir : linkDirs) {
    cmd += " -L" + dir;
  }

  for (auto &lib : linkLibs) {
    cmd += " -l" + lib;
  }

  if (!gpuWrappers.empty()) {
    cmd += " -L/opt/cuda/lib64 -lcuda";
  }

  cmd += " -o " + exePath + " -lm";

  if (verbose)
    std::cout << "Linking: " << cmd << "\n";

  int result = std::system(cmd.c_str());
  if (result != 0) {
    throw std::runtime_error("Linking failed!");
  }

  std::remove(objPath.c_str());
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
  CompilerOptions opts = parseArgs(argc, argv);

  std::string compilerDir = getCompilerDir(argv[0]);

  // temp build directory for intermediate files
  std::string buildDir = "/tmp/techlang_build";
  std::filesystem::create_directories(buildDir);

  try {
    if (opts.verbose)
      std::cout << "Reading " << opts.inputFile << "...\n";
    std::string source = readFile(opts.inputFile);

    if (opts.verbose)
      std::cout << "Lexing...\n";
    Lexers::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    if (opts.verbose)
      std::cout << "Parsing...\n";
    Parser parser(tokens);
    auto ast = parser.parse();

    if (opts.verbose)
      std::cout << "Resolving imports...\n";

    std::string inputDir =
        std::filesystem::path(opts.inputFile).parent_path().string();

    if (inputDir.empty()) {
      inputDir = std::filesystem::current_path().string();
    }

    ImportResolver resolver(inputDir, compilerDir);
    resolver.resolve(ast.get());

    if (opts.verbose)
      std::cout << "Analyzing...\n";
    SemanticAnalyzer analyzer;
    analyzer.analyze(ast.get());

    if (opts.verbose)
      std::cout << "Generating IR...\n";
    IRGenerator irGen;
    for (auto &alias : resolver.gpuAliases) {
      irGen.registerGPUAlias(alias);
    }
    irGen.generate(ast.get());

    if (opts.emitLLVM) {
      irGen.saveToFile(opts.outputFile + ".ll");
    }

    if (opts.verbose)
      std::cout << "Compiling...\n";
    compileToObject(irGen.getModule(), buildDir + "/output");

    // compile any -c C files
    std::vector<std::string> extraObjs;
    for (auto &cFile : opts.cFiles) {
      extraObjs.push_back(compileCFile(cFile, buildDir, opts.verbose));
    }

    if (opts.verbose)
      std::cout << "Linking...\n";
    linkToExecutable(buildDir + "/output.o", opts.outputFile, compilerDir,
                     extraObjs, opts.linkLibs, opts.linkDirs,
                     resolver.gpuWrapperFiles, opts.verbose);

    if (opts.verbose)
      std::cout << "Done! Binary: ./" << opts.outputFile << "\n";

  } catch (CompileError &e) {
    std::cerr << "Error on line " << e.line << ": " << e.message << "\n";

    // split source into lines and show the offending line with a pointer
    std::string source = readFile(opts.inputFile);
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
