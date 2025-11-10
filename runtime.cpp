#include "ast.hpp"
#include "runtime.hpp"
#include "symbol.hpp"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>

headerNode *RuntimeEnv::getFunctionHeader(const std::string &name) {
    fdefNode *fdef = getFunctionDef(name);
    if (!fdef) return nullptr;
    return fdef->head;
}

std::string process_escapes(const std::string& raw_str) {
    std::string result = "";
    for (size_t i = 0; i < raw_str.length(); ++i) {
        if (raw_str[i] == '\\' && i + 1 < raw_str.length()) {
            switch (raw_str[i + 1]) {
                case 'n':  result += '\n'; i++; break;
                case 't':  result += '\t'; i++; break;
                case 'r':  result += '\r'; i++; break;
                case '\\': result += '\\'; i++; break;
                case '"':  result += '"'; i++; break;
                default:
                    result += raw_str[i]; 
                    break;
            }
        } else {
            result += raw_str[i];
        }
    }
    return result;
}

static Value createDefaultValue(typeClass *t) {
    if (!t) return Value(0);
    if (auto *arr = dynamic_cast<arrayType*>(t)) {
        Const *sz = arr->getSize();
        int n = 0;
        if (sz) n = sz->value;
        if (n < 0) n = 0;
        std::vector<std::shared_ptr<Value>> vec;
        vec.reserve(n);
        typeClass *base = arr->getBaseType();
        for (int i = 0; i < n; ++i) {
            Value el = createDefaultValue(base);
            vec.push_back(std::make_shared<Value>(el));
        }
        return Value(vec);
    }
    if (auto *ref = dynamic_cast<refType*>(t)) {
        return createDefaultValue(ref->getBaseType());
    }
    if (auto *b = dynamic_cast<basicType*>(t)) {
        Type ty = b->getType();
        switch (ty) {
            case TYPE_INT:  return Value(0);
            case TYPE_CHAR: return Value(static_cast<char>(0));
            case TYPE_BYTE: return Value(static_cast<char>(0));
            case TYPE_BOOL: return Value(false);
            case TYPE_VOID: return Value(0);
            default: return Value(0);
        }
    }
    return Value(0);
}

Value Id::execute(RuntimeEnv &env) {
    (void)env;
    return Value(name);
}

Value Const::execute(RuntimeEnv &env) {
    (void)env;
    return Value(value);
}

void paramNode::bindParams(const std::vector<Value> &args, RuntimeEnv &env) {
    int idx = 0;
    paramNode *p = this;
    while (p && idx < (int)args.size()) {
        if (p->names) {
            for (auto &nm : *(p->names)) {
                if (idx >= (int)args.size()) return;
                env.setLocal(nm, std::make_shared<Value>(args[idx]));
                ++idx;
            }
        }
        p = p->tail;
    }
}

