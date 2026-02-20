#include "codegen.hpp"
#include "ast.hpp" 
#include "symbol.hpp" 
#include <stdexcept>
#include <iostream>
#include <string>

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/CodeGen.h" 
#include "llvm/ADT/SmallString.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"

static std::string process_escapes(const std::string& raw_str) {
    std::string result = "";
    for (size_t i = 0; i < raw_str.length(); ++i) {
        if (raw_str[i] == '\\' && i + 1 < raw_str.length()) {
            switch (raw_str[i + 1]) {
                case 'n':  result += '\n'; i++; break;
                case 't':  result += '\t'; i++; break;
                case 'r':  result += '\r'; i++; break;
                case '\\': result += '\\'; i++; break;
                case '"':  result += '"'; i++; break;
                default:
                    result += raw_str[i]; 
                    break;
            }
        } else result += raw_str[i];
    }
    return result;
}

CodegenContext::CodegenContext() : Builder(TheContext) {
    TheModule = std::make_unique<llvm::Module>("my_compiler_module", TheContext);
    currentFunction = nullptr;
    MainFunctionNode = nullptr;
    createBuiltinDeclarations();
    clearNamedValues();
}

void CodegenContext::generate(fdefNode* startFunc) {
    if (!startFunc) return;
    this->MainFunctionNode = startFunc;
    startFunc->codegen(*this);
}

llvm::Function* CodegenContext::getBuiltin(const std::string& name) {
    if (builtinFunctions.count(name)) return builtinFunctions[name];
    return nullptr;
}

void CodegenContext::createBuiltinDeclarations() {
    llvm::Type* i32Type = Builder.getInt32Ty();
    llvm::Type* i8Type = Builder.getInt8Ty();
    llvm::Type* voidType = Builder.getVoidTy();
    llvm::Type* i8PtrType = Builder.getInt8Ty()->getPointerTo();

    builtinFunctions["writeInteger"] = llvm::Function::Create(llvm::FunctionType::get(voidType, {i32Type}, false), llvm::Function::ExternalLinkage, "writeInteger", TheModule.get());
    builtinFunctions["writeChar"] = llvm::Function::Create(llvm::FunctionType::get(voidType, {i8Type}, false), llvm::Function::ExternalLinkage, "writeChar", TheModule.get());
    builtinFunctions["writeByte"] = llvm::Function::Create(llvm::FunctionType::get(voidType, {i8Type}, false), llvm::Function::ExternalLinkage, "writeByte", TheModule.get());
    builtinFunctions["writeString"] = llvm::Function::Create(llvm::FunctionType::get(voidType, {i8PtrType}, false), llvm::Function::ExternalLinkage, "writeString", TheModule.get());
    builtinFunctions["readInteger"] = llvm::Function::Create(llvm::FunctionType::get(i32Type, {}, false), llvm::Function::ExternalLinkage, "readInteger", TheModule.get());
    builtinFunctions["readChar"] = llvm::Function::Create(llvm::FunctionType::get(i8Type, {}, false), llvm::Function::ExternalLinkage, "readChar", TheModule.get());
    builtinFunctions["readByte"] = llvm::Function::Create(llvm::FunctionType::get(i8Type, {}, false), llvm::Function::ExternalLinkage, "readByte", TheModule.get());
    builtinFunctions["readString"] = llvm::Function::Create(llvm::FunctionType::get(voidType, {i32Type, i8PtrType}, false), llvm::Function::ExternalLinkage, "readString", TheModule.get());
    builtinFunctions["extend"] = llvm::Function::Create(llvm::FunctionType::get(i32Type, {i8Type}, false), llvm::Function::ExternalLinkage, "extend", TheModule.get());
    builtinFunctions["shrink"] = llvm::Function::Create(llvm::FunctionType::get(i8Type, {i32Type}, false), llvm::Function::ExternalLinkage, "shrink", TheModule.get());
    builtinFunctions["strlen"] = llvm::Function::Create(llvm::FunctionType::get(i32Type, {i8PtrType}, false), llvm::Function::ExternalLinkage, "dana_strlen", TheModule.get());
    builtinFunctions["strcmp"] = llvm::Function::Create(llvm::FunctionType::get(i32Type, {i8PtrType, i8PtrType}, false), llvm::Function::ExternalLinkage, "dana_strcmp", TheModule.get());
    builtinFunctions["strcpy"] = llvm::Function::Create(llvm::FunctionType::get(i8PtrType, {i8PtrType, i8PtrType}, false), llvm::Function::ExternalLinkage, "dana_strcpy", TheModule.get());
    builtinFunctions["strcat"] = llvm::Function::Create(llvm::FunctionType::get(i8PtrType, {i8PtrType, i8PtrType}, false), llvm::Function::ExternalLinkage, "dana_strcat", TheModule.get());
}

