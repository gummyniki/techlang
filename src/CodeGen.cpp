#include "CodeGen.h"

void CodeGen::generate() {}

llvm::Value *CodeGen::genExpr(ASTNode *node);
void CodeGen::genStmt(ASTNode *node);
llvm::Type *CodeGen::toLLVMType(const std::string &type);
