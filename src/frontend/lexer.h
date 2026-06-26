#pragma once

#include "token.h"
#include <vector>
using namespace Tokens;

namespace Lexers {
class Lexer {
public:
  Lexer(const std::string &source)
      : source(source), pos(0), line(1), column(1) {}

  std::vector<Token> tokenize();

  std::string source;
  size_t pos;
  int line, column;

  char current();
  char advance();
  char peek(int offset = 1);
  bool isAtEnd();

  void skipWhitespace();
  void skipComment();

  Token readIdentifierOrKeyword();
  Token readNumber();
  Token readString();
  Token readChar();
  Token readOperator();
  Token readPunctuation();
  Token makeToken(TokenType type, std::string value);
};
} // namespace Lexers