void CodegenContext::pushLoop(std::string name, llvm::BasicBlock* breakBB, llvm::BasicBlock* continueBB) {
    breakBlockStack.push({name, breakBB});
    continueBlockStack.push({name, continueBB});
}

void CodegenContext::popLoop() {
    if (!breakBlockStack.empty()) breakBlockStack.pop();
    if (!continueBlockStack.empty()) continueBlockStack.pop();
}

llvm::BasicBlock* CodegenContext::getBreakBlock(std::string name) {
    if (name == "") {
        if (breakBlockStack.empty()) return nullptr;
        return breakBlockStack.top().second;
    } else {
        std::stack<std::pair<std::string, llvm::BasicBlock*>> tempStack = breakBlockStack;
        while (!tempStack.empty()) {
            if (tempStack.top().first == name) return tempStack.top().second;
            tempStack.pop();
        }
        return nullptr;
    }
}

llvm::BasicBlock* CodegenContext::getContinueBlock(std::string name) {
    if (name == "") {
        if (continueBlockStack.empty()) return nullptr;
        return continueBlockStack.top().second;
    } else {
        std::stack<std::pair<std::string, llvm::BasicBlock*>> tempStack = continueBlockStack;
        while (!tempStack.empty()) {
            if (tempStack.top().first == name) return tempStack.top().second;
            tempStack.pop();
        }
        return nullptr;
    }
}

llvm::Type* CodegenContext::getLLVMType(typeClass* t) {
    if (!t) return Builder.getVoidTy();
    if (dynamic_cast<arrayType*>(t)) {
        std::vector<int> dimensions;
        typeClass* finalBase = t;
        typeClass* current = t;
        while (auto* current_arr = dynamic_cast<arrayType*>(current)) {
            Const* sz = current_arr->getSize();
            int n = 0;
            if (sz) n = sz->value;
            dimensions.push_back(n);
            finalBase = current_arr->getBaseType();
            current = finalBase;
        }
        std::reverse(dimensions.begin(), dimensions.end());
        llvm::Type* baseLLVMType = getLLVMType(finalBase);
        llvm::Type* currentType = baseLLVMType;
        for (auto it = dimensions.rbegin(); it != dimensions.rend(); ++it) currentType = llvm::ArrayType::get(currentType, *it);
        return currentType;
    }
    if (auto* ref = dynamic_cast<refType*>(t)) {
        llvm::Type* baseType = getLLVMType(ref->getBaseType());
        return llvm::PointerType::get(baseType, 0);
    }
    if (auto* b = dynamic_cast<basicType*>(t)) {
        Type ty = b->getType();
        switch (ty) {
            case TYPE_INT:  return Builder.getInt32Ty();
            case TYPE_CHAR: return Builder.getInt8Ty(); 
            case TYPE_BYTE: return Builder.getInt8Ty();
            case TYPE_BOOL: return Builder.getInt1Ty(); 
            case TYPE_VOID: return Builder.getVoidTy();
            default: return Builder.getVoidTy();
        }
    }
    return Builder.getVoidTy();
}

llvm::AllocaInst* CodegenContext::createEntryBlockAlloca(llvm::Type* type, const std::string& VarName) {
    if (!currentFunction) {
        logError("createEntryBlockAlloca called with no current function");
        return nullptr;
    }
    llvm::IRBuilder<> TmpB(&currentFunction->getEntryBlock(), currentFunction->getEntryBlock().begin());
    llvm::AllocaInst* Alloca = TmpB.CreateAlloca(type, nullptr, VarName);
    setVariable(VarName, Alloca);
    return Alloca;
}

llvm::GlobalVariable* CodegenContext::createGlobalVariable(llvm::Type* type, const std::string& name) {
    if (!TheModule) {
        logError("createGlobalVariable: module is null");
        return nullptr;
    }
    llvm::Constant* init = llvm::Constant::getNullValue(type);
    auto *GV = new llvm::GlobalVariable(
        /*Module=*/*TheModule,
        /*Type=*/type,
        /*isConstant=*/false,
        /*Linkage=*/llvm::GlobalValue::ExternalLinkage,
        /*Initializer=*/init,
        /*Name=*/name
    );
    setVariable(name, GV);
    return GV;
}

llvm::Value* CodegenContext::logError(const std::string& str) {
    std::cerr << "Codegen Error: " << str << std::endl;
    return nullptr;
}

