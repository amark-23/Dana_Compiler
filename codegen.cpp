#include "codegen.hpp"
#include "ast.hpp"
#include "symbol.hpp"
#include <stdexcept>
#include <iostream>
#include <llvm/IR/Instructions.h>

CodegenContext::CodegenContext() : Builder(TheContext) {
    TheModule = std::make_unique<llvm::Module>("my_compiler_module", TheContext);
    currentFunction = nullptr;
    createBuiltinDeclarations();
}

void CodegenContext::generate(fdefNode* startFunc) {
    if (!startFunc) return;
    startFunc->codegen(*this);
    TheModule->print(llvm::errs(), nullptr);
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

    // writeInteger(n as int)
    llvm::FunctionType* writeIntType = llvm::FunctionType::get(voidType, {i32Type}, false);
    builtinFunctions["writeInteger"] = llvm::Function::Create(writeIntType, llvm::Function::ExternalLinkage, "writeInteger", TheModule.get());

    // writeChar(c as byte)
    llvm::FunctionType* writeCharType = llvm::FunctionType::get(voidType, {i8Type}, false);
    builtinFunctions["writeChar"] = llvm::Function::Create(writeCharType, llvm::Function::ExternalLinkage, "writeChar", TheModule.get());

    // writeByte(b as byte)
    builtinFunctions["writeByte"] = llvm::Function::Create(writeCharType, llvm::Function::ExternalLinkage, "writeByte", TheModule.get());

    // writeString(s as byte[])
    llvm::FunctionType* writeStringType = llvm::FunctionType::get(voidType, {i8PtrType}, false);
    builtinFunctions["writeString"] = llvm::Function::Create(writeStringType, llvm::Function::ExternalLinkage, "writeString", TheModule.get());

    // readInteger() is int
    llvm::FunctionType* readIntType = llvm::FunctionType::get(i32Type, {}, false);
    builtinFunctions["readInteger"] = llvm::Function::Create(readIntType, llvm::Function::ExternalLinkage, "readInteger", TheModule.get());

    // readChar() is byte
    llvm::FunctionType* readCharType = llvm::FunctionType::get(i8Type, {}, false);
    builtinFunctions["readChar"] = llvm::Function::Create(readCharType, llvm::Function::ExternalLinkage, "readChar", TheModule.get());

    // readByte() is byte
    builtinFunctions["readByte"] = llvm::Function::Create(readCharType, llvm::Function::ExternalLinkage, "readByte", TheModule.get());
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
        for (auto it = dimensions.rbegin(); it != dimensions.rend(); ++it) {
            currentType = llvm::ArrayType::get(currentType, *it);
        }
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

llvm::AllocaInst* CodegenContext::createEntryBlockAlloca(llvm::Function* TheFunction, const std::string& VarName, llvm::Type* type) {
    llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    llvm::AllocaInst* Alloca = TmpB.CreateAlloca(type, nullptr, VarName);
    setVariable(VarName, Alloca);
    return Alloca;
}

llvm::Value* CodegenContext::findVariable(const std::string& name) {
    if (namedValues.count(name)) {
        return namedValues[name];
    }
    return logError("Unknown variable name: " + name);
}

void CodegenContext::setVariable(const std::string& name, llvm::Value* value) {
    namedValues[name] = value;
}

llvm::Value* CodegenContext::logError(const std::string& str) {
    std::cerr << "Codegen Error: " << str << std::endl;
    return nullptr;
}

llvm::Value* fdefNode::codegen(CodegenContext& context) {
    headerNode* hdr = this->head;
    if (!hdr || !hdr->iden) return context.logError("Function definition missing header");
    std::string fnName = hdr->iden->name;
    llvm::Function* TheFunction = context.TheModule->getFunction(fnName);
    if (!TheFunction) {
        std::vector<llvm::Type*> ParamTypes;
        if (hdr->params) {
            paramNode* p = hdr->params;
            while(p) {
                for (size_t i = 0; i < p->names->size(); ++i) {
                    ParamTypes.push_back(context.getLLVMType(p->types));
                }
                p = p->tail;
            }
        }
        llvm::Type* retType = context.getLLVMType(hdr->headType);
        llvm::FunctionType* FT = llvm::FunctionType::get(retType, ParamTypes, false);
        TheFunction = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, fnName, context.TheModule.get());
    }
    llvm::BasicBlock* EntryBB = llvm::BasicBlock::Create(context.TheContext, "entry", TheFunction);
    context.Builder.SetInsertPoint(EntryBB);
    context.currentFunction = TheFunction;
    context.clearNamedValues();
    if (hdr->params) {
        paramNode* p = hdr->params;
        auto arg_it = TheFunction->arg_begin();
        while(p) {
            for (const auto& name : *(p->names)) {
                if (arg_it == TheFunction->arg_end())
                    return context.logError("Too few arguments provided to function " + fnName);
                
                llvm::Value* arg = arg_it++;
                arg->setName(name);
                llvm::Type* type = context.getLLVMType(p->types);
                llvm::AllocaInst* Alloca = context.createEntryBlockAlloca(TheFunction, name, type);
                context.Builder.CreateStore(arg, Alloca);
            }
            p = p->tail;
        }
    }
    if (this->body) this->body->codegen(context);
    llvm::verifyFunction(*TheFunction);
    return TheFunction;
}


