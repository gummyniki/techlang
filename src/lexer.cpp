#include "lexer.h"
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
    // ! for !import
    else if (c == '!') {
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

  // check if it's a keyword
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
    throw std::runtime_error("Line " + std::to_string(line) +
                             ": invalid number '" + word + "'");
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
    // handle escape sequences
    if (current() == '\\') {
      advance(); // consume the backslash
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
        throw std::runtime_error("Line " + std::to_string(line) +
                                 ": unknown escape sequence '\\" + current() +
                                 "'");
      }
    } else {
      value += advance();
    }
  }

  if (isAtEnd()) {
    throw std::runtime_error("Line " + std::to_string(line) +
                             ": unterminated string literal");
  }

  advance(); // consume closing "
  return makeToken(TokenType::STRING_LITERAL, value);
}

Token Lexer::readOperator() {
  std::string word;
  word += advance(); // consume first char, word[0] is now that char

  // two-character operators: check word[0] for first, current() for second
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

  // single-character: switch on word[0], the char we already consumed
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
    throw std::runtime_error("Line " + std::to_string(line) +
                             ": unknown character '" + word[0] + "'");
  }
}
Token Lexer::readChar() {
  advance(); // consume opening '
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
      throw std::runtime_error("Line " + std::to_string(line) +
                               ": unknown escape sequence in char literal");
    }
  } else {
    value += advance();
  }

  // must be followed by closing '
  if (current() != '\'') {
    throw std::runtime_error(
        "Line " + std::to_string(line) +
        ": char literal must contain exactly one character");
  }

  advance(); // consume closing '
  return makeToken(TokenType::CHAR_LITERAL, value);
}
Token Lexer::makeToken(TokenType type, std::string value) {
  Token token(type, value, line, column);

  return token;
}

} // namespace Lexers