Value exprNode::execute(RuntimeEnv &env) {
    switch (op) {
        case 'c': {
            if (!constant) throw RuntimeError("Runtime: missing constant at line " + std::to_string(lineno));
            return Value(constant->value);
        }
        case 'x': {
            if (!constant) throw RuntimeError("Runtime: missing char constant at line " + std::to_string(lineno));
            return Value(static_cast<char>(constant->value));
        }
        case 'b': {
            return Value(tfFlag);
        }
        case 'i': {
            if (!lval) throw RuntimeError("Runtime: identifier expression missing lval at line " + std::to_string(lineno));
            return lval->execute(env);
        }
        case 'f': {
            if (!func || !func->iden) throw RuntimeError("Runtime: invalid function call at line " + std::to_string(lineno));
            return func->execute(env);
        }

        case '+': case '-': case '*': case '/': case '%': {
            if (!leftExpr) {
                Value rv = rightExpr->execute(env);
                int r = rv.asInt();
                if (op == '+') return Value(+r);
                return Value(-r);
            } else {
                int l = leftExpr->execute(env).asInt();
                int r = rightExpr->execute(env).asInt();
                switch (op) {
                    case '+': return Value(l + r);
                    case '-': return Value(l - r);
                    case '*': return Value(l * r);
                    case '/':
                        if (r == 0) throw RuntimeError("Runtime: division by zero at line " + std::to_string(lineno));
                        return Value(l / r);
                    case '%':
                        if (r == 0) throw RuntimeError("Runtime: modulo by zero at line " + std::to_string(lineno));
                        return Value(l % r);
                }
            }
            break;
        }

        case '=': case '<': case '>': case 'g': case 'l': case 'd': {
            Value lv = leftExpr ? leftExpr->execute(env) : Value(0);
            Value rv = rightExpr ? rightExpr->execute(env) : Value(0);
            // std::cout << "int check in every loop: " << lv.asInt() << " == " << rv.asInt() << " = " << (lv.asInt() == rv.asInt()) << std::endl;
            if (lv.isInt() && rv.isInt()) {
                int li = lv.asInt();
                int ri = rv.asInt();
                switch (op) {
                    case '=': return Value(li == ri);
                    case '<': return Value(li < ri);
                    case '>': return Value(li > ri);
                    case 'g': return Value(li >= ri);
                    case 'l': return Value(li <= ri);
                    case 'd': return Value(li != ri);
                }
            }

            if (lv.isChar() && rv.isChar()) {
                char lc = lv.asChar();
                char rc = rv.asChar();
                switch (op) {
                    case '=': return Value(lc == rc);
                    case '<': return Value(lc < rc);
                    case '>': return Value(lc > rc);
                    case 'g': return Value(lc >= rc);
                    case 'l': return Value(lc <= rc);
                    case 'd': return Value(lc != rc);
                }
            }

            if (lv.isBool() && rv.isBool()) {
                bool lb = lv.asBool();
                bool rb = rv.asBool();
                switch (op) {
                    case '=': return Value(lb == rb);
                    case 'd': return Value(lb != rb);
                    case '<': return Value(static_cast<int>(lb) < static_cast<int>(rb));
                    case '>': return Value(static_cast<int>(lb) > static_cast<int>(rb));
                    case 'g': return Value(static_cast<int>(lb) >= static_cast<int>(rb));
                    case 'l': return Value(static_cast<int>(lb) <= static_cast<int>(rb));
                }
            }

            if (lv.isString() && rv.isString()) {
                std::string ls = lv.asString();
                std::string rs = rv.asString();
                switch (op) {
                    case '=': return Value(ls == rs);
                    case 'd': return Value(ls != rs);
                    case '<': return Value(ls < rs);
                    case '>': return Value(ls > rs);
                    case 'g': return Value(ls >= rs);
                    case 'l': return Value(ls <= rs);
                }
            }

            throw RuntimeError("Runtime: incompatible types in comparison at line " + std::to_string(lineno));
        }

        case 'a': {
            bool l = leftExpr->execute(env).asBool();
            bool r = rightExpr->execute(env).asBool();
            return Value(l && r);
        }
        case 'o': {
            bool l = leftExpr->execute(env).asBool();
            bool r = rightExpr->execute(env).asBool();
            return Value(l || r);
        }
        case 'n': {
            bool r = rightExpr->execute(env).asBool();
            return Value(!r);
        }

        case '&': {
            int l = leftExpr->execute(env).asInt();
            int r = rightExpr->execute(env).asInt();
            return Value(l & r);
        }
        case '|': {
            int l = leftExpr->execute(env).asInt();
            int r = rightExpr->execute(env).asInt();
            return Value(l | r);
        }

        default:
            throw RuntimeError("Runtime: unknown operator in expr at line " + std::to_string(lineno));
    }

    throw RuntimeError("Runtime: unreachable code in exprNode::execute");
}

Value fcallNode::execute(RuntimeEnv &env) {
    if (!iden) throw RuntimeError("Runtime: invalid function identifier at line " + std::to_string(lineno));
    std::string fname = iden->name;

    std::vector<Value> evaluatedArgs;
    if (args) {
        for (auto *e : *args) {
            evaluatedArgs.push_back(e->execute(env));
        }
    }

    if (isBuiltin(fname)) {
        return callBuiltin(fname, evaluatedArgs, env);
    }

    return callUserFunction(fname, evaluatedArgs, env);
}

Value lvalNode::execute(RuntimeEnv &env) {
    if (isString) {
        std::string raw_literal = ident->name;
        std::string content = "";
        
        if (raw_literal.length() >= 2 && raw_literal.front() == '"' && raw_literal.back() == '"') {
            content = raw_literal.substr(1, raw_literal.length() - 2);
        } else {
             content = raw_literal;
        }
        content = process_escapes(content);
        return Value(content);
    }
    if (!ident) throw RuntimeError("Runtime: missing identifier at line " + std::to_string(lineno));
    auto sp = env.lookup(ident->name);
    if (!sp) {
        std::ostringstream ss;
        ss << "Runtime Error at line " << lineno << ": Undeclared variable '" << ident->name << "'";
        throw RuntimeError(ss.str());
    }

    if (!ind || ind->empty()) {
        return *sp;
    }

    Value current = *sp;
    for (auto *idxExpr : *ind) {
        int idx = idxExpr->execute(env).asInt();
        // std::cout << "lval execute: " << ident->name << " " << idxExpr->op << " " << idx;
        if (!current.isArray()) {
            throw RuntimeError("Runtime: variable '" + ident->name + "' is not an array at line " + std::to_string(lineno));
        }
        const auto &vec = current.asArray();
        // std::cout << " <= " << (int)vec.size() << std::endl;
        if (idx < 0 || idx >= (int)vec.size()) {
            throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
        }
        current = *vec[idx];
    }
    return current;
}

