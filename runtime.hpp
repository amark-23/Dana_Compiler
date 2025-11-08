#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include <variant>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <memory>
#include <iostream>

// Forward declarations from AST
class exprNode;
class fcallNode;
class lvalNode;
class fdefNode;

// Forward declare Value for recursion
struct Value;

// Define Value as struct wrapping a variant that can include vectors of shared_ptr<Value>
struct Value {
    using Inner = std::variant<int, char, bool, std::string, std::vector<std::shared_ptr<Value>>>;
    Inner data;

    Value() = default;
    Value(int v) : data(v) {}
    Value(char v) : data(v) {}
    Value(bool v) : data(v) {}
    Value(const std::string &v) : data(v) {}
    Value(const std::vector<std::shared_ptr<Value>> &v) : data(v) {}

    // helpers
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

// Runtime environment to hold variable values
struct RuntimeEnv {
    std::unordered_map<std::string, std::shared_ptr<Value>> vars;
    std::unordered_map<std::string, fdefNode*> functions;
    RuntimeEnv *parent;

    explicit RuntimeEnv(RuntimeEnv *p = nullptr) : parent(p) {}

    // lookup variable (recursively)
    std::shared_ptr<Value> lookup(const std::string &name) {
        auto it = vars.find(name);
        if (it != vars.end()) return it->second;
        if (parent) return parent->lookup(name);
        return nullptr;
    }

    // set variable locally
    void setLocal(const std::string &name, std::shared_ptr<Value> val) { vars[name] = val; }

    // assign or create variable
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
};

// runtime builtins (to be implemented later)
bool isBuiltin(const std::string &name);
Value callBuiltin(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env);
Value callUserFunction(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env);

// runtime exceptions
struct RuntimeError : public std::runtime_error { using std::runtime_error::runtime_error; };

struct ReturnException : public RuntimeError {
    Value val;
    explicit ReturnException(const Value &v) : RuntimeError("return"), val(v) {}
};

struct BreakException : public RuntimeError { BreakException(): RuntimeError("break") {} };
struct ContinueException : public RuntimeError { ContinueException(): RuntimeError("continue") {} };

#endif // RUNTIME_HPP
