#include "symbol.hpp"

/* Types */

basicType::basicType(Type t) : type(t) {}
Type basicType::getType() const { return this->type; }
void basicType::printNode(std::ostream& os) const {
    switch (type) {
        case TYPE_INT: os << "int"; break;
        case TYPE_BYTE: os << "byte"; break;
        case TYPE_CHAR: os << "char"; break;
        case TYPE_VOID: os << "void"; break;
        case TYPE_BOOL: os << "bool"; break;
        default: os << "unknown"; break;
    }
}

arrayType::arrayType(typeClass* baseT, Const *s) : baseType(baseT), size(s) {}
bool arrayType::isArray() const { return true; }
Type arrayType::getType() const { return TYPE_ARRAY; }
typeClass* arrayType::getBaseType() const { return baseType; }
Const* arrayType::getSize() const { return size; }
void arrayType::printNode(std::ostream& os) const {
    if (baseType) baseType->printNode(os);
    else os << "NULL_BASE";
    os << "[";
    if (size) os << *size;
    else os << "NULL_SIZE";
    os << "]";
}

refType::refType(typeClass *baseT) : baseType(baseT) {}
bool refType::isRef() const { return true; }
Type refType::getType() const { return baseType->getType(); }
typeClass* refType::getBaseType() const { return baseType; }
void refType::printNode(std::ostream &os) const {
        os << "ref ";
        baseType->printNode(os);
}

/* SymbolEntry & SymbolTable */

SymbolEntry::SymbolEntry(std::string n, typeClass* t, bool param, bool cnst)
    : name(n), type(t), function(nullptr), isFunction(false), isParam(param), isConst(cnst) {}

SymbolEntry::SymbolEntry(std::string n, headerNode* h)
    : name(n), type(h ? h->headType : nullptr), function(h), isFunction(true),
      isParam(false), isConst(false) {}

void SymbolEntry::print(std::ostream& os) const {
    if (isFunction && function) {
        os << "[Function] " << name << " -> ";
        function->printNode(os);
    } else if (type) {
        os << (isParam ? "[Param] " : "[Var] ") << name << " : ";
        type->printNode(os);
    } else {
        os << "[Unknown Entry] " << name;
    }
}


SymbolTable::SymbolTable() {
    enterScope();
}

void SymbolTable::enterScope() {
    scopes.emplace_back();
}

void SymbolTable::exitScope() {
    if (scopes.empty()) throw std::runtime_error("SymbolTable::exitScope() called with no active scope");
    scopes.pop_back();
}

void SymbolTable::addVariable(std::string name, typeClass* type, bool isParam) {
    auto &current = scopes.back();
    if (current.find(name) != current.end()) {
        throw std::runtime_error("Variable '" + name + "' redeclared in same scope");
    }
    current[name] = new SymbolEntry(name, type, isParam, false);
}

void SymbolTable::addConstant(std::string name, typeClass* type) {
    auto &current = scopes.back();
    if (current.find(name) != current.end()) {
        throw std::runtime_error("Constant '" + name + "' redeclared in same scope");
    }
    current[name] = new SymbolEntry(name, type, false, true);
}

void SymbolTable::addFunction(headerNode* h) {
    if (!h || !h->iden) throw std::runtime_error("addFunction() called with invalid header");
    std::string name = h->iden->name;

    auto &globalScope = scopes.front();
    if (globalScope.find(name) != globalScope.end()) {
        throw std::runtime_error("Function '" + name + "' redeclared");
    }
    globalScope[name] = new SymbolEntry(name, h);
}

SymbolEntry* SymbolTable::lookup(std::string name) {
    for (int i = (int)scopes.size() - 1; i >= 0; --i) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end()) return it->second;
    }
    return nullptr;
}

SymbolEntry* SymbolTable::lookupCurrentScope(std::string name) {
    if (scopes.empty()) return nullptr;
    auto &current = scopes.back();
    auto it = current.find(name);
    if (it != current.end()) return it->second;
    return nullptr;
}

headerNode* SymbolTable::lookupFunction(std::string name) {
    for (int i = (int)scopes.size() - 1; i >= 0; --i) {
        auto it = scopes[i].find(name);
        if (it != scopes[i].end() && it->second->isFunction) {
            return it->second->function;
        }
    }
    return nullptr;
}

void SymbolTable::printCurrentScope(std::ostream& os) const {
    if (scopes.empty()) {
        os << "<no active scope>" << std::endl;
        return;
    }
    os << "---- Current Scope ----" << std::endl;
    for (const auto &pair : scopes.back()) {
        pair.second->print(os);
        os << std::endl;
    }
}

void SymbolTable::printAll(std::ostream& os) const {
    os << "==== Symbol Table ====" << std::endl;
    int level = 0;
    for (const auto &scope : scopes) {
        os << "Scope " << level++ << ":" << std::endl;
        for (const auto &pair : scope) {
            pair.second->print(os);
            os << std::endl;
        }
    }
    os << "=======================" << std::endl;
}