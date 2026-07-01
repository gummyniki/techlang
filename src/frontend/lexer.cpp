#include "lexer.h"
#include "../debug/error.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

bool isNumber(const std::string &s) {
  if (s.empty())
    return false;

  size_t pos;
  try {
    std::stod(s, &pos);
    return pos == s.size();
  } catch (...) {
    return false;
  }
}

namespace Lexers {
char Lexer::current() {
  if (isAtEnd())
    return '\0';
  return source[pos];
}

char Lexer::advance() {
  char c = current();
  ++pos;
  ++column;

  if (c == '\n') {
    line++;
    column = 1;
  }

  return c;
}

char Lexer::peek(int offset) {
  int target = pos + offset;

  if (target > source.size())
    return '\0';

  return source[target];
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;

  while (!isAtEnd()) {
    skipWhitespace();
    if (isAtEnd())
      break;

    // skip comments
    if (current() == '/' && peek() == '/') {
      skipComment();
      continue;
    }

    char c = current();

    // Numbers
    if (isdigit(c)) {
      tokens.push_back(readNumber());
    }
    // Strings
    else if (c == '"') {
      tokens.push_back(readString());
    }
    // Chars
    else if (c == '\'') {
      tokens.push_back(readChar());
    }
    // Identifiers and keywords
    else if (isalpha(c) || c == '_') {
      tokens.push_back(readIdentifierOrKeyword());
    }
    // ! for !import, but don't shadow the '!=' operator
    else if (c == '!' && peek() != '=') {
      tokens.push_back(makeToken(TokenType::EXCLAIM, "!"));
      advance();
    } else {
      tokens.push_back(readOperator());
    }
  }

  tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
  return tokens;
}

bool Lexer::isAtEnd() { return pos >= source.size(); }

void Lexer::skipWhitespace() {

  while (!isAtEnd() && (current() == ' ' || current() == '\t' ||
                        current() == '\r' || current() == '\n')) {
    advance();
  }
}

void Lexer::skipComment() {
  while (!isAtEnd() && current() != '\n') {
    advance();
  }
}

Token Lexer::readIdentifierOrKeyword() {
  std::string word;
  while (!isAtEnd() && (isalnum(current()) || current() == '_')) {
    word += advance();
  }

  auto it = keywords.find(word);
  TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;

  return makeToken(type, word);
}

Token Lexer::readNumber() {
  std::string word;

  while (!isAtEnd() && (isalnum(current()) || current() == '.')) {
    word += advance();
  }

  if (!isNumber(word)) {
    std::cout << "Not a number!\n";
    throw CompileError("invalid number '" + word + "'", line, column);
  }

  TokenType tokenType;

  if (word.find(".") != std::string::npos) {
    tokenType = TokenType::FLOAT_LITERAL;

  } else {
    tokenType = TokenType::INT_LITERAL;
  }

  return makeToken(tokenType, word);
}

Token Lexer::readString() {
  advance(); // consume opening "
  std::string value;

  while (!isAtEnd() && current() != '"') {
    if (current() == '\\') {
      advance();
      switch (current()) {
      case 'n':
        value += '\n';
        advance();
        break; // newline
      case 't':
        value += '\t';
        advance();
        break; // tab
      case '"':
        value += '"';
        advance();
        break; // quote
      case '\\':
        value += '\\';
        advance();
        break; // backslash
      case '0':
        value += '\0';
        advance();
        break; // null char
      default:
        throw CompileError("unknown escape sequence '\\" +
                               std::string(1, current()) + "'",
                           line, column);
      }
    } else {
      value += advance();
    }
  }

  if (isAtEnd()) {
    throw CompileError("unterminated string literal", line, column);
  }

  advance();
  return makeToken(TokenType::STRING_LITERAL, value);
}

Token Lexer::readOperator() {
  std::string word;
  word += advance();

  if (word[0] == '=' && current() == '=') {
    word += advance();
    return makeToken(TokenType::EQUALS_EQUALS, word);
  }
  if (word[0] == '+' && current() == '=') {
    word += advance();
    return makeToken(TokenType::PLUS_EQUALS, word);
  }
  if (word[0] == '-' && current() == '=') {
    word += advance();
    return makeToken(TokenType::MINUS_EQUALS, word);
  }
  if (word[0] == '!' && current() == '=') {
    word += advance();
    return makeToken(TokenType::BANG_EQUALS, word);
  }
  if (word[0] == '>' && current() == '=') {
    word += advance();
    return makeToken(TokenType::GREATER_EQUALS, word);
  }
  if (word[0] == '<' && current() == '=') {
    word += advance();
    return makeToken(TokenType::LESS_EQUALS, word);
  }
  if (word[0] == '&' && current() == '&') {
    word += advance();
    return makeToken(TokenType::AND_AND, word);
  }
  if (word[0] == '|' && current() == '|') {
    word += advance();
    return makeToken(TokenType::PIPE_PIPE, word);
  }
  if (word[0] == '*' && current() == '=') {
    word += advance();
    return makeToken(TokenType::STAR_EQUALS, word);
  }
  if (word[0] == '/' && current() == '=') {
    word += advance();
    return makeToken(TokenType::SLASH_EQUALS, word);
  }

  switch (word[0]) {
  case '=':
    return makeToken(TokenType::EQUALS, word);
  case '+':
    return makeToken(TokenType::PLUS, word);
  case '-':
    return makeToken(TokenType::MINUS, word);
  case '*':
    return makeToken(TokenType::STAR, word);
  case '/':
    return makeToken(TokenType::SLASH, word);
  case '%':
    return makeToken(TokenType::PERCENT, word);
  case '>':
    return makeToken(TokenType::GREATER, word);
  case '<':
    return makeToken(TokenType::LESS, word);
  case '!':
    return makeToken(TokenType::BANG, word);
  case ';':
    return makeToken(TokenType::SEMICOLON, word);
  case ',':
    return makeToken(TokenType::COMMA, word);
  case '.':
    return makeToken(TokenType::DOT, word);
  case '(':
    return makeToken(TokenType::LPAREN, word);
  case ')':
    return makeToken(TokenType::RPAREN, word);
  case '{':
    return makeToken(TokenType::LBRACE, word);
  case '}':
    return makeToken(TokenType::RBRACE, word);
  case '[':
    return makeToken(TokenType::LBRACKET, word);
  case ']':
    return makeToken(TokenType::RBRACKET, word);
  default:
    throw CompileError("unknown character '" + std::string(1, word[0]) + "'",
                       line, column);
  }
}
Token Lexer::readChar() {
  advance();
  std::string value;

  if (current() == '\\') {
    // escape sequence in a char
    advance(); // consume backslash
    switch (current()) {
    case 'n':
      value = "\n";
      advance();
      break;
    case 't':
      value = "\t";
      advance();
      break;
    case '\'':
      value = "'";
      advance();
      break;
    case '\\':
      value = "\\";
      advance();
      break;
    default:
      throw CompileError("unknown escape sequence in char literal", line,
                         column);
    }
  } else {
    value += advance();
  }

  if (current() != '\'') {
    throw CompileError("char literal must contain exactly one character", line,
                       column);
  }

  advance();
  return makeToken(TokenType::CHAR_LITERAL, value);
}
Token Lexer::makeToken(TokenType type, std::string value) {
  Token token(type, value, line, column);

  return token;
}

} // namespace Lexers
