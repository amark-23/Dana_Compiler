#include "symbol.hpp"

basicType::basicType(Type t) : type(t) {}
void basicType::printNode(std::ostream& os) const {
    switch (type) {
        case TYPE_INT: os << "int"; break;
        case TYPE_BYTE: os << "byte"; break;
        case TYPE_REAL: os << "real"; break;
        case TYPE_CHAR: os << "char"; break;
        case TYPE_VOID: os << "void"; break;
        default: os << "unknown"; break;
    }
}

arrayType::arrayType(typeClass* baseType, Const *s) : baseType(baseType), size(s) {}
void arrayType::printNode(std::ostream& os) const {
    baseType->printNode(os);
    os << "[" << *size << "]";
}
bool arrayType::isArray() const { return true; }