#pragma once
#include "../ast.h"
#include <string>
#include <vector>

struct KernelInfo {
  std::string name;
  std::string returnType;
  std::vector<std::pair<std::string, std::string>> params;
};

class GPURuntimeGenerator {
public:
  GPURuntimeGenerator(const std::string &alias, const std::string &ptx,
                      const std::vector<KernelInfo> &kernels)
      : alias(alias), ptx(ptx), kernels(kernels) {}

  std::string generate();

private:
  std::string alias;
  std::string ptx;
  std::vector<KernelInfo> kernels;

  std::string generatePTXString();
  std::string generateInitFunction();
  std::string generateKernelWrapper(const KernelInfo &kernel);
  std::string tecTypeToC(const std::string &type);
  std::string tecTypeToCUDA(const std::string &type);
};
