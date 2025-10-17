#ifndef AST_HPP
#define AST_HPP
#include <iostream>
#include <vector>
#include <string>

using namespace std;

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

class Node {
  public:
    virtual void printNode(std::ostream &out) const = 0;
};

inline std::ostream &operator<<(std::ostream &out, const Node &ast) {
  ast.printNode(out);
  return out;
}

class Id : public Node {
  public:
    Id(string s);
    string name;

    void printNode(std::ostream &out) const override;
};

class Const : public Node {
  public:
    Const(int v);
    int value;

    void printNode(std::ostream &out) const override;
};

class typeNode : public Node {
  public:
    typeNode(string typeName, typeNode* next, Const *c = nullptr);
    Const *con;
    string type;
    typeNode* nextType;

    void printNode(std::ostream &out) const override;
};

class paramNode : public Node {
  public:
    paramNode(vector<string> *n, typeNode *type, paramNode *t);
    bool ref;
    vector<string> *names;
    typeNode *types;
    paramNode *tail;

    void printNode(std::ostream &out) const override;
}; 

class headerNode : public Node {
  public:
    headerNode(typeNode *t, paramNode *p, Id *i);
    typeNode *headType;
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
};

class fcallNode : public Node {
  public:
    fcallNode(Id *i);
    vector<exprNode*> *args;
    Id* iden;

    void printNode(std::ostream &out) const override;
};

class lvalNode : public Node {
  public:
    lvalNode(bool str, Id *i);
    vector<exprNode*> *ind;
    bool isString;
    Id *ident;

    void printNode(std::ostream &out) const override;
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
    stmtNode(string type, stmtNode *body, stmtNode *tail, Id *i);
    stmtNode *tail;
    fdefNode *funcDef;
    typeNode *varType;
    vector<string> *varNames;
    ifNode *ifnode;
    lvalNode *lval;
    exprNode *exp;
    string stmtType;
    stmtNode *stmtBody;
    stmtNode *stmtTail;
    Id *tag;

    void printNode(std::ostream &out) const override;
};

class fdefNode : public Node {
  public:
    fdefNode(headerNode *h, stmtNode *b);
    headerNode *head;
    stmtNode *body;

    void printNode(std::ostream &out) const override;
};

#endif