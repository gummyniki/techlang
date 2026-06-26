#pragma once
#include <stdexcept>
#include <string>

struct CompileError : std::exception {
  std::string message;
  int line;
  int column; // -1 if unknown

  CompileError(std::string msg, int line, int column = -1)
      : message(std::move(msg)), line(line), column(column) {}

  const char *what() const noexcept override { return message.c_str(); }
};
