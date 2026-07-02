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
  KW_INT8,
  KW_INT16,
  KW_INT32, // normal int
  KW_INT64,
  KW_UINT8,
  KW_UINT16,
  KW_UINT32,
  KW_UINT64,
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
  KW_ANY,
  KW_FIXED,
  KW_ARRAYOF,
  KW_POINTEROF,
  KW_EXTERN,
  KW_KERNEL,
  KW_SHARED,
  KW_THREADID,

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
  AMPERSAND,
  PIPE,
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
  EXCLAIM, // !
  END_OF_FILE
};

static std::unordered_map<std::string, TokenType> keywords = {
    {"kernel", TokenType::KW_KERNEL},
    {"shared", TokenType::KW_SHARED},
    {"any", TokenType::KW_ANY},
    {"int", TokenType::KW_INT32},
    {"int32", TokenType::KW_INT32},
    {"uint", TokenType::KW_UINT32},
    {"int8", TokenType::KW_INT8},
    {"int16", TokenType::KW_INT16},
    {"int64", TokenType::KW_INT64},
    {"uint8", TokenType::KW_UINT8},
    {"uint16", TokenType::KW_UINT16},
    {"uint32", TokenType::KW_UINT32},
    {"uint64", TokenType::KW_UINT64},
    {"char", TokenType::KW_CHAR},
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
    {"extern", TokenType::KW_EXTERN},
    // etc
}; // namespace Tokens

struct Token {
  TokenType type;
  std::string value;
  int line;
  int column;

  Token(TokenType t, std::string v, int l, int c)
      : type(t), value(v), line(l), column(c) {}
};
} // namespace Tokens