void lvalNode::assign(RuntimeEnv &env, const Value &val) {
    if (!ident) throw RuntimeError("Runtime: missing identifier on assign at line " + std::to_string(lineno));
    auto sp = env.lookup(ident->name);
    if (!sp) {
        std::ostringstream ss;
        ss << "Runtime Error at line " << lineno << ": Undeclared variable '" << ident->name << "'";
        throw RuntimeError(ss.str());
    }

    if (!ind || ind->empty()) {
        *sp = val;
        return;
    }

    std::shared_ptr<Value> container = sp;
    Value current = *container;

    int n = (int)ind->size();
    for (int i = 0; i < n - 1; ++i) {
        int idx = ind->at(i)->execute(env).asInt();
        if (!current.isArray()) throw RuntimeError("Runtime: not an array while assigning at line " + std::to_string(lineno));
        auto &vec = current.asArray();
        if (idx < 0 || idx >= (int)vec.size()) throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
        container = vec[idx];
        current = *container;
    }

    int finalIdx = ind->back()->execute(env).asInt();
    if (!current.isArray()) throw RuntimeError("Runtime: not an array while assigning at line " + std::to_string(lineno));
    auto &finalVec = (*container).asArray();
    if (finalIdx < 0 || finalIdx >= (int)finalVec.size()) throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
    if (!finalVec[finalIdx]) finalVec[finalIdx] = std::make_shared<Value>(val);
    else *finalVec[finalIdx] = val;
}

void ifNode::execute(RuntimeEnv &env) {
    if (ifCond) { 
        bool condv = ifCond->execute(env).asBool();
        if (condv) {
            RuntimeEnv inner(&env);
            if (ifStmtBody) {
                ifStmtBody->execute(inner);
            }
        } else {
            if (ifTail) {
                ifTail->execute(env);
            }
        }
    }
    else {
        RuntimeEnv inner(&env);
        if (ifStmtBody) {
            ifStmtBody->execute(inner);
        }
    }
}

void stmtNode::execute(RuntimeEnv &env) {
    if (stmtType == "vardecl") {
        if (!varType || !varNames) throw RuntimeError("Malformed declaration at line " + std::to_string(lineno));
        for (auto &n : *varNames) {
            Value def = createDefaultValue(varType);
            env.setLocal(n, std::make_shared<Value>(def));
        }
    }
    else if (stmtType == "decl") {
    }
    else if (stmtType == "asgn") {
        if (!lval || !exp) throw RuntimeError("Invalid assignment at line " + std::to_string(lineno));
        Value rv = exp->execute(env);
        lval->assign(env, rv);
    }
    else if (stmtType == "pc") {
        if (!exp) throw RuntimeError("Procedure call missing expression at line " + std::to_string(lineno));
        exp->execute(env);
    }
    else if (stmtType == "exit") {
        throw ReturnException(Value(0));
    }
    else if (stmtType == "return") {
        Value rv = Value(0);
        if (exp) rv = exp->execute(env);
        throw ReturnException(rv);
    }
    else if (stmtType == "if") {
        if (!ifnode) throw RuntimeError("Malformed if at line " + std::to_string(lineno));
        ifnode->execute(env);
    }
    else if (stmtType == "loop") {
        try {
            while (true) {
                try {
                    if (this->stmtBody) this->stmtBody->execute(env);
                } catch (const ContinueException &) { continue; }
            }
        } catch (const BreakException &) {}
    }
    else if (stmtType == "break") {
        throw BreakException();
    }
    else if (stmtType == "continue") {
        throw ContinueException();
    }
    else if (stmtType == "def") {
        env.registerFunction(funcDef->head->iden->name, funcDef);
    }

    if (this->stmtTail) this->stmtTail->execute(env);
}

Value fdefNode::execute(RuntimeEnv &env) {
    env.registerFunction(head->iden->name, this);
    return Value();
}