void CodegenContext::enterScope() {
    namedValuesStack.push_back(std::map<std::string, llvm::Value*>());
}

void CodegenContext::exitScope() {
    if (!namedValuesStack.empty()) {
        namedValuesStack.pop_back();
    } else {
        logError("exitScope called on empty scope stack");
    }
}

void CodegenContext::enterFunctionScope(const std::string& fnName) {
    functionNameStack.push_back(fnName);
}

void CodegenContext::exitFunctionScope() {
    if (!functionNameStack.empty()) {
        functionNameStack.pop_back();
    } else {
        logError("exitFunctionScope called on empty function name stack");
    }
}

std::string CodegenContext::getQualifiedFunctionName(const std::string& fnName) {
    if (functionNameStack.empty()) {
        return fnName;
    }
    std::string qualified = fnName;
    for (const auto& scope : functionNameStack) {
        qualified = scope + "." + qualified;
    }
    return qualified;
}

void CodegenContext::registerLocalFunction(const std::string& fnName, const std::string& qualifiedName) {
    if (localFunctionsStack.empty()) {
        localFunctionsStack.push_back(std::map<std::string, std::string>());
    }
    localFunctionsStack.back()[fnName] = qualifiedName;
}

std::string CodegenContext::lookupLocalFunction(const std::string& fnName) {
    for (auto it = localFunctionsStack.rbegin(); it != localFunctionsStack.rend(); ++it) {
        if (it->count(fnName)) return (*it)[fnName];
    }
    return fnName;  
}

void CodegenContext::clearNamedValues() {
    namedValuesStack.clear();
    enterScope();
}

llvm::Value* CodegenContext::findVariable(const std::string& name) {
    for (auto it = namedValuesStack.rbegin(); it != namedValuesStack.rend(); ++it) {
        if (it->count(name)) return (*it)[name];
    }
    return nullptr; 
}

void CodegenContext::setVariable(const std::string& name, llvm::Value* value) {
    if (!namedValuesStack.empty()) namedValuesStack.back()[name] = value;
    else logError("setVariable called with no active scope");
}

void CodegenContext::promoteToI32(llvm::Value* &L, llvm::Value* &R, CodegenContext& context) {
    auto* LTy = L->getType();
    auto* RTy = R->getType();
    auto* i32Ty = context.Builder.getInt32Ty();
    if (LTy == RTy) return;
    if (LTy->isIntegerTy(1) || LTy->isIntegerTy(8)) L = context.Builder.CreateZExt(L, i32Ty, "promL");
    if (RTy->isIntegerTy(1) || RTy->isIntegerTy(8)) R = context.Builder.CreateZExt(R, i32Ty, "promR");
}

void CodegenContext::printIntermediate(std::ostream& os) {
    std::string ir_str;
    llvm::raw_string_ostream ros(ir_str);
    TheModule->print(ros, nullptr);
    os << ros.str();
}

void CodegenContext::printFinal(std::ostream& os) {
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!Target) {
        llvm::errs() << "Target lookup failed: " << Error;
        throw std::runtime_error("Could not find target for " + TargetTriple);
    }
    auto CPU = "generic";
    auto Features = "";
    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
    TheModule->setDataLayout(TheTargetMachine->createDataLayout());
    TheModule->setTargetTriple(TargetTriple);
    llvm::SmallString<0> AsmStrVec;
    llvm::raw_svector_ostream asm_ros(AsmStrVec); 
    llvm::legacy::PassManager pass;
    if (TheTargetMachine->addPassesToEmitFile(pass, asm_ros, nullptr, llvm::CGFT_AssemblyFile)) throw std::runtime_error("TargetMachine can't emit assembly file");
    pass.run(*TheModule);
    os << asm_ros.str().str();
}

llvm::Value* headerNode::codegen(CodegenContext& context) { return nullptr; }
llvm::Value* paramNode::codegen(CodegenContext& context) { return nullptr; }
llvm::Value* Id::codegen(CodegenContext& context) { return nullptr; }
llvm::Value* Const::codegen(CodegenContext& context) { return nullptr; }
llvm::Type* lvalNode::getType(CodegenContext& context) { return nullptr; }


