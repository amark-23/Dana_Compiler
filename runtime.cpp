#include "ast.hpp"
#include "runtime.hpp"
#include "symbol.hpp"

#include <memory>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>

std::string process_escapes(const std::string& raw_str) {
    std::string result = "";
    for (size_t i = 0; i < raw_str.length(); ++i) {
        if (raw_str[i] == '\\' && i + 1 < raw_str.length()) {
            // Check the character following the backslash
            switch (raw_str[i + 1]) {
                case 'n':  result += '\n'; i++; break; // Newline
                case 't':  result += '\t'; i++; break; // Tab
                case 'r':  result += '\r'; i++; break; // Carriage Return
                case '\\': result += '\\'; i++; break; // Literal Backslash
                case '"':  result += '"'; i++; break;  // Literal Double Quote
                // Add more escape sequences (e.g., \0, \a, \b) as needed
                default:
                    // If it's an unrecognized escape sequence, treat the backslash as a literal character
                    result += raw_str[i]; 
                    break;
            }
        } else {
            result += raw_str[i];
        }
    }
    return result;
}

// -------------------------------
// Helper: create default Value for a typeClass
// -------------------------------
static Value createDefaultValue(typeClass *t) {
    if (!t) return Value(0);
    // dynamic casts for arrayType, refType, basicType
    if (auto *arr = dynamic_cast<arrayType*>(t)) {
        // get size (Const*)
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
        // ref behaves like the base type; create default for base
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
    // fallback
    return Value(0);
}

// -------------------------------
// Id::execute
// -------------------------------
Value Id::execute(RuntimeEnv &env) {
    auto sp = env.lookup(name);
    if (!sp) {
        std::ostringstream ss;
        ss << "Runtime Error at line " << lineno << ": Undefined variable '" << name << "'";
        throw RuntimeError(ss.str());
    }
    // return copy
    return *sp;
}

// -------------------------------
// Const::execute
// -------------------------------
Value Const::execute(RuntimeEnv &env) {
    (void)env;
    return Value(value);
}

// -------------------------------
// paramNode::bindParams
//   - Binds actual args into env. Basic (by-value) binding.
//   - For reference params you need a more advanced strategy; here we
//     support binding by value. See notes below for ref semantics.
// -------------------------------
void paramNode::bindParams(const std::vector<Value> &args, RuntimeEnv &env) {
    // iterate parameters and args in order
    int idx = 0;
    paramNode *p = this;
    while (p && idx < (int)args.size()) {
        if (p->names) {
            for (auto &nm : *(p->names)) {
                if (idx >= (int)args.size()) return;
                // create a shared_ptr copy and setLocal
                env.setLocal(nm, std::make_shared<Value>(args[idx]));
                ++idx;
            }
        }
        p = p->tail;
    }
}

// -------------------------------
// exprNode::execute
// -------------------------------
Value exprNode::execute(RuntimeEnv &env) {
    switch (op) {
        case 'c': { // int constant
            if (!constant) throw RuntimeError("Runtime: missing constant at line " + std::to_string(lineno));
            std::cout << constant->value;
            return Value(constant->value);
        }
        case 'x': { // char constant
            if (!constant) throw RuntimeError("Runtime: missing char constant at line " + std::to_string(lineno));
            std::cout << constant->value;
            return Value(static_cast<char>(constant->value));
        }
        case 'b': { // boolean literal
            std::cout << tfFlag;
            return Value(tfFlag);
        }
        case 'i': { // identifier / lval
            if (!lval) throw RuntimeError("Runtime: identifier expression missing lval at line " + std::to_string(lineno));
            return lval->execute(env);
        }
        case 'f': { // function call
            if (!func || !func->iden) throw RuntimeError("Runtime: invalid function call at line " + std::to_string(lineno));
            return func->execute(env);
        }

        // arithmetic: unary or binary
        case '+': case '-': case '*': case '/': case '%': {
            if (!leftExpr) {
                // unary + / -
                Value rv = rightExpr->execute(env);
                int r = rv.asInt();
                if (op == '+') return Value(+r);
                return Value(-r);
            } else {
                int l = leftExpr->execute(env).asInt();
                int r = rightExpr->execute(env).asInt();
                switch (op) {
                    case '+': std::cout << l + r; return Value(l + r);
                    case '-': std::cout << l - r; return Value(l - r);
                    case '*': std::cout << l * r; return Value(l * r);
                    case '/':
                        if (r == 0) throw RuntimeError("Runtime: division by zero at line " + std::to_string(lineno));
                        std::cout << l / r;
                        return Value(l / r);
                    case '%':
                        if (r == 0) throw RuntimeError("Runtime: modulo by zero at line " + std::to_string(lineno));
                        std::cout << l % r;
                        return Value(l % r);
                }
            }
            break;
        }

        // comparisons
        case '=': case '<': case '>': case 'g': case 'l': case 'd': {
            Value lv = leftExpr ? leftExpr->execute(env) : Value(0);
            Value rv = rightExpr ? rightExpr->execute(env) : Value(0);

            // int
            if (lv.isInt() && rv.isInt()) {
                int li = lv.asInt();
                int ri = rv.asInt();
                switch (op) {
                    case '=': std::cout << (li == ri); return Value(li == ri);
                    case '<': std::cout << (li < ri); return Value(li < ri);
                    case '>': std::cout << (li > ri); return Value(li > ri);
                    case 'g': std::cout << (li >= ri); return Value(li >= ri);
                    case 'l': std::cout << (li <= ri); return Value(li <= ri);
                    case 'd': std::cout << (li != ri); return Value(li != ri);
                }
            }

            // char
            if (lv.isChar() && rv.isChar()) {
                char lc = lv.asChar();
                char rc = rv.asChar();
                switch (op) {
                    case '=': std::cout << (lc == rc); return Value(lc == rc);
                    case '<': std::cout << (lc < rc); return Value(lc < rc);
                    case '>': std::cout << (lc > rc); return Value(lc > rc);
                    case 'g': std::cout << (lc >= rc); return Value(lc >= rc);
                    case 'l': std::cout << (lc <= rc); return Value(lc <= rc);
                    case 'd': std::cout << (lc != rc); return Value(lc != rc);
                }
            }

            // bool
            if (lv.isBool() && rv.isBool()) {
                bool lb = lv.asBool();
                bool rb = rv.asBool();
                switch (op) {
                    case '=': std::cout << (lb == rb); return Value(lb == rb);
                    case 'd': std::cout << (lb != rb); return Value(lb != rb);
                    case '<': std::cout << (lb < rb); return Value(static_cast<int>(lb) < static_cast<int>(rb));
                    case '>': std::cout << (lb > rb); return Value(static_cast<int>(lb) > static_cast<int>(rb));
                    case 'g': std::cout << (lb >= rb); return Value(static_cast<int>(lb) >= static_cast<int>(rb));
                    case 'l': std::cout << (lb <= rb); return Value(static_cast<int>(lb) <= static_cast<int>(rb));
                }
            }

            // string
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

        // logical
        case 'a': { // and
            bool l = leftExpr->execute(env).asBool();
            bool r = rightExpr->execute(env).asBool();
            return Value(l && r);
        }
        case 'o': { // or
            bool l = leftExpr->execute(env).asBool();
            bool r = rightExpr->execute(env).asBool();
            return Value(l || r);
        }
        case 'n': { // not (unary)
            bool r = rightExpr->execute(env).asBool();
            return Value(!r);
        }

        // bitwise ops (on ints)
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

// -------------------------------
// fcallNode::execute
// -------------------------------
Value fcallNode::execute(RuntimeEnv &env) {
    if (!iden) throw RuntimeError("Runtime: invalid function identifier at line " + std::to_string(lineno));
    std::string fname = iden->name;

    std::vector<Value> evaluatedArgs;
    if (args) {
        // args are evaluated in the order stored in vector
        for (auto *e : *args) {
            evaluatedArgs.push_back(e->execute(env));
        }
    }

    // Builtin
    if (isBuiltin(fname)) {
        return callBuiltin(fname, evaluatedArgs, env);
    }

    // User function
    return callUserFunction(fname, evaluatedArgs, env);
}

// -------------------------------
// lvalNode::execute  (read value)
// -------------------------------
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

    // if no indices => return copy of variable
    if (!ind || ind->empty()) {
        return *sp;
    }

    // traverse indices
    Value current = *sp;
    for (auto *idxExpr : *ind) {
        int idx = idxExpr->execute(env).asInt();
        if (!current.isArray()) {
            throw RuntimeError("Runtime: variable '" + ident->name + "' is not an array at line " + std::to_string(lineno));
        }
        const auto &vec = current.asArray();
        if (idx < 0 || idx >= (int)vec.size()) {
            throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
        }
        // dereference shared_ptr<Value>
        current = *vec[idx];
    }
    return current;
}

// -------------------------------
// lvalNode::assign  (assign to lvalue)
//   - Handles both whole-variable and indexed-element assignments
// -------------------------------
void lvalNode::assign(RuntimeEnv &env, const Value &val) {
    if (!ident) throw RuntimeError("Runtime: missing identifier on assign at line " + std::to_string(lineno));
    auto sp = env.lookup(ident->name);
    if (!sp) {
        std::ostringstream ss;
        ss << "Runtime Error at line " << lineno << ": Undeclared variable '" << ident->name << "'";
        throw RuntimeError(ss.str());
    }

    // No indices => replace the variable's value
    if (!ind || ind->empty()) {
        // copy into existing shared_ptr
        *sp = val;
        return;
    }

    // Walk down to the container and assign element
    std::shared_ptr<Value> container = sp;        // pointer to root value
    Value current = *container;

    // We have to walk until the parent of the final index so we can assign into the element slot
    int n = (int)ind->size();
    for (int i = 0; i < n - 1; ++i) {
        int idx = ind->at(i)->execute(env).asInt();
        if (!current.isArray()) throw RuntimeError("Runtime: not an array while assigning at line " + std::to_string(lineno));
        auto &vec = current.asArray();
        if (idx < 0 || idx >= (int)vec.size()) throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
        // step into this element
        container = vec[idx];
        current = *container;
    }
    // Now container holds shared_ptr<Value> to the array at depth n-1
    // final index:
    int finalIdx = ind->back()->execute(env).asInt();
    if (!current.isArray()) throw RuntimeError("Runtime: not an array while assigning at line " + std::to_string(lineno));
    auto &finalVec = (*container).asArray();
    if (finalIdx < 0 || finalIdx >= (int)finalVec.size()) throw RuntimeError("Runtime: array index out of bounds at line " + std::to_string(lineno));
    // assign into the shared_ptr slot (copy)
    if (!finalVec[finalIdx]) finalVec[finalIdx] = std::make_shared<Value>(val);
    else *finalVec[finalIdx] = val;
}

// -------------------------------
// ifNode::execute
// -------------------------------
void ifNode::execute(RuntimeEnv &env) {
    if (cond) { 
        bool condv = cond->execute(env).asBool();
        if (condv) {
            // create a new dynamic scope for the statement block
            RuntimeEnv inner(&env);
            if (stmt) {
                stmt->execute(inner);
            }
            return;
        }
    } else {
        // else branch condition is absent -> acts as unconditional?
        // We'll treat null cond as true for 'else' node's stmt invocation
        RuntimeEnv inner(&env);
        if (stmt) stmt->execute(inner);
        return;
    }
    // tail chain (elif/else)
    if (tail) tail->execute(env);
}

// -------------------------------
// stmtNode::execute
// -------------------------------
void stmtNode::execute(RuntimeEnv &env) {
    if (stmtType == "vardecl") {
        if (!varType || !varNames) throw RuntimeError("Malformed declaration at line " + std::to_string(lineno));
        for (auto &n : *varNames) {
            // create default value based on varType
            Value def = createDefaultValue(varType);
            env.setLocal(n, std::make_shared<Value>(def));
        }
    }
    else if (stmtType == "decl") {
        // function declaration: nothing to execute at runtime
    }
    else if (stmtType == "asgn") {
        if (!lval || !exp) throw RuntimeError("Invalid assignment at line " + std::to_string(lineno));
        Value rv = exp->execute(env);
        lval->assign(env, rv);
    }
    else if (stmtType == "pc") {
        if (!exp) throw RuntimeError("Procedure call missing expression at line " + std::to_string(lineno));
        // execute the call and ignore the returned value
        exp->execute(env);
    }
    else if (stmtType == "exit") {
        // exit: treat as return from current function with no value (0)
        throw ReturnException(Value(0));
    }
    else if (stmtType == "return") {
        Value rv = Value(0);
        if (exp) rv = exp->execute(env);
        throw ReturnException(rv);
    }
    else if (stmtType == "if") {
        if (!ifnode) throw RuntimeError("Malformed if at line " + std::to_string(lineno));
        // execute ifnode (which will create an inner scope)
        ifnode->execute(env);
    }
    else if (stmtType == "loop") {
        // stmtBody is the body list (chain via tail)
        try {
            while (true) {
                // Each iteration we create an inner env to mimic block scoping in loop body
                RuntimeEnv inner(&env);
                stmtNode *cur = stmtBody;
                while (cur) {
                    cur->execute(inner);
                    cur = cur->stmtTail;
                }
            }
        } catch (const BreakException &) {
            // break: exit loop normally
        } catch (const ContinueException &) {
            // continue: continue outer while loop (we simply loop again)
            // But with this structure we will not catch continue here; handled above
        }
    }
    else if (stmtType == "break") {
        // ensure we're inside a loop? We cannot easily check here (we could use SymbolTable loopDepth)
        throw BreakException();
    }
    else if (stmtType == "continue") {
        throw ContinueException();
    }
    else if (stmtType == "def") {
        // nested function def: nothing at runtime (function registered in symbol table already)
        // Some implementations may register a runtime closure here; we skip.
    }

    // execute tail chain
    if (stmtTail) stmtTail->execute(env);
}

// -------------------------------
// fdefNode::execute
//   When a function definition node appears at runtime we do nothing.
//   Actual function calls are handled by callUserFunction (which uses symbol table to
//   find the function body and will create a RuntimeEnv, bind params and execute the body).
// -------------------------------
Value fdefNode::execute(RuntimeEnv &env) {
    // Register the function (in case of recursive or future calls)
    env.registerFunction(head->iden->name, this);
    
    // Create a local scope for main
    RuntimeEnv localEnv(&env);

    try {
        if (body)
            body->execute(localEnv);
    } catch (const ReturnException &r) {
        return r.val; // handle 'return' in main if present
    }

    // For any non-main function definitions, do not execute immediately
    return Value();
}

// Forward declare Value helpers if not already included:
static bool isInteger(const Value &v) {
    return v.isInt();
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
        "strcmp", "strcpy", "strlen"
    };
    return std::find(builtins.begin(), builtins.end(), name) != builtins.end();
}

/**
 * Execute a built-in function.
 */
Value callBuiltin(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env) {
    if (name == "writeString") {
        if (args.size() != 1 || !isString(args[0])) {
            throw RuntimeError("writeString expects a single string argument");
        }
        std::cout << args[0].asString();
        return Value(); // void
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
            throw RuntimeError("writeInteger expects a single integer argument");
        }
        std::cout << args[0].asChar();
        return Value();
    }

    if (name == "writeByte") {
        if (args.size() != 1 || !isChar(args[0])) {
            throw RuntimeError("writeInteger expects a single integer argument");
        }
        std::cout << args[0].asInt();
        return Value();
    }

    if (name == "strcmp") {
        if (args.size() != 2 || !isString(args[0]) || !isString(args[1])) {
            throw RuntimeError("strcmp expects two strings");
        }
        // int result = std::get<std::string>(args[0]).compare(std::get<std::string>(args[1]));
        // return result;
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

    throw RuntimeError("Unknown built-in function: " + name);
}

/**
 * Call a user-defined function (lookup in symbol table or runtime environment)
 */
Value callUserFunction(const std::string &name, const std::vector<Value> &args, RuntimeEnv &env) {
    // headerNode *hdr = env.getFunctionHeader(name);
    // if (!hdr) {
    //     throw RuntimeError("Undefined function: " + name);
    // }

    // // Create a new local scope for the call
    // RuntimeEnv localEnv(&env);

    // // Bind parameters
    // paramNode *param = hdr->params;
    // size_t argIndex = 0;
    // while (param) {
    //     for (const auto &n : *param->names) {
    //         if (argIndex >= args.size()) {
    //             throw RuntimeError("Too few arguments to " + name);
    //         }
    //         localEnv.setLocal(n, args[argIndex++]);
    //     }
    //     param = param->tail;
    // }

    // // Execute body
    // try {
    //     hdr->iden->execute(localEnv);  // if header has initialization or pre-body code
    //     if (fdefNode *def = env.getFunctionDef(name)) {
    //         def->body->execute(localEnv);
    //     }
    // } catch (const ReturnException &r) {
    //     return r.val;
    // }

    return Value(); // void if no return
}