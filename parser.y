%{
#include "ast.hpp"
#include "lexer.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <stack>

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define RESET "\033[0m"

extern int yylineno;
extern char* yytext;
extern unsigned int *indent_stack;  
extern size_t indent_stack_size;
extern size_t level;
extern void stackinit();

std::stack<fdefNode*> fNames;
std::vector<exprNode*> *lastArg;

fdefNode *startFunc;

%}

%union{
      fdefNode *func;
      exprNode *expr;
      stmtNode *stmt;
      paramNode *param;
      headerNode *header;
      lvalNode *lval;
      fcallNode *funcCall;
      ifNode *ifStmt;
      typeClass *types;
      std::vector<std::string> *idList;

      int constval;
      char *idstr;
}

%token T_and "and"
%token T_as "as"
%token T_begin "begin"
%token T_break "break"
%token T_byte "byte"
%token T_continue "continue"
%token T_decl "decl"
%token T_def "def"
%token T_elif "elif"
%token T_else "else"
%token T_end "end"
%token T_exit "exit"
%token T_false "false"
%token T_if "if"
%token T_is "is"
%token T_int "int"
%token T_loop "loop"
%token T_not "not"
%token T_or "or"
%token T_ref "ref"
%token T_return "return"
%token T_skip "skip"
%token T_true "true"
%token T_var "var"
%token T_asgn ":="
%token T_greq ">=" 
%token T_leq "<="
%token T_neq "<>"

%nonassoc "def" "if" "loop" "break" "continue" "return"
%nonassoc "not" '!'
%nonassoc '=' "<>" '<' '>' "<=" ">="
%left '*' '/' '%' '&'
%left '+' '-' '|'
%left "and"
%left "or"

%type<func> program func_def func_decl
%type<stmt> stmt stmt_list local_def local_def_list loop
%type<expr> expr cond expr_list
%type<funcCall> func_call proc_call
%type<ifStmt> if_stmts opt_elif_else
%type<header> header
%type<param> opt_fpar
%type<types> fpar_type ref_data_type array_type type data_type
%type<idList> id_list
%type<lval> l_value

%token<idstr> T_id T_string
%token<constval> T_num_const T_char_const
%token auto_end

%expect 1

%%

program
      : func_def                                                                                      { std::cout << "AST:\n" << *($1) << std::endl; $$ = $1; startFunc = $1; }
      ;

func_def
      : T_def header {fNames.push(new fdefNode($2, NULL)); } local_def_list auto_end                  { $$ = new fdefNode($2, $4); fNames.pop(); }
      ;

func_decl
      : T_decl header                                                                                 { $$ = new fdefNode($2, NULL); }
      ;

header
      : T_id "is" type ':' opt_fpar                                                                   { $$ = new headerNode($3, $5, new Id($1)); }
      | T_id "is" type                                                                                { $$ = new headerNode($3, NULL, new Id($1)); }
      | T_id ':' opt_fpar                                                                             { $$ = new headerNode(new basicType(TYPE_VOID), $3, new Id($1)); }
      | T_id                                                                                          { $$ = new headerNode(new basicType(TYPE_VOID), NULL, new Id($1)); }
      ;

opt_fpar
      : id_list "as" ref_data_type                                                                    { $$ = new paramNode($1, $3, NULL); $$->ref = true; }
      | id_list "as" fpar_type                                                                        { $$ = new paramNode($1, $3, NULL); $$->ref = false; }
      | id_list "as" ref_data_type ',' opt_fpar                                                       { $$ = new paramNode($1, $3, $5); $$->ref = true; }
      | id_list "as" fpar_type ',' opt_fpar                                                           { $$ = new paramNode($1, $3, $5); $$->ref = false; }
      ;

fpar_type
      : "int"                                                                                         { $$ = new basicType(TYPE_INT); }
      | "byte"                                                                                        { $$ = new basicType(TYPE_CHAR); }
      | array_type                                                                                    { $$ = $1; }
      ;

