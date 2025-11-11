#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"

class Node;
class exprNode;
class stmtNode;
class lvalNode;
class fdefNode;
class paramNode;
class headerNode;
class fcallNode;
class ifNode;
class Const;
class Id;
class typeClass;

class CodegenContext {
private:
    std::map<std::string, llvm::Value*> namedValues;
    std::map<std::string, llvm::Function*> builtinFunctions;

public:
    llvm::LLVMContext TheContext;
    llvm::IRBuilder<> Builder;
    std::unique_ptr<llvm::Module> TheModule;
    llvm::Function* currentFunction;

    CodegenContext();

    void generate(fdefNode* startFunc);
    llvm::Function* getBuiltin(const std::string& name);
    llvm::Type* getLLVMType(typeClass* t);
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* TheFunction, const std::string& VarName, llvm::Type* type);
    llvm::Value* findVariable(const std::string& name);
    void setVariable(const std::string& name, llvm::Value* value);
    llvm::Value* logError(const std::string& str);
    void clearNamedValues() { namedValues.clear(); }

private:
    void createBuiltinDeclarations();
};

#endif