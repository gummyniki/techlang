#include "gpu_runtime_generator.h"
#include <sstream>

std::string GPURuntimeGenerator::tecTypeToC(const std::string &type) {
  if (type == "int")
    return "int";
  if (type == "float")
    return "float";
  if (type == "double")
    return "double";
  if (type == "bool")
    return "int";
  if (type == "char")
    return "char";
  if (type.substr(0, 7) == "ArrayOf") {
    std::string inner = type.substr(8, type.size() - 9);
    return tecTypeToC(inner) + "*";
  }
  return "void*";
}

std::string GPURuntimeGenerator::tecTypeToCUDA(const std::string &type) {
  if (type == "int")
    return "sizeof(int)";
  if (type == "float")
    return "sizeof(float)";
  if (type == "double")
    return "sizeof(double)";
  if (type == "bool")
    return "sizeof(int)";
  if (type == "char")
    return "sizeof(char)";
  return "sizeof(void*)";
}

std::string GPURuntimeGenerator::generatePTXString() {
  std::stringstream ss;
  ss << "static const char* ptx_" << alias << " = \n";

  std::istringstream ptxStream(ptx);
  std::string line;
  while (std::getline(ptxStream, line)) {
    std::string escaped;
    for (char c : line) {
      if (c == '\\')
        escaped += "\\\\";
      else if (c == '"')
        escaped += "\\\"";
      else
        escaped += c;
    }
    ss << "    \"" << escaped << "\\n\"\n";
  }
  ss << ";\n\n";
  return ss.str();
}

std::string GPURuntimeGenerator::generateInitFunction() {
  std::stringstream ss;

  ss << "static CUmodule tec_gpu_module_" << alias << ";\n";
  for (auto &k : kernels) {
    ss << "static CUfunction tec_gpu_kernel_" << alias << "_" << k.name
       << ";\n";
  }
  ss << "\n";

  ss << "void tec_gpu_init_" << alias << "() {\n";
  ss << "    CUdevice device;\n";
  ss << "    CUcontext ctx;\n";
  ss << "    CUDA_CHECK(cuInit(0));\n";
  ss << "    CUDA_CHECK(cuDeviceGet(&device, 0));\n";
  ss << "    CUDA_CHECK(cuDevicePrimaryCtxRetain(&ctx, device));\n";
  ss << "    CUDA_CHECK(cuCtxSetCurrent(ctx));\n";

  ss << "    CUDA_CHECK(cuModuleLoadData(&tec_gpu_module_" << alias << ", ptx_"
     << alias << "));\n";

  for (auto &k : kernels) {
    ss << "    CUDA_CHECK(cuModuleGetFunction(&tec_gpu_kernel_" << alias << "_"
       << k.name << ", tec_gpu_module_" << alias << ", \"" << k.name
       << "\"));\n";
  }

  ss << "}\n\n";
  return ss.str();
}

