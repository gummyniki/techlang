#include <stdexcept>

#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct Symbol {
  std::string name;
  std::string type;
  bool isConst = false;
  bool isFunction = false;
  std::vector<std::string> paramTypes; // for functions
  std::string returnType;              // for functions
};

class SymbolTable {
public:
  // push a new scope (entering a block or function)
  void pushScope() { scopes.push_back({}); }

  // pop the current scope (leaving a block or function)
  void popScope() {
    if (scopes.empty())
      throw std::runtime_error("No scope to pop!");
    scopes.pop_back();
  }

  // declare a new symbol in the current scope
  void declare(const Symbol &symbol) {
    auto &current = scopes.back();
    if (current.count(symbol.name)) {
      throw std::runtime_error("'" + symbol.name +
                               "' is already declared in this scope");
    }
    current[symbol.name] = symbol;
  }

  // look up a symbol, searching from innermost to outermost scope
  std::optional<Symbol> lookup(const std::string &name) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
      auto it = scopes[i].find(name);
      if (it != scopes[i].end()) {
        return it->second;
      }
    }
    return std::nullopt; // not found
  }

  // check if a symbol exists only in current scope (for duplicate detection)
  bool existsInCurrentScope(const std::string &name) {
    return scopes.back().count(name) > 0;
  }

private:
  // stack of scopes, each scope is a map of name -> symbol
  std::vector<std::unordered_map<std::string, Symbol>> scopes;
};