ref_data_type
      : T_ref "int"                                                                                   { $$ = new basicType(TYPE_INT); }
      | T_ref "byte"                                                                                  { $$ = new basicType(TYPE_CHAR); }
      ;

array_type
      : "int" '[' ']'                                                                                 { $$ = new arrayType(new basicType(TYPE_INT), NULL); }
      | "byte" '[' ']'                                                                                { $$ = new arrayType(new basicType(TYPE_CHAR), NULL); }
      | "int" '[' T_num_const ']'                                                                     { $$ = new arrayType(new basicType(TYPE_INT), new Const($3)); }
      | "byte" '[' T_num_const ']'                                                                    { $$ = new arrayType(new basicType(TYPE_CHAR), new Const($3)); }
      | array_type '[' T_num_const ']'                                                                { $$ = new arrayType($1, new Const($3)); }
      ; 

stmt_list
      : stmt                                                                                          { $$ = $1; }
      | stmt stmt_list                                                                                { $1->tail = $2; $$ = $1; } 
      ;

type
      : type '[' T_num_const ']'                                                                      { $$ = new arrayType($1, new Const($3)); }
      | data_type                                                                                     { $$ = $1; }
      ;

data_type
      : "int"                                                                                         { $$ = new basicType(TYPE_INT); }
      | "byte"                                                                                        { $$ = new basicType(TYPE_CHAR); }
      ;

local_def_list
      : T_begin stmt_list T_end                                                                       { $$ = $2; }
      | stmt_list                                                                                     { $$ = $1; }
      | local_def local_def_list                                                                      { $$ = $1; $1->tail = $2; }
      ;

local_def
      : func_def                                                                                      { $$ = new stmtNode("def", NULL, NULL, NULL); $$->funcDef = $1; }
      | func_decl                                                                                     { $$ = new stmtNode("decl", NULL, NULL, NULL); $$->funcDef = $1; }
      | "var" id_list "is" type                                                                       { $$ = new stmtNode("vardecl", NULL, NULL, NULL); $$->varNames = $2; $$->varType = $4; }
      ;

stmt
      : "skip"                                                                                        { $$ = new stmtNode("skip", NULL, NULL, NULL); }
      | l_value ":=" expr                                                                             { $$ = new stmtNode("asgn", NULL, NULL, NULL); $$->lval = $1; $$->exp = $3; }
      | proc_call                                                                                     { $$ = new stmtNode("pc", NULL, NULL, NULL); $$->exp = new exprNode('f',NULL,0,NULL,NULL, 0); $$->exp->func = $1; }
      | "exit"                                                                                        { $$ = new stmtNode("exit", NULL, NULL, NULL); $$->funcDef = fNames.top(); }
      | "return" ':' expr                                                                             { $$ = new stmtNode("return", NULL, NULL, NULL); $$->exp = $3; $$->funcDef = fNames.top(); }
      | if_stmts                                                                                      { $$ = new stmtNode("if", NULL, NULL, NULL); $$->ifnode = $1; }
      | loop                                                                                          { $$ = $1; }
      | "break"                                                                                       { $$ = new stmtNode("break", NULL, NULL, NULL); }
      | "break" ':' T_id                                                                              { $$ = new stmtNode("break", NULL, NULL, new Id($3)); }
      | "continue"                                                                                    { $$ = new stmtNode("continue", NULL, NULL, NULL); }
      | "continue" ':' T_id                                                                           { $$ = new stmtNode("continue", NULL, NULL, new Id($3)); }
      ;

if_stmts
      : "if" cond ':' local_def_list auto_end "else" ':' local_def_list auto_end                      { $$ = new ifNode($2, $4); auto elseNode = new ifNode(NULL, $8); $$->tail = elseNode; elseNode->tail = NULL; }
      | "if" cond ':' local_def_list auto_end "elif" cond ':' local_def_list auto_end opt_elif_else   { $$ = new ifNode($2, $4); auto elseNode = new ifNode($7, $9); $$->tail = elseNode; elseNode->tail = $11; }
      | "if" cond ':' local_def_list auto_end                                                         { $$ = new ifNode($2, $4); }
      ;

