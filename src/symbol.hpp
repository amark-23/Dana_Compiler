#ifndef SYMBOL_HPP
#define SYMBOL_HPP

#include <vector>
#include "ast.hpp"

using namespace std;

class Const;

enum Type {
    TYPE_INT,
    TYPE_BYTE,
    TYPE_REAL,
    TYPE_CHAR,
    TYPE_VOID,
    TYPE_ARRAY
};

class typeClass {
public:
    virtual ~typeClass() = default;
    virtual void printNode(std::ostream& os) const = 0;
    virtual bool isArray() const { return false; }
};

inline std::ostream& operator<<(std::ostream& os, const typeClass& t) {
    t.printNode(os);
    return os;
}

class basicType : public typeClass {
public:
    basicType(Type t);
    void printNode(std::ostream& os) const override;
private:
    Type type;
};

class arrayType : public typeClass {
public:
    arrayType(typeClass* baseType, Const *s);
    void printNode(std::ostream& os) const override;
    bool isArray() const override;
private:
    typeClass* baseType;
    Const *size;
};

#endif