Value callUserFunction(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env) {
    fdefNode *def = env.getFunctionDef(name);
    if (!def) throw RuntimeError("Undefined function: " + name);

    headerNode *hdr = def->head;
    RuntimeEnv localEnv(&env);

    paramNode *param = hdr->params;
    size_t argIndex = 0;
    while (param) {
        for (const auto &n : *param->names) {
            if (argIndex >= args.size()) throw RuntimeError("Too few arguments to " + name);
            localEnv.setLocal(n, std::make_shared<Value>(args[argIndex++]));
        }
        param = param->tail;
    }

    try {
        if (def->body) def->body->execute(localEnv);
    } catch (const ReturnException &r) {
        return r.val;
    }

    return Value();
}

static bool isInteger(const Value &v) {
    return v.isInt();
}
static bool isArray(const Value &v) {
    return v.isArray();
}

static bool isString(const Value &v) {
    return v.isString();
}

static bool isChar(const Value &v) {
    return v.isChar();
}

bool isBuiltin(const std::string &name) {
    static const std::vector<std::string> builtins = {
        "writeString", "writeInteger", "writeChar", "writeByte",
        "readString", "readInteger", "readChar", "readByte",
        "strcmp", "strcpy", "strlen"
    };
    return std::find(builtins.begin(), builtins.end(), name) != builtins.end();
}

Value callBuiltin(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env) {
    if (name == "writeString") {
        if (args.size() != 1 || !isString(args[0])) {
            throw RuntimeError("writeString expects a single string argument");
        }
        std::cout << args[0].asString();
        return Value();
    }

    if (name == "writeInteger") {
        if (args.size() != 1 || !isInteger(args[0])) {
            throw RuntimeError("writeInteger expects a single integer argument");
        }
        std::cout << args[0].asInt();
        return Value();
    }

    if (name == "writeChar") {
        if (args.size() != 1 || !isChar(args[0])) {
            throw RuntimeError("writeChar expects a single integer argument");
        }
        std::cout << args[0].asChar();
        return Value();
    }

    if (name == "writeByte") {
        if (args.size() != 1 || !isChar(args[0])) {
            throw RuntimeError("writeByte expects a single integer argument");
        }
        std::cout << args[0].asInt();
        return Value();
    }

    if (name == "readString") {
        if (args.size() != 2 || !isInteger(args[0]) || !isArray(args[1])) {
            throw RuntimeError("readString expects two arguments");
        }
        // int n_max_size = args[0].asInt();
        // const auto& s_array = args[1].asArray();
        // int buffer_capacity = s_array.size();

        // if (buffer_capacity == 0) {
        //     return Value();
        // }
        // int max_chars_to_read = std::min(n_max_size - 1, buffer_capacity - 1);
        // if (max_chars_to_read < 0) max_chars_to_read = 0;

        // int i = 0;
        // char c;

        // for (i = 0; i < max_chars_to_read; ++i) {
        //     int next_char = std::cin.peek();
        //     if (next_char == EOF || next_char == '\n') {
        //         break;
        //     }
        //     std::cin.get(c);
        //     *s_array[i] = Value(c);
        // }

        // *s_array[i] = Value('\0');
        // if (std::cin.peek() == '\n') {
        //     std::cin.get();
        // }
        return Value("test");
    }

    if (name == "readInteger") {
        // int c = 0;
        // std::cin >> c;
        return Value(10);
    }

    if (name == "readChar") {
        // char c = '\0';
        // std::cin >> c;
        return Value(10);
    }

    if (name == "readByte") {
        // char c = '\0';
        // std::cin >> c;
        return Value(10);
    }

    if (name == "strcmp") {
        if (args.size() != 2 || !isString(args[0]) || !isString(args[1])) {
            throw RuntimeError("strcmp expects two strings");
        }
        int result = args[0].asString().compare(args[1].asString());
        return result;
    }

    if (name == "strcpy") {
        if (args.size() != 2 || !isString(args[0]) || !isString(args[1])) {
            throw RuntimeError("strcpy expects two strings");
        }
        std::string dest = args[1].asString();
        return dest;
    }

    if (name == "strlen") {
        if (args.size() != 1 || !isString(args[0])) {
            throw RuntimeError("strlen expects one string");
        }
        return static_cast<int>(args[0].asString().size());
    }

    if (name == "strcat") {
        if (args.size() != 2 || !isString(args[0]) || !isString(args[1])) {
            throw RuntimeError("strcat expects two strings");
        }
        std::string res = args[0].asString() + args[1].asString();
        return res;
    }

    throw RuntimeError("Unknown built-in function: " + name);
}