opt_elif_else
      : /* empty */                                                                                   { $$ = NULL; }
      | "elif" cond ':' local_def_list auto_end opt_elif_else                                         { $$ = new ifNode($2, $4); $$->tail = $6; }
      | "else" ':' local_def_list auto_end                                                            { $$ = new ifNode(NULL, $3); }
      ;

loop
      : "loop" T_id ':' local_def_list auto_end                                                       { $$ = new stmtNode("loop", $4, NULL, new Id($2)); }
      | "loop" ':' local_def_list auto_end                                                            { $$ = new stmtNode("loop", $3, NULL, NULL); }
      ;

proc_call
      : T_id                                                                                          { $$ = new fcallNode(new Id($1)); $$->args = NULL; }
      | T_id ':' expr_list                                                                            { $$ = new fcallNode(new Id($1)); $$->args = lastArg; lastArg = new std::vector<exprNode*>(); }
      ;

func_call
      : T_id '('')'                                                                                   { $$ = new fcallNode(new Id($1)); $$->args = NULL; }
      | T_id '(' expr_list ')'                                                                        { $$ = new fcallNode(new Id($1)); $$->args = lastArg; lastArg = new std::vector<exprNode*>(); }
      ;

l_value
      : T_id                                                                                          { $$ = new lvalNode(false, new Id($1)); }
      | T_string                                                                                      { $$ = new lvalNode(true, new Id($1)); }
      | l_value '[' expr ']'                                                                          { $1->ind->push_back($3); $$ = $1; }
      ;

expr
      : T_num_const                                                                                   { $$ = new exprNode('c', NULL, new Const($1), NULL, NULL, 0); }
      | T_char_const                                                                                  { $$ = new exprNode('x', NULL, new Const($1), NULL, NULL, 0); }
      | l_value                                                                                       { $$ = new exprNode('i', $1, NULL, NULL, NULL, 0); }
      | func_call                                                                                     { $$ = new exprNode('f', NULL, NULL, NULL, NULL, 0); $$->func = $1; }
      | '(' expr ')'                                                                                  { $$ = $2; }
      | '+' expr                                                                                      { $$ = new exprNode('+', NULL, NULL, NULL, $2, 0); }
      | '-' expr                                                                                      { $$ = new exprNode('-', NULL, NULL, NULL, $2, 0); }
      | '!' expr                                                                                      { $$ = new exprNode('!', NULL, NULL, NULL, $2, 0); }
      | expr '+' expr                                                                                 { $$ = new exprNode('+', NULL, NULL, $1, $3, 0); }
      | expr '-' expr                                                                                 { $$ = new exprNode('-', NULL, NULL, $1, $3, 0); }
      | expr '*' expr                                                                                 { $$ = new exprNode('*', NULL, NULL, $1, $3, 0); }
      | expr '/' expr                                                                                 { $$ = new exprNode('/', NULL, NULL, $1, $3, 0); }
      | expr '%' expr                                                                                 { $$ = new exprNode('%', NULL, NULL, $1, $3, 0); }
      | expr '&' expr                                                                                 { $$ = new exprNode('&', NULL, NULL, $1, $3, 0); }
      | expr '|' expr                                                                                 { $$ = new exprNode('|', NULL, NULL, $1, $3, 0); }
      | "true"                                                                                        { $$ = new exprNode('b', NULL, NULL, NULL, NULL, 1); }
      | "false"                                                                                       { $$ = new exprNode('b', NULL, NULL, NULL, NULL, 0); }
      ;

