#include "ast.hpp"
#include "runtime.hpp"
#include "symbol.hpp"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>

static Value createDefaultValue(typeClass *t);

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
        } else result += raw_str[i];
    }
    return result;
}

static Value build_recursive_array(typeClass *base, const std::vector<int>& dims, int depth) {
    if (depth == (int)dims.size()) {
        if (auto *ref = dynamic_cast<refType*>(base)) return createDefaultValue(ref->getBaseType());
        if (auto *b = dynamic_cast<basicType*>(base)) {
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
    int n = dims[depth];
    if (n < 0) n = 0;
    std::vector<std::shared_ptr<Value>> vec;
    vec.reserve(n);
    for (int i = 0; i < n; ++i) {
        Value el = build_recursive_array(base, dims, depth + 1);
        vec.push_back(std::make_shared<Value>(el));
    }
    return Value(vec);
}

static Value createDefaultValue(typeClass *t) {
    if (!t) return Value(0);
    if (dynamic_cast<arrayType*>(t)) {
        std::vector<int> dimensions;
        typeClass* finalBase = t;
        typeClass* current = t;
        while (auto* current_arr = dynamic_cast<arrayType*>(current)) {
            Const *sz = current_arr->getSize();
            int n = 0;
            if (sz) n = sz->value;
            dimensions.push_back(n); 
            
            finalBase = current_arr->getBaseType();
            current = finalBase;
        }
        std::reverse(dimensions.begin(), dimensions.end());
        return build_recursive_array(finalBase, dimensions, 0);
    }
    if (auto *ref = dynamic_cast<refType*>(t)) return createDefaultValue(ref->getBaseType());
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

std::shared_ptr<Value> exprNode::getReference(RuntimeEnv &env) {
    if (op == 'i' && lval) return lval->getReference(env);
    throw RuntimeError("Runtime: expression is not an l-value, cannot pass as ref at line " + std::to_string(lineno));
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
                if (rv.isInt()) {
                    int r = rv.asInt();
                    if (op == '+') return Value(+r);
                    return Value(-r);
                }
                if (rv.isChar()) {
                    char r = rv.asChar(); 
                    if (op == '+') return Value(+r);
                    return Value(-r);
                }
                throw RuntimeError("Runtime: unary +/- on non-numeric type at line " + std::to_string(lineno));
            } else {
                Value lv = leftExpr->execute(env);
                Value rv = rightExpr->execute(env);
                if (lv.isInt() && rv.isInt()) {
                    int l = lv.asInt();
                    int r = rv.asInt();
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
                if (lv.isChar() && rv.isChar()) {
                    char l = lv.asChar();
                    char r = rv.asChar();
                    switch (op) {
                        case '+': return Value(static_cast<char>(l + r));
                        case '-': return Value(static_cast<char>(l - r));
                        case '*': return Value(static_cast<char>(l * r));
                        case '/':
                            if (r == 0) throw RuntimeError("Runtime: division by zero at line " + std::to_string(lineno));
                            return Value(static_cast<char>(l / r));
                        case '%':
                            if (r == 0) throw RuntimeError("Runtime: modulo by zero at line " + std::to_string(lineno));
                            return Value(static_cast<char>(l % r));
                    }
                }
                if (lv.isInt() && rv.isChar()) {
                    int l = lv.asInt();
                    int r = static_cast<int>(rv.asChar());
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
                if (lv.isChar() && rv.isInt()) {
                    int l = static_cast<int>(lv.asChar());
                    int r = rv.asInt();
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
                
                throw RuntimeError("Runtime: invalid types for arithmetic operator at line " + std::to_string(lineno));
            }
            break;
        }
        case '=': case '<': case '>': case 'g': case 'l': case 'd': {
            Value lv = leftExpr ? leftExpr->execute(env) : Value(0);
            Value rv = rightExpr ? rightExpr->execute(env) : Value(0);
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
            if (!l) return Value(false);
            return Value(rightExpr->execute(env).asBool()); 
        }
        case 'o': {
            bool l = leftExpr->execute(env).asBool();
            if (l) return Value(true);
            return Value(rightExpr->execute(env).asBool());
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
    fdefNode *def = env.getFunctionDef(fname);
    if (!def && !isBuiltin(fname)) throw RuntimeError("Undefined function: " + fname);
    std::vector<std::shared_ptr<Value>> evaluatedArgs;
    if (args) {
        paramNode *currentParam = (def ? def->head->params : nullptr);
        int paramNameIdx = 0;
        for (auto *argExpr : *args) {
            bool isRef = false;
            if (currentParam) {
                isRef = currentParam->ref;
                paramNameIdx++;
                if (paramNameIdx >= (int)currentParam->names->size()) {
                    currentParam = currentParam->tail;
                    paramNameIdx = 0;
                }
            }
            if (isRef) evaluatedArgs.push_back(argExpr->getReference(env));
            else evaluatedArgs.push_back(std::make_shared<Value>(argExpr->execute(env)));
        }
    }
    if (isBuiltin(fname)) {
        std::vector<Value> builtinArgs;
        for (auto &sp : evaluatedArgs) {
            builtinArgs.push_back(*sp);
        }
        return callBuiltin(fname, builtinArgs, env);
    }
    return callUserFunction(fname, evaluatedArgs, env);
}

std::shared_ptr<Value> lvalNode::getReference(RuntimeEnv &env) {
    if (!ident) throw RuntimeError("Runtime: missing identifier for ref at line " + std::to_string(lineno));
    auto sp = env.lookup(ident->name);
    if (!sp) {
        std::ostringstream ss;
        ss << "Runtime Error at line " << lineno << ": Undeclared variable '" << ident->name << "'";
        throw RuntimeError(ss.str());
    }
    if (!ind || ind->empty()) return sp;
    std::shared_ptr<Value> container = sp;
    Value current = *container;
    int n = (int)ind->size();
    for (int i = 0; i < n; ++i) {
        int idx = ind->at(i)->execute(env).asInt();
        if (!current.isArray()) throw RuntimeError("Runtime: not an array for ref at line " + std::to_string(lineno));
        auto &vec = current.asArray();
        if (idx < 0 || idx >= (int)vec.size()) throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
        container = vec[idx];
        if (i < n - 1) current = *container;
    }
    return container;
}

Value lvalNode::execute(RuntimeEnv &env) {
    if (isString) {
        std::string raw_literal = ident->name;
        std::string content = "";
        if (raw_literal.length() >= 2 && raw_literal.front() == '"' && raw_literal.back() == '"') content = raw_literal.substr(1, raw_literal.length() - 2);
        else content = raw_literal;
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
    if (!ind || ind->empty()) { return *sp; }
    Value current = *sp;
    for (auto *idxExpr : *ind) {
        int idx = idxExpr->execute(env).asInt();
        if (!current.isArray()) throw RuntimeError("Runtime: variable '" + ident->name + "' is not an array at line " + std::to_string(lineno));
        const auto &vec = current.asArray();
        if (idx < 0 || idx >= (int)vec.size()) throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
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
            if (ifStmtBody) ifStmtBody->execute(inner);
        } else {
            if (ifTail) ifTail->execute(env);
        }
    }
    else {
        RuntimeEnv inner(&env);
        if (ifStmtBody) ifStmtBody->execute(inner);
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
    else if (stmtType == "decl") {}
    else if (stmtType == "asgn") {
        if (!lval || !exp) throw RuntimeError("Invalid assignment at line " + std::to_string(lineno));
        Value rv = exp->execute(env);
        lval->assign(env, rv);
    }
    else if (stmtType == "pc") {
        if (!exp) throw RuntimeError("Procedure call missing expression at line " + std::to_string(lineno));
        exp->execute(env);
    }
    else if (stmtType == "exit") { throw ReturnException(Value(0)); }
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
        std::string loopName = (this->tag ? this->tag->name : "");
        try {
            while (true) {
                try {
                    if (this->stmtBody) this->stmtBody->execute(env);
                } catch (const ContinueException &e) { 
                    if (e.targetName == "" || e.targetName == loopName) { continue; }
                    else { throw; }
                }
            }
        } catch (const BreakException &e) {
            if (e.targetName == "" || e.targetName == loopName) {}
            else { throw; }
        }
    }
    else if (stmtType == "break") {
        if (tag) throw BreakException(tag->name);
        else throw BreakException();
    }
    else if (stmtType == "continue") {
        if (tag) throw ContinueException(tag->name);
        else throw ContinueException();
    }
    else if (stmtType == "def") { env.registerFunction(funcDef->head->iden->name, funcDef); }
    if (this->stmtTail) this->stmtTail->execute(env);
}

Value fdefNode::execute(RuntimeEnv &env) {
    env.registerFunction(head->iden->name, this);
    return Value();
}

static bool isInteger(const Value &v) {
    return v.isInt();
}
static bool isArray(const Value &v) {
    return v.isArray();
}

static bool isChar(const Value &v) {
    return v.isChar();
}

bool isBuiltin(const std::string &name) {
    static const std::vector<std::string> builtins = {
        "writeString", "writeInteger", "writeChar", "writeByte",
        "readString", "readInteger", "readChar", "readByte",
        "strcmp", "strcpy", "strlen", "strcat",
        "extend", "shrink"
    };
    return std::find(builtins.begin(), builtins.end(), name) != builtins.end();
}

Value callUserFunction(const std::string &name, const std::vector<std::shared_ptr<Value>> &args, RuntimeEnv &env) {
    fdefNode *def = env.getFunctionDef(name);
    if (!def) throw RuntimeError("Undefined function: " + name);
    RuntimeEnv localEnv(def->definition_env ? def->definition_env : &env); 
    headerNode *hdr = def->head;
    paramNode *param = hdr->params;
    size_t argIndex = 0;
    while (param) {
        typeClass* expectedType = param->types; 
        arrayType* arrType = dynamic_cast<arrayType*>(expectedType);
        basicType* baseType = arrType ? dynamic_cast<basicType*>(arrType->getBaseType()) : nullptr;
        bool isExpectedByteArray = arrType && baseType && (baseType->getType() == TYPE_CHAR || baseType->getType() == TYPE_BYTE);
        for (const auto &n : *param->names) {
            if (argIndex >= args.size()) {
                throw RuntimeError("Too few arguments to " + name);
            }
            const Value& arg = *args[argIndex];
            if (isExpectedByteArray && arg.isString()) {
                std::string literal = arg.asString();
                std::vector<std::shared_ptr<Value>> vec;
                vec.reserve(literal.length() + 1);
                for (char c : literal) {
                    vec.push_back(std::make_shared<Value>(c));
                }
                vec.push_back(std::make_shared<Value>('\0'));
                localEnv.setLocal(n, std::make_shared<Value>(vec));
            } else {
                localEnv.setLocal(n, args[argIndex]);
            }
            argIndex++;
        }
        param = param->tail;
    }
    try { 
        if (def->body) def->body->execute(localEnv); 
    } 
    catch (const ReturnException &r) { return r.val; }
    return Value();
}

static std::string getStringFromValue(const Value &arg) {
    if (arg.isString()) return arg.asString();
    if (arg.isArray()) {
        std::string s = "";
        const auto& vec = arg.asArray();
        for (const auto& val_ptr : vec) {
            if (!val_ptr || (val_ptr->isChar() && val_ptr->asChar() == '\0')) break;
            s += val_ptr->asChar();
        }
        return s;
    }
    throw RuntimeError("Type error: expected string literal or byte array");
}

static int getStringLength(const Value &arg) {
    if (arg.isString()) return static_cast<int>(arg.asString().size());
    if (arg.isArray()) {
        const auto& vec = arg.asArray();
        int len = 0;
        for (const auto& val_ptr : vec) {
            if (!val_ptr || (val_ptr->isChar() && val_ptr->asChar() == '\0')) break;
            len++;
        }
        return len;
    }
    throw RuntimeError("Type error: expected string literal or byte array for strlen");
}

static void copyStringToArray(const Value &dest_val, const Value &src_val) {
    if (!dest_val.isArray()) throw RuntimeError("strcpy target must be a byte array");
    auto& dest_vec = const_cast<Value&>(dest_val).asArray();
    std::string src_str = getStringFromValue(src_val);
    if (dest_vec.empty()) return;
    size_t len = std::min(src_str.length(), dest_vec.size() - 1);
    size_t i = 0;
    for (i = 0; i < len; ++i) {
        if (!dest_vec[i]) dest_vec[i] = std::make_shared<Value>(' ');
        *dest_vec[i] = Value(src_str[i]);
    }
    if (!dest_vec[i]) dest_vec[i] = std::make_shared<Value>('\0');
    *dest_vec[i] = Value('\0');
}

static void concatStringToArray(const Value &dest_val, const Value &src_val) {
    if (!dest_val.isArray()) throw RuntimeError("strcat target must be a byte array");
    auto& dest_vec = const_cast<Value&>(dest_val).asArray();
    if (dest_vec.empty()) return;
    size_t dest_len = 0;
    for (dest_len = 0; dest_len < dest_vec.size(); ++dest_len) {
        if (!dest_vec[dest_len] || (dest_vec[dest_len]->isChar() && dest_vec[dest_len]->asChar() == '\0')) break;
    }
    if (dest_len == dest_vec.size())return;
    std::string src_str = getStringFromValue(src_val);
    size_t i = 0;
    size_t max_copy = dest_vec.size() - dest_len - 1;
    size_t copy_len = std::min(src_str.length(), max_copy);

    for (i = 0; i < copy_len; ++i) {
        if (!dest_vec[dest_len + i]) dest_vec[dest_len + i] = std::make_shared<Value>(' ');
        *dest_vec[dest_len + i] = Value(src_str[i]);
    }
    if (!dest_vec[dest_len + i]) dest_vec[dest_len + i] = std::make_shared<Value>('\0');
    *dest_vec[dest_len + i] = Value('\0');
}

Value callBuiltin(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env) {
    (void)env;

    if (name == "writeInteger") {
        if (args.size() != 1 || !isInteger(args[0])) throw RuntimeError("writeInteger expects a single integer argument");
        std::cout << args[0].asInt();
        return Value();
    }

    if (name == "writeChar") {
        if (args.size() != 1 || !isChar(args[0])) throw RuntimeError("writeChar expects a single byte/char argument");
        std::cout << args[0].asChar();
        return Value();
    }

    if (name == "writeByte") {
        if (args.size() != 1 || !isChar(args[0])) throw RuntimeError("writeByte expects a single byte/char argument");
        std::cout << static_cast<int>(args[0].asChar());
        return Value();
    }

    if (name == "writeString") {
        if (args.size() != 1) throw RuntimeError("writeString expects a single string/array argument");
        std::cout << getStringFromValue(args[0]);
        
        return Value();
    }

    if (name == "readInteger") {
        int i = 0;
        std::cin >> i;
        if (std::cin.fail()) {
             std::cin.clear();
             std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
             return Value(0); 
        }
        return Value(i);
    }

    if (name == "readChar") {
        char c = '\0';
        std::cin.get(c);
        if (std::cin.eof()) c = '\0';
        return Value(c);
    }

    if (name == "readByte") {
        char c = '\0';
        std::cin.get(c);
        if (std::cin.eof()) c = '\0';
        return Value(c);
    }

    if (name == "readString") {
        if (args.size() != 2 || !isInteger(args[0]) || !isArray(args[1])) throw RuntimeError("readString expects (int, byte[]) arguments");
        int n_max_size = args[0].asInt();
        auto& s_array = const_cast<Value&>(args[1]).asArray();
        int buffer_capacity = s_array.size();
        if (buffer_capacity == 0) return Value();
        int max_chars_to_read = std::min(n_max_size - 1, buffer_capacity - 1);
        if (max_chars_to_read < 0) max_chars_to_read = 0;
        int i = 0;
        char c;
        for (i = 0; i < max_chars_to_read; ++i) {
            int next_char = std::cin.peek();
            if (next_char == EOF || next_char == '\n') break;
            std::cin.get(c);
            *s_array[i] = Value(c);
        }
        *s_array[i] = Value('\0');
        if (std::cin.peek() == '\n') std::cin.get();
        return Value();
    }

    if (name == "strlen") {
        if (args.size() != 1) throw RuntimeError("strlen expects one string/array argument");
        return Value(getStringLength(args[0]));
    }

    if (name == "strcmp") {
        if (args.size() != 2) throw RuntimeError("strcmp expects two string/array arguments");
        std::string s1 = getStringFromValue(args[0]);
        std::string s2 = getStringFromValue(args[1]);
        int result = s1.compare(s2);
        return Value(result);
    }

    if (name == "strcpy") {
        if (args.size() != 2) throw RuntimeError("strcpy expects (trg as byte[], src as string/array)");
        if (!args[0].isArray()) throw RuntimeError("strcpy target must be a byte array");
        copyStringToArray(args[0], args[1]);
        return Value();
    }

    if (name == "strcat") {
        if (args.size() != 2) throw RuntimeError("strcat expects (trg as byte[], src as string/array)");
        if (!args[0].isArray()) throw RuntimeError("strcat target must be a byte array");
        concatStringToArray(args[0], args[1]);
        return Value();
    }

    if (name == "extend") {
        if (args.size() != 1 || !isChar(args[0])) throw RuntimeError("extend expects one byte/char argument");
        return Value(static_cast<int>(args[0].asChar()));
    }

    if (name == "shrink") {
        if (args.size() != 1 || !isInteger(args[0])) throw RuntimeError("shrink expects one integer argument");
        return Value(static_cast<char>(args[0].asInt() & 0xFF));
    }

    throw RuntimeError("Unknown built-in function: " + name);
}