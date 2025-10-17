#include "ast.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

Id::Id(string s) : name(s) {}
void Id::printNode(std::ostream &out) const {
    // out << "Id(" << name << ")";
    out << name;
}


Const::Const(int v) : value(v) {}
void Const::printNode(std::ostream &out) const {
    // out << "Const("<< value << ")";
    out << value;
}


typeNode::typeNode(string typeName, typeNode* next, Const *c) : type(typeName), nextType(next), con(c) {}
void typeNode::printNode(std::ostream &out) const {
    out << type;
    if (nextType) {
        out << " of " << *nextType;
    }
    if (con) {
        con->value == 0 ? out << "[]" : out << "[" << *(con) << "]";
    }
}


paramNode::paramNode(vector<string> *n, typeNode *type, paramNode *t) : names(n), types(type), tail(t) {}
void paramNode::printNode(std::ostream &out) const {
    for (const auto &name : *(names)) {
    if (ref) {
        out << "ref ";
    }
    out << *types << " " << name;
    if (tail || name != names->back()) {
        out << ", ";
    }
    }
}


headerNode::headerNode(typeNode *t, paramNode *p, Id *i) : headType(t), params(p), iden(i) {}
void headerNode::printNode(std::ostream &out) const {
    out << "Header( " << *iden << ", ";
    headType == NULL ? out << "void" : out << *headType;

    paramNode *current = params;
    if (current != NULL) {
    out << ", Parameters( ";
    while (current) {
        out << *current;
        current = current->tail;
    }
    out << " )";
    }
    out << " )";
}


exprNode::exprNode(char c, lvalNode *l, Const *con, exprNode *left, exprNode *right, bool tf) : op(c), lval(l), constant(con), leftExpr(left), rightExpr(right), tfFlag(tf) {}
void exprNode::printNode(std::ostream &out) const {
    switch (op)
    {
    case 'c': out << *constant;
        break;
    case 'i': out << *lval;
        break;
    case 'f': out << *func;
        break;
    case 'x': out << "0x" << std::hex << constant->value << std::dec;
        break;
    case '+': case '-': case '*': case '/': case '%': case '=': case '<': case '>': case '!': case '&': case '|':
        if (leftExpr) {
            out << "(" << *leftExpr << " " << op << " " << *rightExpr << ")";
        } else {
            out << "(" << op << *rightExpr << ")";
        }
        break;
    case 'b': out << (tfFlag ? "true" : "false");
        break;
    case 'g': out << "(" << *leftExpr << " >= " << *rightExpr << ")";
        break;
    case 'l': out << "(" << *leftExpr << " <= " << *rightExpr << ")";
        break;
    case 'd': out << "(" << *leftExpr << " <> " << *rightExpr << ")";
        break;
    case 'a': out << "(" << *leftExpr << " and " << *rightExpr << ")";
        break;
    case 'o': out << "(" << *leftExpr << " or " << *rightExpr << ")";
        break;
    case 'n': out << "(" << " not " << *rightExpr << ")";
        break;
    default:
        out << "unknown";
        break;
    }
}


fcallNode::fcallNode(Id *i) : iden(i) {}
void fcallNode::printNode(std::ostream &out) const {
    out << "FuncCall(";
    out << *iden;
    if (args) {
        out << ", ";
        for (const auto &i : *args) {
            out << *i;
            if (i != args->back()) out << ", ";
        }
    }
    out << ")";
}


lvalNode::lvalNode(bool str, Id *i) : isString(str), ident(i) { ind = new vector<exprNode*>(); }
void lvalNode::printNode(std::ostream &out) const {
    out << *ident;
    for (const auto &index : *ind) {
        out << "[" << *index << "]";
    }
}


ifNode::ifNode(exprNode *e, stmtNode *s) : cond(e), stmt(s) {}
void ifNode::printNode(std::ostream &out) const {
    auto *statement = stmt;
    if (tail == nullptr && cond) {
        out << " else if " << *cond<< " {"; 
        while(statement) {
            out << *statement;
            if (statement->tail) { out << ", "; }
            statement = statement->tail;
        }
        out << "}";
    } else if (tail == nullptr && cond == nullptr) {
        out << " else {";
        while(statement) {
            out << *statement;
            if (statement->tail) { out << ", "; }
            statement = statement->tail;
        }
        out << "}";
    }
}


stmtNode::stmtNode(string type, stmtNode *body, stmtNode *tail, Id *i) : stmtType(type), stmtBody(body), stmtTail(tail), tag(i) {}
void stmtNode::printNode(std::ostream &out) const {
    if (stmtType == "asgn") {
    out << *lval << " := " << *exp;
    } else if (stmtType == "skip") {
    out << "skip";
    } else if (stmtType == "exit") {
    out << "exit";
    } else if (stmtType == "return") {
    out << "return: " << *exp;
    } else if (stmtType == "break") {
    out << "break";
    if (tag) {
        out << ": " << *tag;
    }
    } else if (stmtType == "continue") {
    out << "continue";
    if (tag) {
        out << ": " << *tag;
    }
    } else if (stmtType == "if") {
    auto *current = ifnode;
    auto *statement = current->stmt;
    out << "if " << *current->cond << " {";
    while (statement) {
        out << *statement;
        if (statement->tail) { out << ", "; }
        statement = statement->tail;
    }
    out << "}";
    current = current->tail;
    while (current) {
        out << *current;
        current = current->tail;
    }
    } else if (stmtType == "loop") {
    out << "Loop( " << (tag ? "tag: " + tag->name + ", " : "");
    auto *current = stmtBody;
    while (current) {
        out << *current << ", ";
        current = current->tail;
    }
    out << "endloop )";
    } else if (stmtType == "fc") {
    out << "FuncCall( " << *exp;
    } else if (stmtType == "pc") {
    out << "ProcCall: " << *exp;
    } else if (stmtType == "def") {
    out << *funcDef;
    } else if (stmtType == "decl") {
    out << "VarDecl( ";
    for (const auto &name : *(varNames)) {
        out << *varType << " " << name;
        if (name != varNames->back()) {
        out << ", ";
        }
    }
    out << " )";
    } else {
    out << "uknown";
    }
}


fdefNode::fdefNode(headerNode *h, stmtNode *b) : head(h), body(b) {}
void fdefNode::printNode(std::ostream &out) const {
    out << "FuncDef( " << *(head) << " {\n";
    auto *current = body;
    while (current) {
    out << "  " << *current << "\n";
    current = current->tail;
    }
    out << "})";
}