llvm::Value* stmtNode::codegen(CodegenContext& context) {
    if (stmtType == "vardecl") {
        if (!varType || !varNames) return context.logError("Malformed vardecl");
        llvm::Type* type = context.getLLVMType(varType);
        for (const auto& n : *varNames) {
            context.createEntryBlockAlloca(context.currentFunction, n, type);
        }
    } 
    else if (stmtType == "asgn") {
        if (!lval || !exp) return context.logError("Invalid assignment");
        llvm::Value* lhsPtr = lval->codegen_ptr(context);
        if (!lhsPtr) return context.logError("LHS of assignment is not a valid l-value");
        llvm::Value* rhsVal = exp->codegen(context);
        if (!rhsVal) return context.logError("RHS of assignment failed to generate code");
        context.Builder.CreateStore(rhsVal, lhsPtr);
    } 
    else if (stmtType == "pc") {
        if (!exp) return context.logError("Procedure call missing expression");
        exp->codegen(context);
    } 
    else if (stmtType == "return") {
        if (exp) {
            llvm::Value* retVal = exp->codegen(context);
            context.Builder.CreateRet(retVal);
        } else context.Builder.CreateRetVoid();
    } 
    else if (stmtType == "if") {
        if (!ifnode) return context.logError("Malformed if");
        ifnode->codegen(context);
    }

    if (this->stmtTail) this->stmtTail->codegen(context);
    return nullptr;
}

