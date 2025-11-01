#include "ast.hpp"
#include <iostream>
#include <vector>
#include <string>

Id::Id(std::string s) : Node(), name(s) {}
void Id::printNode(std::ostream &out) const {
    out << name;
}


Const::Const(int v) : Node(), value(v) {}
void Const::printNode(std::ostream &out) const {
    out << value;
}


paramNode::paramNode(std::vector<std::string> *n, typeClass *type, paramNode *t) : Node(), names(n), types(type), tail(t) {}
void paramNode::printNode(std::ostream &out) const {
    for (const auto &name : *(names)) {
        out << *types << " " << name;
        if (tail || name != names->back()) out << ", ";
    }
}


headerNode::headerNode(typeClass *t, paramNode *p, Id *i) : Node(), headType(t), params(p), iden(i) {}
void headerNode::printNode(std::ostream &out) const {
    out << "Header( " << *iden << ", " << *headType;

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


exprNode::exprNode(char c, lvalNode *l, Const *con, exprNode *left, exprNode *right, bool tf) : Node(), op(c), lval(l), constant(con), leftExpr(left), rightExpr(right), tfFlag(tf) {}
void exprNode::printNode(std::ostream &out) const {
    switch (op) {
    case 'c': out << *constant;
        break;
    case 'i': out << *lval;
        break;
    case 'f': out << *func;
        break;
    case 'x': out << "0x" << std::hex << constant->value << std::dec;
        break;
    case '+': case '-': case '*': case '/': case '%': case '=': case '<': case '>': case '!': case '&': case '|':
        if (leftExpr) out << "(" << *leftExpr << " " << op << " " << *rightExpr << ")";
        else out << "(" << op << *rightExpr << ")";
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


fcallNode::fcallNode(Id *i) : Node(), iden(i) {}
void fcallNode::printNode(std::ostream &out) const {
    out << "FuncCall(" << *iden;
    if (args) {
        out << ", Arguments(";
        for (const auto &i : *args) {
            out << *i;
            if (i != args->back()) out << ", ";
        }
        out << ")";
    }
    out << ")";
}


lvalNode::lvalNode(bool str, Id *i) : Node(), isString(str), ident(i) { ind = new std::vector<exprNode*>(); }
void lvalNode::printNode(std::ostream &out) const {
    out << *ident;
    for (const auto &index : *ind) {
        out << "[" << *index << "]";
    }
}


ifNode::ifNode(exprNode *e, stmtNode *s) : Node(), cond(e), stmt(s) {}
void ifNode::printNode(std::ostream &out) const {
    auto *statement = stmt;
    if (tail == nullptr && cond) {
        out << " else if " << *cond<< " {"; 
        while(statement) {
            out << *statement;
            if (statement->stmtTail) out << ", ";
            statement = statement->stmtTail;
        }
    }
    else if (tail == nullptr && cond == nullptr) {
        out << " else {";
        while(statement) {
            out << *statement;
            if (statement->stmtTail) out << ", ";
            statement = statement->stmtTail;
        }
    }
    else if (cond) {
        out << " else if " << *cond << " {";
        while(statement) {
            out << *statement;
            if (statement->stmtTail) out << ", ";
            statement = statement->stmtTail;
        }
    }
    else out << "unknown";
    out << "}";
}


stmtNode::stmtNode(std::string type, stmtNode *body, stmtNode *tail, Id *i) : Node(), stmtType(type), stmtBody(body), stmtTail(tail), tag(i) {}
void stmtNode::printNode(std::ostream &out) const {
    if (stmtType == "asgn") out << *lval << " := " << *exp;
    else if (stmtType == "skip") out << "skip";
    else if (stmtType == "exit") out << "exit";
    else if (stmtType == "return") out << "return: " << *exp;
    else if (stmtType == "break") {
        out << "break";
        if (tag) out << ": " << *tag;
    }
    else if (stmtType == "continue") {
        out << "continue";
        if (tag) out << ": " << *tag;
    }
    else if (stmtType == "if") {
        out << "if " << *ifnode->cond << " {";
        auto *ifStmt = ifnode->stmt;
        while (ifStmt) {
            out << *ifStmt;
            if (ifStmt->stmtTail) out << ", ";
            ifStmt = ifStmt->stmtTail;
        }
        out << "}";
        auto *ifTail = ifnode->tail;
        while (ifTail) {
            out << *ifTail;
            ifTail = ifTail->tail;
        }
    }
    else if (stmtType == "loop") {
        out << "Loop( " << (tag ? "tag: " + tag->name + ", " : "");
        auto *current = stmtBody;
        while (current) {
            out << *current << ", ";
            current = current->stmtTail;
        }
        out << "endloop )";
    }
    else if (stmtType == "pc") out << "ProcCall: " << *exp;
    else if (stmtType == "def") out << *funcDef;
    else if (stmtType == "decl") out << "FuncDecl( " << *funcDef->head << " )";
    else if (stmtType == "vardecl") {
        out << "VarDecl( ";
        for (const auto &name : *(varNames)) {
            out << *varType << " " << name;
            if (name != varNames->back()) out << ", ";
        }
        out << " )";
    }
    else out << "unknown";
}


fdefNode::fdefNode(headerNode *h, stmtNode *b) : Node(), head(h), body(b) {}
void fdefNode::printNode(std::ostream &out) const {
    out << "FuncDef( " << *(head) << " {\n";
    auto *current = body;
    while (current) {
        out << "  " << *current << "\n";
        current = current->stmtTail;
    }
    out << "})";
}