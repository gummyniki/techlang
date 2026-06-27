// src/vectec_main.cpp
#include "analysis/semantic_analyzer.h"
#include "backend/gpu_generator.h"
#include "frontend/import_resolver.h"
#include "frontend/lexer.h"
#include "frontend/parser.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

std::string readFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + path);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string getOutputName(const std::string &inputPath) {
  size_t dotPos = inputPath.rfind(".vtec");
  if (dotPos != std::string::npos) {
    return inputPath.substr(0, dotPos);
  }
  return inputPath + "_out";
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: vectec <file.vtec>\n";
    std::cerr << "  Compiles a VecTec kernel file to PTX.\n";
    return 1;
  }

  std::string inputPath = argv[1];
  std::string outputPath = getOutputName(inputPath);

  std::string compilerDir =
      std::filesystem::canonical(std::filesystem::path(argv[0]).parent_path())
          .string();

  try {
    std::cout << "Reading " << inputPath << "...\n";
    std::string source = readFile(inputPath);

    std::cout << "Lexing...\n";
    Lexers::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    std::cout << "Parsing...\n";
    Parser parser(tokens);
    auto ast = parser.parse();

    std::cout << "Generating GPU IR...\n";
    GPUGenerator gpuGen;
    gpuGen.generate(ast.get());

    // save PTX output
    gpuGen.savePTX(outputPath + ".ptx");
    std::cout << "Done! PTX saved to: " << outputPath << ".ptx\n";

  } catch (std::runtime_error &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