llvm::Value* fdefNode::codegen(CodegenContext& context) {
    headerNode* hdr = this->head;
    if (!hdr || !hdr->iden) return context.logError("Function definition missing header");
    std::string fnName = hdr->iden->name;
    bool isMain = (this == context.MainFunctionNode);
    if (isMain) fnName = "main";
    else {
        fnName = context.getQualifiedFunctionName(fnName);
    }

    llvm::Function* TheFunction = context.TheModule->getFunction(fnName);
    if (!TheFunction) {
        std::vector<llvm::Type*> ParamTypes;
        if (hdr->params) {
            paramNode* p = hdr->params;
            while(p) {
                if (p->names) {
                    llvm::Type* paramT = context.getLLVMType(p->types);
                    if (paramT->isArrayTy()) {
                        llvm::Type* baseT = paramT;
                        baseT = baseT->getArrayElementType();
                        paramT = baseT->getPointerTo();
                    }
                    for (size_t i = 0; i < p->names->size(); ++i) {
                        ParamTypes.push_back(paramT); 
                    }
                }
                p = p->tail;
            }
        }

        llvm::Type* retType;
        if (isMain) retType = context.Builder.getInt32Ty();
        else  retType = context.getLLVMType(hdr->headType);

        llvm::FunctionType* FT = llvm::FunctionType::get(retType, ParamTypes, false);
        TheFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fnName, context.TheModule.get());
    }
    
    if (!TheFunction->empty()) {
        return context.logError("Function " + fnName + " is already defined.");
    }

    llvm::BasicBlock* EntryBB = llvm::BasicBlock::Create(context.TheContext, "entry", TheFunction);
    llvm::Function* OldFunction = context.currentFunction;
    context.currentFunction = TheFunction;
    context.enterScope();
    
    if (!isMain) {
        context.registerLocalFunction(hdr->iden->name, fnName);
        context.enterFunctionScope(hdr->iden->name);
        context.localFunctionsStack.push_back(std::map<std::string, std::string>());
    }
    context.Builder.SetInsertPoint(EntryBB);
    
    if (hdr->params) {
        paramNode* p = hdr->params;
        auto arg_it = TheFunction->arg_begin();
        while(p) {
            if (p->names) {
                llvm::Type* paramT = context.getLLVMType(p->types);
                if (paramT->isArrayTy()) {
                    llvm::Type* baseT = paramT;
                    baseT = baseT->getArrayElementType();
                    paramT = baseT->getPointerTo();
                }

                for (const auto& name : *(p->names)) {
                    if (arg_it == TheFunction->arg_end())
                        return context.logError("Too few arguments provided to function " + fnName);
                    
                    llvm::Value* arg = arg_it++;
                    arg->setName(name);
                    llvm::AllocaInst* Alloca = context.createEntryBlockAlloca(paramT, name);
                    context.Builder.CreateStore(arg, Alloca);
                }
            }
            p = p->tail;
        }
    }
    if (this->body) this->body->codegen(context);
    llvm::BasicBlock* CurrentBB = context.Builder.GetInsertBlock();
    if (!CurrentBB || CurrentBB->getTerminator() == nullptr) {
        llvm::Type* retType = TheFunction->getReturnType();
        if (isMain) context.Builder.CreateRet(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0, true)));
        else if (retType->isVoidTy()) context.Builder.CreateRetVoid();
        else context.Builder.CreateRet(llvm::Constant::getNullValue(retType));
    }
    if (!isMain) {
        if (!context.localFunctionsStack.empty()) {
            context.localFunctionsStack.pop_back();
        }
        context.exitFunctionScope(); 
    }
    context.exitScope(); 
    context.currentFunction = OldFunction;
    for (auto &BB : *TheFunction) {
    if (!BB.getTerminator()) {
        context.Builder.SetInsertPoint(&BB);
        if (isMain) context.Builder.CreateRet(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0, true)));
        else if (TheFunction->getReturnType()->isVoidTy()) context.Builder.CreateRetVoid();
        else context.Builder.CreateRet(llvm::Constant::getNullValue(TheFunction->getReturnType()));
        }
    }
    llvm::verifyFunction(*TheFunction);
    if (llvm::verifyFunction(*TheFunction, &llvm::errs())) {
        llvm::errs() << "Invalid IR in function " << fnName << "\n";
        TheFunction->print(llvm::errs());
    }
    return TheFunction;
}


