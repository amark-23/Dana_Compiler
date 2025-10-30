#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include "ast.hpp"

class Const;
class headerNode;

enum Type {
    TYPE_INT,
    TYPE_BYTE,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_BOOL
};

inline std::string typeToString(Type t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_BYTE: return "byte";
        case TYPE_CHAR: return "char";
        case TYPE_VOID: return "void";
        case TYPE_ARRAY: return "array";
        case TYPE_BOOL: return "bool";
        default: return "unknown";
    }
}

class typeClass {
public:
    virtual ~typeClass() = default;
    virtual void printNode(std::ostream& os) const = 0;
    virtual bool isArray() const { return false; }
    virtual Type getType() const = 0;
    virtual bool isRef() const { return false; }
};

inline std::ostream& operator<<(std::ostream& os, const typeClass& t) {
    t.printNode(os);
    return os;
}

class basicType : public typeClass {
public:
    basicType(Type t);
    void printNode(std::ostream& os) const override;
    Type getType() const override;
private:
    Type type;
};

class arrayType : public typeClass {
public:
    arrayType(typeClass* baseT, Const *s);
    void printNode(std::ostream& os) const override;
    bool isArray() const override;
    Type getType() const override;
    typeClass* getBaseType() const;
    Const* getSize() const;
private:
    typeClass* baseType;
    Const *size;
};

class refType : public typeClass {
public:
    refType(typeClass *baseT);
    bool isRef() const override;
    Type getType() const override;
    typeClass* getBaseType() const;
    void printNode(std::ostream &os) const override;
private:
    typeClass *baseType;
};

class SemanticError : public std::runtime_error {
public:
    int line;
    SemanticError(const std::string &msg, int ln) : std::runtime_error(msg), line(ln) {}
};

class SymbolEntry {
public:
    std::string name;
    typeClass* type;
    headerNode* function;
    bool isFunction;
    bool isParam;
    bool isConst;

    SymbolEntry(std::string n, typeClass* t = nullptr, bool param = false, bool cnst = false);
    SymbolEntry(std::string n, headerNode* h);

    void print(std::ostream& os) const;
};

class SymbolTable {
public:
    SymbolTable();

    void enterScope();
    void exitScope();

    void addVariable(std::string name, typeClass* type, bool isParam = false);
    void addConstant(std::string name, typeClass* type);
    void addFunction(headerNode* h);

    SymbolEntry* lookup(std::string name);
    SymbolEntry* lookupCurrentScope(std::string name);
    headerNode* lookupFunction(std::string name);

    int loopDepth = 0;

    void enterLoop() { loopDepth++; }
    void exitLoop()  { if (loopDepth > 0) loopDepth--; }
    bool insideLoop() const { return loopDepth > 0; }

    void printCurrentScope(std::ostream& os) const;
    void printAll(std::ostream& os) const;

private:
    std::vector<std::unordered_map<std::string, SymbolEntry*>> scopes;
};

void submitBuiltInFunctions(SymbolTable &sym);

#endif