llvm::Value* ifNode::codegen(CodegenContext& context) {
    llvm::Value* condVal = ifCond->codegen(context);
    if (!condVal) return context.logError("If condition failed to generate code");
    if (condVal->getType()->isIntegerTy(32)) {
        condVal = context.Builder.CreateICmpNE(condVal, 
            llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)), "ifcond");
    }
    llvm::Function* TheFunction = context.currentFunction;
    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(context.TheContext, "then", TheFunction);
    llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(context.TheContext, "else", TheFunction);
    llvm::BasicBlock* MergeBB = llvm::BasicBlock::Create(context.TheContext, "ifcont", TheFunction);
    context.Builder.CreateCondBr(condVal, ThenBB, ElseBB);
    context.Builder.SetInsertPoint(ThenBB);
    if (ifStmtBody) ifStmtBody->codegen(context);
    context.Builder.CreateBr(MergeBB);
    context.Builder.SetInsertPoint(ElseBB);
    if (ifTail) ifTail->codegen(context);
    context.Builder.CreateBr(MergeBB);
    context.Builder.SetInsertPoint(MergeBB);
    
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
            llvm::Value* ptr = lval->codegen_ptr(context);
            if (!ptr) return nullptr;
            llvm::Type* loadType = nullptr;
            if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ptr)) loadType = alloca->getAllocatedType();
            else if (auto* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(ptr)) loadType = gep->getResultElementType();
            else return context.logError("lval pointer is not an Alloca or GEP");
            return context.Builder.CreateLoad(loadType, ptr, "loadtmp");
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
        case '=': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            return context.Builder.CreateICmpEQ(L, R, "eqtmp");
        }
        case '<': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            return context.Builder.CreateICmpSLT(L, R, "lttmp");
        }
        case '>': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            return context.Builder.CreateICmpSGT(L, R, "gttmp");
        }
        case 'a': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            return context.Builder.CreateAnd(L, R, "andtmp");
        }
        case 'o': {
            llvm::Value* L = leftExpr->codegen(context);
            llvm::Value* R = rightExpr->codegen(context);
            return context.Builder.CreateOr(L, R, "ortmp");
        }
        case 'n': {
            llvm::Value* R = rightExpr->codegen(context);
            return context.Builder.CreateNot(R, "nottmp");
        }
        default:
            return context.logError("Unknown expression operator");
    }
}


llvm::Value* fcallNode::codegen(CodegenContext& context) {
    llvm::Function* CalleeF = context.TheModule->getFunction(iden->name);
    if (!CalleeF) CalleeF = context.getBuiltin(iden->name);
    if (!CalleeF) return context.logError("Unknown function referenced: " + iden->name);

    std::vector<llvm::Value*> ArgsV;
    if (args) {
        for (auto* argExpr : *args) {
            ArgsV.push_back(argExpr->codegen(context));
            if (!ArgsV.back()) return nullptr;
        }
    }

    return context.Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

llvm::Value* lvalNode::codegen_ptr(CodegenContext& context) {
    llvm::Value* Ptr = context.findVariable(ident->name);

    if (!Ptr) return context.logError("lval base variable not found: " + ident->name);
    if (!ind || ind->empty()) return Ptr;

    std::vector<llvm::Value*> IdxList;
    IdxList.push_back(llvm::ConstantInt::get(context.TheContext, llvm::APInt(32, 0)));
    llvm::Type* currentType = static_cast<llvm::AllocaInst*>(Ptr)->getAllocatedType();

    for (auto* idxExpr : *ind) {
        if (currentType->isArrayTy()) {
            IdxList.push_back(idxExpr->codegen(context));
            currentType = currentType->getArrayElementType();
        } else return context.logError("Too many indices for array: " + ident->name);
    }

    llvm::Type* elemType = static_cast<llvm::AllocaInst*>(Ptr)->getAllocatedType();
    return context.Builder.CreateGEP(elemType, Ptr, IdxList, "geptmp");
}

llvm::Value* lvalNode::codegen(CodegenContext& context) {
    llvm::Value* ptr = codegen_ptr(context);
    if (!ptr) return nullptr;

    llvm::Type* loadType = nullptr;
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ptr)) loadType = alloca->getAllocatedType();
    else if (auto* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(ptr)) loadType = gep->getResultElementType();
    else return context.logError("lval pointer is not an Alloca or GEP");

    if (loadType == nullptr) return context.logError("Could not determine type to load from lval");
    return context.Builder.CreateLoad(loadType, ptr, "loadtmp");
}

llvm::Type* lvalNode::getType(CodegenContext& context) {
    return nullptr;
}

llvm::Value* headerNode::codegen(CodegenContext& context) {
    return nullptr; 
}
llvm::Value* paramNode::codegen(CodegenContext& context) {
    return nullptr; 
}
llvm::Value* Id::codegen(CodegenContext& context) {
    return nullptr; 
}
llvm::Value* Const::codegen(CodegenContext& context) {
    return nullptr; 
}