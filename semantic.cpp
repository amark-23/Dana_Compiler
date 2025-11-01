#include "symbol.hpp"
#include "ast.hpp"
#include <iostream>
#include <vector>
#include <string>

bool sameType(typeClass *a, typeClass *b) {
    if (!a || !b) return false;
    if (a->isRef() && b->isRef()) {
        auto *ra = dynamic_cast<refType*>(a);
        auto *rb = dynamic_cast<refType*>(b);
        return sameType(ra->getBaseType(), rb->getBaseType());
    }
    if (a->isRef() && !b->isRef()) return sameType(dynamic_cast<refType*>(a)->getBaseType(), b);
    if (!a->isRef() && b->isRef()) return sameType(a, dynamic_cast<refType*>(b)->getBaseType());
    return a->getType() == b->getType();
}

void param_semanticCheck(paramNode *node, SymbolTable &sym) {
    while (node) {
        if (node->names) {
            for (auto &n : *(node->names)) {
                if (sym.lookupCurrentScope(n)) throw SemanticError("Parameter '" + n + "' redeclared", node->lineno);
                sym.addVariable(n, node->types, true);
            }
        }
        node = node->tail;
    }
}

void if_semanticCheck(ifNode *node, SymbolTable &sym) {
    if (!node) return;
    if (node->cond) {
        typeClass *condType = node->cond->semanticCheck(sym);
        static basicType boolType(TYPE_BOOL);
        if (!sameType(condType, &boolType)) throw SemanticError("Condition must be of integer (boolean) type " + typeToString(condType->getType()), node->lineno);
    }

    sym.enterScope();
    stmtNode *stmt = node->stmt;
    while (stmt) {
        stmt->semanticCheck(sym);
        stmt = stmt->stmtTail;
    }
    sym.exitScope();

    if (node->tail) if_semanticCheck(node->tail, sym);
}

typeClass *exprNode::semanticCheck(SymbolTable &sym) {
    switch (op) {
        case 'c': {
            static basicType intType(TYPE_INT);
            return &intType;
        }
        case 'x': {
            static basicType charType(TYPE_CHAR);
            return &charType;
        }
        case 'b': {
            static basicType charType(TYPE_CHAR);
            return &charType;
        }
        case 'i': {
            if (!lval) throw SemanticError("Identifier expression missing lval", this->lineno);
            return lval->semanticCheck(sym);
        }
        case 'f': {
            if (!func || !func->iden) throw SemanticError("Invalid function call", this->lineno);
            headerNode *hdr = sym.lookupFunction(func->iden->name);
            if (!hdr) throw SemanticError("Undefined function '" + func->iden->name + "'", this->lineno);

            std::vector<exprNode *> args = func->args ? *(func->args) : std::vector<exprNode *>();

            int paramCount = 0;
            for (paramNode *p = hdr->params; p; p = p->tail)
                if (p->names) paramCount += (int)p->names->size();

            if ((int)args.size() != paramCount) throw SemanticError("Function '" + func->iden->name + "' expects " + std::to_string(paramCount) + " args, got " + std::to_string(args.size()), this->lineno);

            int idx = 0;
            for (paramNode *p = hdr->params; p; p = p->tail)
                for (auto &n : *(p->names)) {
                    typeClass *expected = p->types;
                    typeClass *given = args[idx++]->semanticCheck(sym);
                    if (!sameType(expected, given)) throw SemanticError("Type mismatch in argument '" + n + "' (" + typeToString(expected->getType()) + ") of '" + func->iden->name + "' (" + typeToString(given->getType()) + ")", this->lineno);
                }

            return hdr->headType;
        }
        case '+': case '-': case '*': case '/': case '%': {
            typeClass *lt = leftExpr ? leftExpr->semanticCheck(sym) : nullptr;
            typeClass *rt = rightExpr ? rightExpr->semanticCheck(sym) : nullptr;
            static basicType intType(TYPE_INT);
            if (!lt && rt) return rt;
            if (!rt && lt) return lt;
            std::string opStr(1, op);
            if (!sameType(lt, rt)) throw SemanticError("Type mismatch in '" + opStr + "' expression (" + typeToString(lt->getType()) + " " + opStr + " " + typeToString(rt->getType()) + ")", this->lineno);
            return lt;
        }
        case '=': case '<': case '>': case 'g': case 'l': case 'd': case 'a':
        case 'o': case 'n': case '&': case '|': {
            if (leftExpr) {
                typeClass *l = leftExpr->semanticCheck(sym);
                typeClass *r = rightExpr->semanticCheck(sym);
                if (!l || !r) throw SemanticError("Null operand in relational/logical expression", this->lineno);
                if (!sameType(l, r)) throw SemanticError("Incompatible operand types for relational/logical operator", this->lineno);
            } else {
                typeClass *r = rightExpr->semanticCheck(sym);
                if (!r) throw SemanticError("Null operand for unary logical operator", this->lineno);
            }
            static basicType boolType(TYPE_BOOL);
            return &boolType;
        }
        default:
            throw SemanticError("Unknown expression operator '" + std::string(1, op) + "'", this->lineno);
    }
}

