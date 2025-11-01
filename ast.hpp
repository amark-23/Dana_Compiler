#ifndef AST_HPP
#define AST_HPP
#include <iostream>
#include <vector>
#include <string>
#include "symbol.hpp"

extern int yylineno;

class Id;
class Const;
class typeNode;
class paramNode;
class headerNode;
class exprNode;
class fcallNode;
class lvalNode;
class ifNode;
class stmtNode;
class fdefNode;
class typeClass;

class SymbolTable;

class Node {
    public:
        int lineno;
        Node() : lineno(yylineno) {}
        Node(int ln) : lineno(ln) {}
        virtual void printNode(std::ostream &out) const = 0;
};

inline std::ostream &operator<<(std::ostream &out, const Node &ast) {
    ast.printNode(out);
        return out;
}

class Id : public Node {
    public:
        Id(std::string s);
        std::string name;
        void printNode(std::ostream &out) const override;
};

class Const : public Node {
    public:
        Const(int v);
        int value;
        void printNode(std::ostream &out) const override;
};

class paramNode : public Node {
    public:
        paramNode(std::vector<std::string> *n, typeClass *type, paramNode *t);
        bool ref;
        std::vector<std::string> *names;
        typeClass *types;
        paramNode *tail;
        void printNode(std::ostream &out) const override;
}; 

class headerNode : public Node {
    public:
        headerNode(typeClass *t, paramNode *p, Id *i);
        typeClass *headType;
        paramNode *params;
        Id *iden;
        void printNode(std::ostream &out) const override;
};

class exprNode : public Node {
    public:
        exprNode(char c, lvalNode *l, Const *con, exprNode *left, exprNode *right, bool tf);
        fcallNode *func;
        char op;
        lvalNode *lval;
        Const *constant;
        exprNode *leftExpr;
        exprNode *rightExpr;
        bool tfFlag;
        void printNode(std::ostream &out) const override;
        typeClass *semanticCheck(SymbolTable &sym);
};

class fcallNode : public Node {
    public:
        fcallNode(Id *i);
        std::vector<exprNode*> *args;
        Id* iden;
        void printNode(std::ostream &out) const override;
};

class lvalNode : public Node {
    public:
        lvalNode(bool str, Id *i);
        std::vector<exprNode*> *ind;
        bool isString;
        Id *ident;
        void printNode(std::ostream &out) const override;
        typeClass *semanticCheck(SymbolTable &sym);
};

class ifNode : public Node {
    public:
        ifNode(exprNode *e, stmtNode *s);
        ifNode *tail;
        exprNode *cond;
        stmtNode *stmt;
        void printNode(std::ostream &out) const override;
};

class stmtNode : public Node {
    public:
        stmtNode(std::string type, stmtNode *body, stmtNode *tail, Id *i);
        fdefNode *funcDef;
        typeClass *varType;
        std::vector<std::string> *varNames;
        ifNode *ifnode;
        lvalNode *lval;
        exprNode *exp;
        std::string stmtType;
        stmtNode *stmtBody;
        stmtNode *stmtTail;
        Id *tag;
        void printNode(std::ostream &out) const override;
        void semanticCheck(SymbolTable &sym);
};

class fdefNode : public Node {
    public:
        fdefNode(headerNode *h, stmtNode *b);
        headerNode *head;
        stmtNode *body;
        void printNode(std::ostream &out) const override;
        void semanticCheck(SymbolTable &sym);
};

#endif