llvm::Value* stmtNode::codegen(CodegenContext& context) {
    if (context.Builder.GetInsertBlock() == nullptr || 
        context.Builder.GetInsertBlock()->getTerminator() != nullptr) {
        return nullptr;
    }
    
    if (stmtType == "loop") {
        std::string loopName = (this->tag ? this->tag->name : "");
        llvm::Function* TheFunction = context.currentFunction;
        if (!TheFunction) return context.logError("Loop outside of a function");
        llvm::BasicBlock* LoopHeaderBB = llvm::BasicBlock::Create(context.TheContext, loopName + "_header", TheFunction);
        llvm::BasicBlock* LoopBodyBB = llvm::BasicBlock::Create(context.TheContext, loopName + "_body", TheFunction);
        llvm::BasicBlock* AfterLoopBB = llvm::BasicBlock::Create(context.TheContext, loopName + "_after", TheFunction);
        context.pushLoop(loopName, AfterLoopBB, LoopHeaderBB);
        context.Builder.CreateBr(LoopHeaderBB);
        context.Builder.SetInsertPoint(LoopHeaderBB);
        context.Builder.CreateBr(LoopBodyBB);
        context.Builder.SetInsertPoint(LoopBodyBB);
        if (this->stmtBody) this->stmtBody->codegen(context);
        if (context.Builder.GetInsertBlock()->getTerminator() == nullptr) context.Builder.CreateBr(LoopHeaderBB);
        context.popLoop();
        context.Builder.SetInsertPoint(AfterLoopBB);
    }
    else if (stmtType == "break") {
        std::string targetName = (this->tag ? this->tag->name : "");
        llvm::BasicBlock* BreakBB = context.getBreakBlock(targetName);
        if (!BreakBB) return context.logError("Break statement not within a loop or target '" + targetName + "' not found");
        context.Builder.CreateBr(BreakBB);
    }
    else if (stmtType == "continue") {
        std::string targetName = (this->tag ? this->tag->name : "");
        llvm::BasicBlock* ContinueBB = context.getContinueBlock(targetName);
        if (!ContinueBB) return context.logError("Continue statement not within a loop or target '" + targetName + "' not found");
        context.Builder.CreateBr(ContinueBB);
    }
    else if (stmtType == "exit") {
        if (context.currentFunction->getReturnType()->isVoidTy()) context.Builder.CreateRetVoid();
        else context.Builder.CreateRet(llvm::Constant::getNullValue(context.currentFunction->getReturnType()));
    }
    else if (stmtType == "vardecl") {
        if (!varType || !varNames) return context.logError("Malformed vardecl");
        llvm::Type* type = context.getLLVMType(varType);
        if (type->isVoidTy()) return context.logError("Cannot declare variable of type void");
        
        for (const auto& n : *varNames) {
            bool isMain = (context.currentFunction && context.currentFunction->getName() == "main");
            if (context.currentFunction == nullptr || isMain) {
                llvm::GlobalVariable* GV = context.createGlobalVariable(type, n);
                if (!GV) return context.logError("Failed to create global variable " + n);
            } else {
                llvm::AllocaInst* alloca = context.createEntryBlockAlloca(type, n);
                if (!alloca) return context.logError("Failed to create local alloca for " + n);
                context.setVariable(n, alloca);
            }
        }
    } 
    else if (stmtType == "decl") {}
    else if (stmtType == "asgn") {
        if (!lval || !exp) return context.logError("Invalid assignment");
        
        llvm::Value* lhsPtr = lval->codegen_ptr(context);
        if (!lhsPtr) return context.logError("LHS of assignment is not a valid l-value");

        llvm::Value* rhsVal = exp->codegen(context);
        if (!rhsVal) return context.logError("RHS of assignment failed to generate code");
        
        llvm::Type* lhsType = lhsPtr->getType()->getPointerElementType();
        llvm::Type* rhsType = rhsVal->getType();

        if (lhsType != rhsType) {
            if (lhsType->isIntegerTy(32) && (rhsType->isIntegerTy(1) || rhsType->isIntegerTy(8))) {
                rhsVal = context.Builder.CreateZExt(rhsVal, lhsType, "zext_assign");
            }
            else if (lhsType->isIntegerTy(8) && rhsType->isIntegerTy(1)) {
                rhsVal = context.Builder.CreateZExt(rhsVal, lhsType, "zext_assign_byte");
            }
            else if (lhsType->isIntegerTy(8) && rhsType->isIntegerTy(32)) {
                rhsVal = context.Builder.CreateTrunc(rhsVal, lhsType, "trunc_assign");
            }
        }

        context.Builder.CreateStore(rhsVal, lhsPtr);
    } 
    else if (stmtType == "pc") {
        if (!exp) return context.logError("Procedure call missing expression");
        exp->codegen(context); 
    } 
    else if (stmtType == "return") {
        if (exp) {
            llvm::Value* retVal = exp->codegen(context);
            if (!retVal) return context.logError("Return expression failed to generate code");
            llvm::Type* funcRetType = context.currentFunction->getReturnType();
            if (retVal->getType() != funcRetType) {
                if (funcRetType->isIntegerTy() && retVal->getType()->isIntegerTy(1)) retVal = context.Builder.CreateZExt(retVal, funcRetType, "castexpr");
            }
            context.Builder.CreateRet(retVal);
        } else {
            context.Builder.CreateRetVoid();
        }
    } 
    else if (stmtType == "if") {
        if (!ifnode) return context.logError("Malformed if");
        ifnode->codegen(context);
    }
    else if (stmtType == "def") {
        llvm::BasicBlock* currentBlock = context.Builder.GetInsertBlock();
        funcDef->codegen(context);
        if (currentBlock) context.Builder.SetInsertPoint(currentBlock);
    }
    if (context.Builder.GetInsertBlock() != nullptr && 
        context.Builder.GetInsertBlock()->getTerminator() == nullptr) 
    {
        if (this->stmtTail) {
            this->stmtTail->codegen(context);
        }
    }
    return nullptr;
}

