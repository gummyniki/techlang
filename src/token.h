#pragma once

#include <iostream>
#include <unordered_map>

namespace Tokens {
enum class TokenType {
  // Literals
  INT_LITERAL,
  FLOAT_LITERAL,
  DOUBLE_LITERAL,
  STRING_LITERAL,
  CHAR_LITERAL,
  BOOL_LITERAL,

  // Keywords
  KW_INT,
  KW_FLOAT,
  KW_DOUBLE,
  KW_CHAR,
  KW_STRING,
  KW_BOOL,
  KW_STRUCT,
  KW_ENUM,
  KW_LET,
  KW_CONST,
  KW_FUNCTION,
  KW_RETURNS,
  KW_RETURN,
  KW_IF,
  KW_ELSE,
  KW_DO,
  KW_FOR,
  KW_WHILE,
  KW_NONE,
  KW_IMPORT,
  KW_AS,
  KW_FIXED,
  KW_ARRAYOF,
  KW_POINTEROF,

  // Identifiers (variable names, function names)
  IDENTIFIER,

  // Operators
  PLUS,
  MINUS,
  STAR,
  SLASH,
  PERCENT,
  PLUS_EQUALS,
  MINUS_EQUALS,
  STAR_EQUALS,
  SLASH_EQUALS,
  EQUALS,
  EQUALS_EQUALS,
  BANG_EQUALS,
  LESS,
  GREATER,
  LESS_EQUALS,
  GREATER_EQUALS,
  AND_AND,
  PIPE_PIPE,
  BANG,

  // Punctuation
  SEMICOLON,
  COMMA,
  DOT,
  LPAREN,
  RPAREN, // ( )
  LBRACE,
  RBRACE, // { }
  LBRACKET,
  RBRACKET, // [ ]

  // Special
  EXCLAIM, // ! for !import
  END_OF_FILE
};

static std::unordered_map<std::string, TokenType> keywords = {
    {"int", TokenType::KW_INT},
    {"float", TokenType::KW_FLOAT},
    {"double", TokenType::KW_DOUBLE},
    {"string", TokenType::KW_STRING},
    {"bool", TokenType::KW_BOOL},
    {"function", TokenType::KW_FUNCTION},
    {"returns", TokenType::KW_RETURNS},
    {"return", TokenType::KW_RETURN},
    {"if", TokenType::KW_IF},
    {"else", TokenType::KW_ELSE},
    {"do", TokenType::KW_DO},
    {"while", TokenType::KW_WHILE},
    {"for", TokenType::KW_FOR},
    {"struct", TokenType::KW_STRUCT},
    {"enum", TokenType::KW_ENUM},
    {"none", TokenType::KW_NONE},
    {"as", TokenType::KW_AS},
    {"true", TokenType::BOOL_LITERAL},
    {"false", TokenType::BOOL_LITERAL},
    {"ArrayOf", TokenType::KW_ARRAYOF},
    {"PointerOf", TokenType::KW_POINTEROF},
    // etc
};

struct Token {
  TokenType type;
  std::string value; // the actual text
  int line;          // for error messages!
  int column;

  Token(TokenType t, std::string v, int l, int c)
      : type(t), value(v), line(l), column(c) {}
};
} // namespace Tokens