cond
      : expr '>' expr                                                                                 { $$ = new exprNode('>', NULL, NULL, $1, $3, 0); }
      | expr '<' expr                                                                                 { $$ = new exprNode('<', NULL, NULL, $1, $3, 0); }
      | expr T_greq expr                                                                              { $$ = new exprNode('g', NULL, NULL, $1, $3, 0); }
      | expr T_leq expr                                                                               { $$ = new exprNode('l', NULL, NULL, $1, $3, 0); }
      | expr '=' expr                                                                                 { $$ = new exprNode('=', NULL, NULL, $1, $3, 0); }
      | expr T_neq expr                                                                               { $$ = new exprNode('d', NULL, NULL, $1, $3, 0); }
      | cond "and" cond                                                                               { $$ = new exprNode('a', NULL, NULL, $1, $3, 0); }
      | cond "or" cond                                                                                { $$ = new exprNode('o', NULL, NULL, $1, $3, 0); }
      | "not" cond                                                                                    { $$ = new exprNode('n', NULL, NULL, NULL, $2, 0); }
      | '(' cond ')'                                                                                  { $$ = $2; }
      | expr                                                                                          { $$ = $1; }
      ;

id_list
      : T_id                                                                                          { $$ = new std::vector<std::string>(); $$->push_back($1); }
      | id_list T_id                                                                                  { $1->push_back($2); $$ = $1; }
      ;

expr_list
      : expr                                                                                          { lastArg->insert(lastArg->begin(), $1); }
      | expr ',' expr_list                                                                            { lastArg->insert(lastArg->begin(), $1); }
      ;

%%

void yyerror(const char *msg) {
    extern int yylineno;
    extern char *yytext;

    const char *token = (yytext && strlen(yytext) > 0) ? yytext : nullptr;

    if (token) {
        if (strcmp(token, "elif") == 0) {
            fprintf(stderr, RED "Syntax Error" RESET " at line %d: 'elif' used without a preceding 'if'.\n", yylineno);
            return;
        }
        if (strcmp(token, "else") == 0) {
            fprintf(stderr, RED "Syntax Error" RESET " at line %d: 'else' used without a preceding 'if'.\n", yylineno);
            return;
        }
        if (strcmp(token, "end") == 0) {
            fprintf(stderr, RED "Syntax Error" RESET " at line %d: unexpected 'end' -- there is no matching 'begin' or block to close.\n", yylineno);
            return;
        }
        if (strcmp(token, "begin") == 0) {
            fprintf(stderr, RED "Syntax Error" RESET " at line %d: 'begin' appears here (possible misplaced block or incorrect indentation).\n", yylineno);
            return;
        }
        if (strcmp(token, "def") == 0) {
            fprintf(stderr, RED "Syntax Error" RESET " at line %d: misplaced or malformed 'def'. Check function header syntax and indentation.\n", yylineno);
            return;
        }

        if (isalpha((unsigned char)token[0]) || token[0] == '_') {
            fprintf(stderr, RED "Syntax Error" RESET " at line %d: unexpected token '%s' — perhaps missing ':' after a function/proc name or wrong statement syntax.\n", yylineno, token);
            return;
        }

        fprintf(stderr, RED "Syntax Error" RESET " at line %d: unexpected token '%s'.\n", yylineno, token);
        return;
    }

    if (feof(stdin)) {
        fprintf(stderr, RED "Syntax Error" RESET " at end of file (line %d): unexpected end of input — likely a missing 'end', 'else', or ':' or an unmatched block.\n", yylineno);
    } else {
        fprintf(stderr, RED "Syntax Error" RESET " at line %d: unexpected end of input — possible missing 'end' or unmatched block.\n", yylineno);
    }
}

int main() {
      stackinit(); 
      SymbolTable st;
      startFunc = NULL;
      fNames = std::stack<fdefNode*>();
      lastArg = new std::vector<exprNode*>();

      submitBuiltInFunctions(st);
      int result = yyparse();

      try {
            if (result == 0 && startFunc != NULL) {
                  startFunc->semanticCheck(st);
                  std::cout << GREEN "No semantic errors found." RESET "\n";
            }
      } catch (const SemanticError &e) {
            fprintf(stderr, RED "Error at line %d:" RESET " %s\n" RESET, e.line, e.what());
            result = 1;
      }
      free(indent_stack);
      return result;
}