llvm::Value* ifNode::codegen(CodegenContext& context) {
    if (ifCond == nullptr) {
        if (ifStmtBody) ifStmtBody->codegen(context);
        return nullptr;
    }
    llvm::Value* condVal = ifCond->codegen(context);
    if (!condVal) return context.logError("If condition failed to generate code");
    if (condVal->getType()->isIntegerTy(32)) { 
        condVal = context.Builder.CreateICmpNE(condVal, 
        llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)), "ifcond");
    } else if (!condVal->getType()->isIntegerTy(1)) {
        return context.logError("If condition is not a boolean or integer");
    }
    llvm::Function* TheFunction = context.currentFunction;
    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(context.TheContext, "then", TheFunction);
    llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(context.TheContext, "else", TheFunction);
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(context.TheContext, "ifcont", TheFunction);
    bool hasElse = (ifTail != nullptr);
    llvm::BasicBlock* NextBB = hasElse ? ElseBB : MergeBB;
    context.Builder.CreateCondBr(condVal, ThenBB, NextBB);
    context.Builder.SetInsertPoint(ThenBB);
    if (ifStmtBody) ifStmtBody->codegen(context);
    if (context.Builder.GetInsertBlock()->getTerminator() == nullptr) {
        context.Builder.CreateBr(MergeBB);
    }

    if (hasElse) {
        context.Builder.SetInsertPoint(ElseBB);
        if (ifTail) ifTail->codegen(context);
        if (context.Builder.GetInsertBlock()->getTerminator() == nullptr) {
            context.Builder.CreateBr(MergeBB);
        }
    } else {
        ElseBB->eraseFromParent();
    }
    if (!MergeBB->hasNPredecessorsOrMore(1)) MergeBB->eraseFromParent();
    else context.Builder.SetInsertPoint(MergeBB);
    
    return nullptr;
}


llvm::Value* exprNode::codegen(CodegenContext& context) {
    switch (op) {
        case 'c':
            return llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, constant->value, true));
        case 'x':
            return llvm::ConstantInt::get(context.TheContext, llvm::APInt(8, constant->value, true));
        case 'b':
            return llvm::ConstantInt::get(context.TheContext, llvm::APInt(1, tfFlag ? 1 : 0, true));
        case 'i':
        {
            if (!lval) return context.logError("Identifier expression missing lval");
            return lval->codegen(context); 
        }
        case 'f':
            if (!func) return context.logError("Function call node missing fcallNode");
            return func->codegen(context);
        case '+': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            return context.Builder.CreateAdd(L, R, "addtmp");
        }
        case '-': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            return context.Builder.CreateSub(L, R, "subtmp");
        }
        case '*': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            return context.Builder.CreateMul(L, R, "multmp");
        }
        case '/': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            return context.Builder.CreateSDiv(L, R, "divtmp"); 
        }
        case '%': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            return context.Builder.CreateSRem(L, R, "modtmp"); 
        }
        case '=': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            CodegenContext::promoteToI32(L, R, context);
            return context.Builder.CreateICmpEQ(L, R, "eqtmp");
        }
        case '<': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            CodegenContext::promoteToI32(L, R, context); 
            return context.Builder.CreateICmpSLT(L, R, "lttmp"); 
        }
        case '>': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            CodegenContext::promoteToI32(L, R, context); 
            return context.Builder.CreateICmpSGT(L, R, "gttmp"); 
        }
        case 'g': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            CodegenContext::promoteToI32(L, R, context);
            return context.Builder.CreateICmpSGE(L, R, "getmp");
        }
        case 'l': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            CodegenContext::promoteToI32(L, R, context);
            return context.Builder.CreateICmpSLE(L, R, "letmp");
        }
        case 'd': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            CodegenContext::promoteToI32(L, R, context);
            return context.Builder.CreateICmpNE(L, R, "netmp");
        }
        case 'a': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            return context.Builder.CreateAnd(L, R, "andtmp");
        }
        case 'o': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            if (!L || !R) return nullptr;
            return context.Builder.CreateOr(L, R, "ortmp");
        }
        case 'n': {
            llvm::Value* R = rightExpr->codegen(context);
            if (!R) return nullptr;
            return context.Builder.CreateNot(R, "nottmp");
        }
        default:
            return context.logError("Unknown expression operator");
    }
}