typeClass *lvalNode::semanticCheck(SymbolTable &sym) {
    if (!ident) throw SemanticError("Invalid identifier", this->lineno);
    if (isString) {
        static arrayType strType(new basicType(TYPE_CHAR), new Const(0));
        return &strType;
    }
    SymbolEntry *entry = sym.lookup(ident->name);
    if (!entry) throw SemanticError("Undeclared variable '" + ident->name + "'", this->lineno);
    typeClass *curType = entry->type;
    if (ind && !ind->empty()) {
        for (auto *idxExpr : *ind) {
            arrayType *arrT = dynamic_cast<arrayType*>(curType);
            if (!arrT) throw SemanticError("Variable '" + ident->name + "' is not an array", this->lineno);
            typeClass *idxType = idxExpr->semanticCheck(sym);
            static basicType intType(TYPE_INT);
            if (!sameType(idxType, &intType)) throw SemanticError("Array index for '" + ident->name + "' must be int", this->lineno);
            curType = arrT->getBaseType();
        }
    }
    return curType;
}

void stmtNode::semanticCheck(SymbolTable &sym) {
    if (stmtType == "vardecl") {
        if (!varType || !varNames) throw SemanticError("Malformed declaration", this->lineno);
        for (auto &n : *varNames) {
            if (sym.lookupCurrentScope(n)) {
                sym.printCurrentScope(std::cout);
                throw SemanticError("Redeclaration of variable '" + n + "'", this->lineno);
            }
            sym.addVariable(n, varType);
        }
    }
    else if (stmtType == "decl") {
        if (!funcDef || !funcDef->head || !funcDef->head->iden) throw SemanticError("Malformed function declaration", this->lineno);
        if (!sym.lookupFunction(funcDef->head->iden->name)) sym.addFunction(funcDef->head);
        else throw SemanticError("Redeclaration of function '" + funcDef->head->iden->name + "'", this->lineno);
    }
    else if (stmtType == "asgn") {
        if (!lval || !exp) throw SemanticError("Invalid assignment statement", this->lineno);
        typeClass *lt = lval->semanticCheck(sym);
        typeClass *rt = exp->semanticCheck(sym);
        if (lt->isArray() && !rt->isArray()) throw SemanticError("Invalid assignment: right-hand expression is not an array.", this->lineno);
        if (!lt->isArray() && rt->isArray()) throw SemanticError("Invalid assignment: cannot assign an array to a non-array element.", this->lineno);
        if (lt->isArray() && rt->isArray()) throw SemanticError("Invalid assignment: entire arrays cannot be directly assigned.", this->lineno);
        if (!sameType(lt, rt)) throw SemanticError("Type mismatch in assignment to '" + lval->ident->name + "' (" + typeToString(lt->getType()) + " cannot be converted to " + typeToString(rt->getType()) + ")", this->lineno);
    }
    else if (stmtType == "if") {
        if (!ifnode) throw SemanticError("Malformed if statement", this->lineno);
        sym.enterScope();
        if_semanticCheck(ifnode, sym);
        sym.exitScope();
    }
    else if (stmtType == "loop") {
        sym.enterLoop();
        sym.enterScope();
        stmtNode *bodyStmt = stmtBody;
        while (bodyStmt) {
            bodyStmt->semanticCheck(sym);
            bodyStmt = bodyStmt->stmtTail;
        }
        sym.exitScope();
        sym.exitLoop();
    }
    else if (stmtType == "pc") {
        if (!exp) throw SemanticError("ProcCall missing expression", this->lineno);
        sym.enterScope();
        exp->semanticCheck(sym);
        sym.exitScope();
    }
    else if (stmtType == "def") {
        if (!funcDef) throw SemanticError("Function definition missing body", this->lineno);
        funcDef->semanticCheck(sym);
    }
    else if (stmtType == "return") { if (exp) exp->semanticCheck(sym); }
    else if (stmtType == "break")  { if (!sym.insideLoop()) throw SemanticError("'break' used outside of any loop", this->lineno); }
    else if (stmtType == "continue") { if (!sym.insideLoop()) throw SemanticError("'continue' used outside of any loop", this->lineno); }

    if (stmtTail) stmtTail->semanticCheck(sym);
}

