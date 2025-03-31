#include <iostream>
#include <vector>

class AST {
  public:
    virtual void printAST(std::ostream &out) const = 0;
};

inline std::ostream &operator<<(std::ostream &out, const AST &ast) {
  ast.printAST(out);
  return out;
}


class Stmt : public AST {
  public:
  private:
};

class Expr : public AST {
  public:
  private:
};

class Func : public Stmt {
  public:
  Func() : func_block() {}
  void append(Stmt *s) { func_block.push_back(s); }
  void printAST(std::ostream &out) const override {
    out << "Block(";
    bool first = true;
    for (const auto &s : func_block) {  // XXX: Fixme
      if (!first) out << ", ";
      first = false;
      out << *s;
    }
    out << ")";
  }
  private:
    std::vector<Stmt *> func_block;
 };

class Block : public Stmt {
 public:
  Block() : stmt_list() {}
  void append(Stmt *s) { stmt_list.push_back(s); }
  void printAST(std::ostream &out) const override {
    out << "Block(";
    bool first = true;
    for (const auto &s : stmt_list) {  // XXX: Fixme
      if (!first) out << ", ";
      first = false;
      out << *s;
    }
    out << ")";
  }
 private:
  std::vector<Stmt *> stmt_list;
};

class BinOp : public Expr {
 public:
  BinOp(Expr *e1, char o, Expr *e2) : expr1(e1), op(o), expr2(e2) {}
  void printAST(std::ostream &out) const override {
    out << op << "(" << *expr1 << ", " << *expr2 << ")";
  }
 private:
  Expr *expr1;
  char op;
  Expr *expr2;
};

class Const : public Expr {
 public:
  Const(int n) : num(n) {}
  void printAST(std::ostream &out) const override {
    out << "Const(" << num << ")";
  }
 private:
  int num;
};

class Id : public Expr {
 public:
  Id(char x) : var(x) {}
  void printAST(std::ostream &out) const override {
    out << "Id(" << var << ")";
  }
 private:
  char var;
};

class Let : public Stmt {
 public:
  Let(Id *lhs, Expr *rhs) : var(lhs), expr(rhs) {}
  void printAST(std::ostream &out) const override {
    out << "Let(" << *var << ", " << *expr << ")";
  }
 private:
  Id   *var;
  Expr *expr;
};

class For : public Stmt {
 public:
  For(Expr *e, Stmt *s) : expr(e), stmt(s) {}
  void printAST(std::ostream &out) const override {
    out << "For(" << *expr << ", " << *stmt << ")";
  }
 private:
  Expr *expr;
  Stmt *stmt;
};

class If : public Stmt {
 public:
  If(Expr *c, Stmt *s1, Stmt *s2 = nullptr) : cond(c), stmt1(s1), stmt2(s2) {}
  void printAST(std::ostream &out) const override {
    out << "If(" << *cond << ", " << *stmt1;
    if (stmt2 != nullptr) out << ", " << *stmt2;
    out << ")";
  }
 private:
  Expr *cond;
  Stmt *stmt1;
  Stmt *stmt2;
};

class Print : public Stmt {
 public:
  Print(Expr *e) : expr(e) {}
  void printAST(std::ostream &out) const override {
    out << "Print(" << *expr << ")";
  }
 private:
  Expr *expr;
};