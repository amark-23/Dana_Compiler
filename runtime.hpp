#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <memory>
#include <iostream>

class exprNode;
class fcallNode;
class lvalNode;
class fdefNode;
class headerNode;

struct Value;

struct Value {
    using Inner = std::variant<int, char, bool, std::string, std::vector<std::shared_ptr<Value>>>;
    Inner data;

    Value() = default;
    Value(int v) : data(v) {}
    Value(char v) : data(v) {}
    Value(bool v) : data(v) {}
    Value(const std::string &v) : data(v) {}
    Value(const std::vector<std::shared_ptr<Value>> &v) : data(v) {}

    bool isInt() const    { return std::holds_alternative<int>(data); }
    bool isChar() const   { return std::holds_alternative<char>(data); }
    bool isBool() const   { return std::holds_alternative<bool>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isArray() const  { return std::holds_alternative<std::vector<std::shared_ptr<Value>>>(data); }

    int asInt() const { return std::get<int>(data); }
    char asChar() const { return std::get<char>(data); }
    bool asBool() const {
        if (std::holds_alternative<bool>(data)) return std::get<bool>(data);
        if (std::holds_alternative<int>(data)) return std::get<int>(data) != 0;
        throw std::runtime_error("Type error: expected bool");
    }
    std::string asString() const { return std::get<std::string>(data); }

    std::vector<std::shared_ptr<Value>>& asArray() {
        return std::get<std::vector<std::shared_ptr<Value>>>(data);
    }
    const std::vector<std::shared_ptr<Value>>& asArray() const {
        return std::get<std::vector<std::shared_ptr<Value>>>(data);
    }
};

struct RuntimeEnv {
    std::unordered_map<std::string, std::shared_ptr<Value>> vars;
    std::unordered_map<std::string, fdefNode*> functions;
    RuntimeEnv *parent;

    explicit RuntimeEnv(RuntimeEnv *p = nullptr) : parent(p) {}

    std::shared_ptr<Value> lookup(const std::string &name) {
        auto it = vars.find(name);
        if (it != vars.end()) return it->second;
        if (parent) return parent->lookup(name);
        return nullptr;
    }

    void setLocal(const std::string &name, std::shared_ptr<Value> val) { vars[name] = val; }

    void set(const std::string &name, std::shared_ptr<Value> val) {
        std::shared_ptr<Value> existing = lookup(name);
        if (existing) *existing = *val;
        else setLocal(name, val);
    }

    void registerFunction(const std::string &name, fdefNode *def) { functions[name] = def; }
    fdefNode *getFunctionDef(const std::string &name) {
        auto it = functions.find(name);
        return (it != functions.end()) ? it->second : nullptr;
    }

    headerNode *getFunctionHeader(const std::string &name);
};

bool isBuiltin(const std::string &name);
Value callBuiltin(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env);
Value callUserFunction(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env);

struct RuntimeError : public std::runtime_error { using std::runtime_error::runtime_error; };

struct ReturnException : public RuntimeError {
    Value val;
    explicit ReturnException(const Value &v) : RuntimeError("return"), val(v) {}
};

struct BreakException : public RuntimeError { BreakException(): RuntimeError("break") {} };
struct ContinueException : public RuntimeError { ContinueException(): RuntimeError("continue") {} };

#endif