std::string
GPURuntimeGenerator::generateKernelWrapper(const KernelInfo &kernel) {
  std::stringstream ss;

  bool returnsArray = kernel.returnType.substr(0, 7) == "ArrayOf";
  std::string innerType =
      returnsArray ? kernel.returnType.substr(8, kernel.returnType.size() - 9)
                   : kernel.returnType;
  std::string cReturnType = returnsArray ? tecTypeToC(innerType) + "*"
                                         : tecTypeToC(kernel.returnType);

  ss << cReturnType << " " << alias << "_" << kernel.name << "(";

  // parameters
  for (size_t i = 0; i < kernel.params.size(); i++) {
    auto &[type, name] = kernel.params[i];
    ss << tecTypeToC(type) << " " << name;
    // arrays need a size parameter
    if (type.substr(0, 7) == "ArrayOf") {
      ss << ", int " << name << "_size";
    }
    if (i < kernel.params.size() - 1)
      ss << ", ";
  }
  ss << ") {\n";

  std::string sizeVar = "1";
  for (auto &[type, name] : kernel.params) {
    if (type.substr(0, 7) == "ArrayOf") {
      sizeVar = name + "_size";
      break;
    }
  }

  for (auto &[type, name] : kernel.params) {
    bool isArray = type.substr(0, 7) == "ArrayOf";
    std::string inner = isArray ? type.substr(8, type.size() - 9) : type;

    ss << "    CUdeviceptr d_" << name << ";\n";
    if (isArray) {
      ss << "    CUDA_CHECK(cuMemAlloc(&d_" << name << ", " << name
         << "_size * " << tecTypeToCUDA(inner) << "));\n";
      ss << "    CUDA_CHECK(cuMemcpyHtoD(d_" << name << ", " << name << ", "
         << name << "_size * " << tecTypeToCUDA(inner) << "));\n";
    } else {
      ss << "    CUDA_CHECK(cuMemAlloc(&d_" << name << ", "
         << tecTypeToCUDA(type) << "));\n";
      ss << "    CUDA_CHECK(cuMemcpyHtoD(d_" << name << ", &" << name << ", "
         << tecTypeToCUDA(type) << "));\n";
    }
  }

  if (kernel.returnType != "none") {
    ss << "    CUdeviceptr d_result;\n";
    if (returnsArray) {
      ss << "    CUDA_CHECK(cuMemAlloc(&d_result, " << sizeVar << " * "
         << tecTypeToCUDA(innerType) << "));\n";
    } else {
      ss << "    CUDA_CHECK(cuMemAlloc(&d_result, "
         << tecTypeToCUDA(kernel.returnType) << "));\n";
    }
  }

  ss << "    void* args[] = {";
  for (auto &[type, name] : kernel.params) {
    ss << "&d_" << name << ", ";
  }
  if (kernel.returnType != "none") {
    ss << "&d_result";
  }
  ss << "};\n";

  ss << "    int threadsPerBlock = " << sizeVar << " < 256 ? " << sizeVar
     << " : 256;\n";
  ss << "    int numBlocks = (" << sizeVar
     << " + threadsPerBlock - 1) / threadsPerBlock;\n";

  ss << "    CUDA_CHECK(cuLaunchKernel(tec_gpu_kernel_" << alias << "_"
     << kernel.name << ",\n";
  ss << "        numBlocks, 1, 1,\n";
  ss << "        threadsPerBlock, 1, 1,\n";
  ss << "        0, 0, args, 0));\n";
  ss << "    CUDA_CHECK(cuCtxSynchronize());\n\n";

  for (auto &[type, name] : kernel.params) {
    if (type.substr(0, 7) == "ArrayOf") {
      std::string inner = type.substr(8, type.size() - 9);
      ss << "    cuMemcpyDtoH(" << name << ", d_" << name << ", " << name
         << "_size * " << tecTypeToCUDA(inner) << ");\n";
    }
  }

  if (kernel.returnType != "none") {
    if (returnsArray) {
      ss << "    " << tecTypeToC(innerType) << "* result = ("
         << tecTypeToC(innerType) << "*)malloc(" << sizeVar << " * "
         << tecTypeToCUDA(innerType) << ");\n";
      ss << "    CUDA_CHECK(cuMemcpyDtoH(result, d_result, " << sizeVar << " * "
         << tecTypeToCUDA(innerType) << "));\n";
    } else {
      ss << "    " << cReturnType << " result;\n";
      ss << "    CUDA_CHECK(cuMemcpyDtoH(&result, d_result, "
         << tecTypeToCUDA(kernel.returnType) << "));\n";
    }
  }

  for (auto &[type, name] : kernel.params) {
    ss << "    CUDA_CHECK(cuMemFree(d_" << name << "));\n";
  }
  if (kernel.returnType != "none") {
    ss << "    CUDA_CHECK(cuMemFree(d_result));\n";
  }

  if (kernel.returnType != "none") {
    ss << "    return result;\n";
  }

  ss << "}\n\n";
  return ss.str();
}

std::string GPURuntimeGenerator::generate() {
  std::stringstream ss;

  ss << "#include <cuda.h>\n";
  ss << "#include <stdlib.h>\n";
  ss << "#include <string.h>\n";
  ss << "#include <stdio.h>\n\n";
  ss << "#define CUDA_CHECK(x) do { CUresult _r = (x); if (_r != CUDA_SUCCESS) "
        "{ const char* _s; cuGetErrorString(_r, &_s); fprintf(stderr, \"CUDA "
        "error at %s:%d: %s\\n\", __FILE__, __LINE__, _s); } } while(0)\n\n";

  ss << generatePTXString();

  ss << generateInitFunction();

  for (auto &kernel : kernels) {
    ss << generateKernelWrapper(kernel);
  }

  return ss.str();
}