llvm::Value* fcallNode::codegen(CodegenContext& context) {
    std::string lookupName = context.lookupLocalFunction(iden->name);
    llvm::Function* CalleeF = context.TheModule->getFunction(lookupName);
    
    if (!CalleeF) CalleeF = context.TheModule->getFunction(iden->name);
    if (!CalleeF) CalleeF = context.getBuiltin(iden->name);
    if (!CalleeF) return context.logError("Unknown function referenced: " + iden->name);
    std::vector<llvm::Value*> ArgsV;
    llvm::FunctionType* FType = CalleeF->getFunctionType();
    if (args) {
        if (args->size() != FType->getNumParams()) return context.logError("Incorrect # of arguments passed to " + iden->name);
        int i = 0;
        for (auto* argExpr : *args) {
            llvm::Type* ParamType = FType->getParamType(i++);
            llvm::Value* ArgVal = nullptr;

            if (ParamType->isPointerTy()) {
                if (argExpr->lval) {
                   llvm::Value* ptr = argExpr->lval->codegen_ptr(context);
                   if (ptr && ptr->getType()->getPointerElementType()->isArrayTy()) {
                       std::vector<llvm::Value*> indices;
                       indices.push_back(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)));
                       indices.push_back(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)));
                       ArgVal = context.Builder.CreateGEP(ptr->getType()->getPointerElementType(), ptr, indices, "arraydecay");
                   } else {
                       ArgVal = ptr;
                   }
                } else {
                    return context.logError("Expression is not an l-value, cannot pass as ref");
                }

                if (ArgVal && ArgVal->getType() != ParamType) {
                     ArgVal = context.Builder.CreateBitCast(ArgVal, ParamType, "argcast");
                }
            } else {
                ArgVal = argExpr->codegen(context);
            }
            if (!ArgVal) return nullptr;

            if (ArgVal->getType() != ParamType) {
                if (ArgVal->getType()->isIntegerTy() && ParamType->isIntegerTy()) {
                     if (ArgVal->getType()->getIntegerBitWidth() < ParamType->getIntegerBitWidth()) {
                         ArgVal = context.Builder.CreateZExt(ArgVal, ParamType, "arg_promote");
                     } else {
                         ArgVal = context.Builder.CreateTrunc(ArgVal, ParamType, "arg_trunc");
                     }
                }
            }

            ArgsV.push_back(ArgVal);
        }
    }
    if (FType->getReturnType()->isVoidTy()) {
        context.Builder.CreateCall(FType, CalleeF, ArgsV);
        return nullptr;
    } else return context.Builder.CreateCall(FType, CalleeF, ArgsV, "calltmp");
}


llvm::Value* lvalNode::codegen(CodegenContext& context) {
    if (isString) {
        std::string raw_literal = ident->name;
        std::string processed_str = process_escapes(raw_literal.substr(1, raw_literal.length() - 2));
        return context.Builder.CreateGlobalStringPtr(processed_str);
    }
    llvm::Value* ptr = codegen_ptr(context);
    if (!ptr) return nullptr;
    llvm::Type* loadType = nullptr;
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ptr)) {
        loadType = alloca->getAllocatedType();
    } else if (auto* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(ptr)) {
        loadType = gep->getResultElementType();
    } else if (auto* gv = llvm::dyn_cast<llvm::GlobalVariable>(ptr)) {
        loadType = gv->getValueType();
    } else if (auto* arg = llvm::dyn_cast<llvm::Argument>(ptr)) {
        if (auto *PT = llvm::dyn_cast<llvm::PointerType>(arg->getType())) {
            loadType = PT->getElementType();
            if (!loadType) return context.logError("Failed to get pointer element type for argument");
        } else {
            loadType = arg->getType();
        }
    } else {
        if (ptr->getType()->isPointerTy()) {
            if (auto *PT = llvm::dyn_cast<llvm::PointerType>(ptr->getType())) {
                loadType = PT->getElementType();
                if (!loadType) return context.logError("Failed to get pointer element type");
            } else {
                return context.logError("lval::codegen: could not cast to PointerType");
            }
        } else {
            return context.logError("lval::codegen: ptr is not a valid pointer type");
        }
    }

    if (loadType == nullptr) return context.logError("Could not determine type to load from lval");
    
    if (loadType->isArrayTy()) {
        std::vector<llvm::Value*> IdxList;
        IdxList.push_back(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)));
        IdxList.push_back(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)));
        return context.Builder.CreateGEP(loadType, ptr, IdxList, "arraydecayptr");
    }
    return context.Builder.CreateLoad(loadType, ptr, "loadtmp");
}


