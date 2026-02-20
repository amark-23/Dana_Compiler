#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stack>

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
#include "llvm/IR/Instructions.h"

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
public:
    std::vector<std::map<std::string, llvm::Value*>> namedValuesStack;
    std::map<std::string, llvm::Function*> builtinFunctions;
    std::stack<std::pair<std::string, llvm::BasicBlock*>> breakBlockStack;
    std::stack<std::pair<std::string, llvm::BasicBlock*>> continueBlockStack;
    std::vector<std::string> functionNameStack;  // Track nested function names
    std::vector<std::map<std::string, std::string>> localFunctionsStack;  // Map simple names to qualified names

public:
    llvm::LLVMContext TheContext;
    llvm::IRBuilder<> Builder;
    std::unique_ptr<llvm::Module> TheModule;
    llvm::Function* currentFunction; 
    fdefNode* MainFunctionNode = nullptr; 

    CodegenContext();
    void generate(fdefNode* startFunc);
    llvm::Function* getBuiltin(const std::string& name);
    llvm::Type* getLLVMType(typeClass* t);
    void createBuiltinDeclarations();

    llvm::AllocaInst* createEntryBlockAlloca(llvm::Type* type, const std::string& VarName);
    llvm::GlobalVariable* createGlobalVariable(llvm::Type* type, const std::string& name);
    llvm::Value* logError(const std::string& str);

    void enterScope();
    void exitScope();

    void enterFunctionScope(const std::string& fnName);
    void exitFunctionScope();
    std::string getQualifiedFunctionName(const std::string& fnName);
    void registerLocalFunction(const std::string& fnName, const std::string& qualifiedName);
    std::string lookupLocalFunction(const std::string& fnName);

    llvm::Value* findVariable(const std::string& name);
    void setVariable(const std::string& name, llvm::Value* value);
    void clearNamedValues();

    static void promoteToI32(llvm::Value* &L, llvm::Value* &R, CodegenContext& context);

    llvm::Module& GetModule() { return *TheModule; }
    void printIntermediate(std::ostream& os);
    void printFinal(std::ostream& os);

    void pushLoop(std::string name, llvm::BasicBlock* breakBB, llvm::BasicBlock* continueBB);
    void popLoop();
    llvm::BasicBlock* getBreakBlock(std::string name);
    llvm::BasicBlock* getContinueBlock(std::string name);

    void optimize();

};

#endif