#include "ast_printer.h"
#include "lexer.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include <stdexcept>

int main() {
  std::string code = R"(
        function add(int a, int b) returns int {
            int result = a + b;
            return result;
        }

        int x = 5;
        if (x > 3) do {
            x += 1;
        } else do {
            x = 0;
        }
    )";

  Lexers::Lexer lexer(code);
  auto tokens = lexer.tokenize();

  Parser parser(tokens);
  auto ast = parser.parse();

  ASTPrinter printer;
  printer.print(ast.get(), "", true);

  SemanticAnalyzer analyzer;

  try {
    analyzer.analyze(ast.get());
    std::cout << "Semantic analysis passed!\n";
  } catch (std::runtime_error &e) {
    std::cerr << "Semantic error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