llvm::Value* lvalNode::codegen_ptr(CodegenContext& context) {
    if (isString) {
        std::string raw_literal = ident->name;
        std::string processed_str = process_escapes(raw_literal.substr(1, raw_literal.length() - 2));
        return context.Builder.CreateGlobalStringPtr(processed_str);
    }
    llvm::Value* Ptr = context.findVariable(ident->name);
    if (!Ptr) return context.logError("lval base variable not found: " + ident->name);

    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(Ptr)) {
        if (alloca->getAllocatedType()->isPointerTy()) {
            Ptr = context.Builder.CreateLoad(alloca->getAllocatedType(), Ptr, "loadrefptr");
        }
    }

    llvm::Type* gepBaseType = nullptr;
    if (auto *PT = llvm::dyn_cast<llvm::PointerType>(Ptr->getType())) {
        gepBaseType = PT->getElementType();
        if (!gepBaseType) return context.logError("Failed to get element type for pointer in codegen_ptr");
    } else {
        return context.logError("Variable is not a pointer/address");
    }

    if (!ind || ind->empty()) {
        bool isGlobalArray = false;
        if (auto* GV = llvm::dyn_cast<llvm::GlobalVariable>(Ptr)) {
             if (GV->getValueType()->isArrayTy()) isGlobalArray = true;
        }
        if (isGlobalArray) {
            return Ptr;
        }
        return Ptr;
    }

    std::vector<llvm::Value*> IdxList;
    llvm::Type* currentType = Ptr->getType(); 

    bool isArrayAlloca = false;

    if (auto* AI = llvm::dyn_cast<llvm::AllocaInst>(context.findVariable(ident->name))) {
        if (AI->getAllocatedType()->isArrayTy()) isArrayAlloca = true;
    }
    else if (auto* GV = llvm::dyn_cast<llvm::GlobalVariable>(context.findVariable(ident->name))) {
        if (GV->getValueType()->isArrayTy()) isArrayAlloca = true;
    }

    if (isArrayAlloca) {
        IdxList.push_back(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)));
        currentType = gepBaseType; 
    }

    for (auto* idxExpr : *ind) {
        llvm::Value* idxVal = idxExpr->codegen(context);
        if (!idxVal) return nullptr;
        
        // FIX: Ensure index is i32
        if (idxVal->getType()->isIntegerTy(8)) {
            idxVal = context.Builder.CreateZExt(idxVal, context.Builder.getInt32Ty(), "idxzext");
        } else if (idxVal->getType()->isIntegerTy(1)) {
            idxVal = context.Builder.CreateZExt(idxVal, context.Builder.getInt32Ty(), "idxzext");
        }

        if (currentType->isPointerTy()) {
            IdxList.push_back(idxVal);
            if (auto *PTy = llvm::dyn_cast<llvm::PointerType>(currentType)) {
                currentType = PTy->getElementType();
                if (!currentType) return context.logError("Failed to get pointer element type during indexing");
            } else {
                return context.logError("Could not cast to PointerType during indexing");
            }
        } else if (currentType->isArrayTy()) {
            IdxList.push_back(idxVal);
            currentType = currentType->getArrayElementType();
        }
        else {
            return context.logError("Too many indices for array: " + ident->name);
        }
    }
    return context.Builder.CreateGEP(gepBaseType, Ptr, IdxList, "geptmp");
}

void CodegenContext::optimize() {
    llvm::legacy::FunctionPassManager fpm(TheModule.get());

    // 1. Promote Memory to Register (Mem2Reg):
    fpm.add(llvm::createPromoteMemoryToRegisterPass());

    // 2. Instruction Combining:
    fpm.add(llvm::createInstructionCombiningPass());

    // 3. Reassociate expressions:
    fpm.add(llvm::createReassociatePass());

    // 4. Eliminate Common SubExpressions (GVN):
    fpm.add(llvm::createNewGVNPass());

    // 5. CFG Simplification:
    fpm.add(llvm::createCFGSimplificationPass());
    fpm.doInitialization();
    for (auto &F : *TheModule) {
        fpm.run(F);
    }
}