void fdefNode::semanticCheck(SymbolTable &sym) {
    if (!head || !head->iden) throw SemanticError("Invalid function definition", this->lineno);
    if (!sym.lookupFunction(head->iden->name)) sym.addFunction(head);

    sym.enterScope();
    if (head->params) param_semanticCheck(head->params, sym);
    if (body) body->semanticCheck(sym);
    sym.exitScope();
}

void submitBuiltInFunctions(SymbolTable &sym) {
    using std::vector;
    using std::string;

    auto *tInt   = new basicType(TYPE_INT);
    auto *tVoid  = new basicType(TYPE_VOID);
    auto *tChar  = new basicType(TYPE_CHAR);
    auto *tStr = new arrayType(tChar, new Const(0));

    // decl writeInteger: n as int
    sym.addFunction(new headerNode(
        tVoid,
        new paramNode(new vector<string>{"n"}, tInt, nullptr),
        new Id("writeInteger")
    ));

    // decl writeByte: b as byte
    sym.addFunction(new headerNode(
        tVoid,
        new paramNode(new vector<string>{"b"}, tChar, nullptr),
        new Id("writeByte")
    ));

    // decl writeChar: b as byte
    sym.addFunction(new headerNode(
        tVoid,
        new paramNode(new vector<string>{"b"}, tChar, nullptr),
        new Id("writeChar")
    ));

    // decl writeString: s as byte []
    sym.addFunction(new headerNode(
        tVoid,
        new paramNode(new vector<string>{"s"}, tStr, nullptr),
        new Id("writeString")
    ));

    // decl readInteger is int
    sym.addFunction(new headerNode(
        tInt, nullptr, new Id("readInteger")
    ));

    // decl readByte is byte
    sym.addFunction(new headerNode(
        tChar, nullptr, new Id("readByte")
    ));

    // decl readChar is byte
    sym.addFunction(new headerNode(
        tChar, nullptr, new Id("readChar")
    ));

    // decl readString: n as int, s as byte []
    sym.addFunction(new headerNode(
        tVoid,
        new paramNode(
            new vector<string>{"n"}, tInt,
            new paramNode(new vector<string>{"s"}, tStr, nullptr)
        ),
        new Id("readString")
    ));

    // decl extend is int: b as byte
    sym.addFunction(new headerNode(
        tInt,
        new paramNode(new vector<string>{"b"}, tChar, nullptr),
        new Id("extend")
    ));

    // decl shrink is byte: i as int
    sym.addFunction(new headerNode(
        tChar,
        new paramNode(new vector<string>{"i"}, tInt, nullptr),
        new Id("shrink")
    ));

    // decl strlen is int: s as byte []
    sym.addFunction(new headerNode(
        tInt,
        new paramNode(new vector<string>{"s"}, tStr, nullptr),
        new Id("strlen")
    ));

    // decl strcmp is int: s1 s2 as byte []
    sym.addFunction(new headerNode(
        tInt,
        new paramNode(
            new vector<string>{"s1"}, tStr,
            new paramNode(new vector<string>{"s2"}, tStr, nullptr)
        ),
        new Id("strcmp")
    ));

    // decl strcpy: trg src as byte []
    sym.addFunction(new headerNode(
        tVoid,
        new paramNode(
            new vector<string>{"trg"}, tStr,
            new paramNode(new vector<string>{"src"}, tStr, nullptr)
        ),
        new Id("strcpy")
    ));

    // decl strcat: trg src as byte []
    sym.addFunction(new headerNode(
        tVoid,
        new paramNode(
            new vector<string>{"trg"}, tStr,
            new paramNode(new vector<string>{"src"}, tStr, nullptr)
        ),
        new Id("strcat")
    ));
}