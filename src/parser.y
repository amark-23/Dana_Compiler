%{
/* C/C++ Headers */
#include <cstdio>
#include <cstdlib>
#include "lexer.hpp"
/* Forward declarations for helper functions, AST node creation, etc. */
// e.g., void yyerror(const char *s);
extern int yylineno;
extern char **yytext;
%}

/* The parser’s data type. Can store integers, strings, AST pointers, etc. */
%union {
    int ival;         /* For T_number values */
    char *sval;       /* For T_id, T_string, etc. */
    /* Later you can add pointers for AST nodes, e.g. ASTNode* node; */
}

/* Dana’s tokens */
%token T_as        "as"
%token T_begin     "begin"
%token T_break     "break"
%token T_byte      "byte"
%token T_continue  "continue"
%token T_decl      "decl"
%token T_def       "def"
%token T_elif      "elif"
%token T_else      "else"
%token T_end       "end"
%token T_exit      "exit"
%token T_false     "false"
%token T_if        "if"
%token T_is        "is"
%token T_int       "int"
%token T_loop      "loop"
%token T_not       "not"
%token T_or        "or"
%token T_ref       "ref"
%token T_return    "return"
%token T_skip      "skip"
%token T_true      "true"
%token T_var       "var"
%token T_and       "and"
%token T_neq       "!="
%token T_lte       "<="
%token T_gte       ">="
%token T_asgn      ":="
%token T_number
%token T_id
%token T_array
%token T_char
%token T_string

/* For operators like '+', '-', etc., we can keep them as single-char tokens

/* Precedence & Associativity Declarations */
/*might need adjustment */
%left T_or
%left T_and
%left T_not
%left '|' '&'
%left '+' '-'
%left '*' '/' '%'
%left '=' T_neq T_lte T_gte '<' '>'
%right UMINUS  /* For unary minus/plus */

/* The parser’s start symbol */
%start program

%%

/* -------------- Dana Grammar Rules -------------- */

/* Program starts with a function definition */
program
    : func_def
    ;

/* Function definition: Includes header, local definitions, and a block */
func_def
    : T_def header local_defs block
    ;

/* Header includes function name, optional return type, and parameters */
header
    : T_id header_options
    ;

header_options
    : /* Empty */
    | T_is data_type maybe_params
    | maybe_params
    ;

/* Function parameters */
maybe_params
    : /* Empty */
    | ':' fpar_def fpar_def_list
    ;

fpar_def
    : id_list T_as fpar_type
    ;

fpar_def_list
    : /* Empty */
    | ',' fpar_def fpar_def_list
    ;

id_list
    : T_id
    | id_list T_id
    ;

/* Function parameter types */
fpar_type
    : type
    | T_ref data_type
    | data_type '[' ']' dims
    ;

/* The type system now supports arrays explicitly */
type
    : data_type
    | array_type
    ;

array_type
    : data_type dims
    | T_array  /* Match arrays like: x[10], arr[5][5] */
    ;

dims
    : /* Empty */  
    | '[' T_number ']' dims
    ;

/* Data types */
data_type
    : T_int
    | T_byte
    ;


/* Local definitions can be function definitions, declarations, or variables */
local_defs
    : /* Empty */
    | local_defs local_def
    ;

local_def
    : func_def
    | func_decl
    | var_def
    ;

/* Function declaration */
func_decl
    : T_decl header
    ;

/* Variable definition */
var_def
    : T_var id_list T_is type
    ;

/* Code Blocks */
block
    : T_begin stmt_list T_end
    | stmt_list /* Using layout rules */
    ;

/* Statements */
stmt_list
    : /* Empty */
    | stmt_list stmt
    ;

stmt
    : T_skip
    | l_value T_asgn expr
    | proc_call
    | T_exit
    | T_return ':' expr
    | T_if cond ':' block elif_list else_part
    | T_loop loop_id ':' block
    | T_break loop_id_opt
    | T_continue loop_id_opt
    ;

/* elif and else parts */
elif_list
    : /* Empty */
    | elif_list T_elif cond ':' block
    ;

else_part
    : /* Empty */
    | T_else ':' block
    ;

/* Loop identifier */
loop_id
    : /* Empty */
    | T_id
    ;

loop_id_opt
    : /* Empty */
    | ':' T_id
    ;

/* Procedure call */
proc_call
    : T_id proc_call_args
    ;

proc_call_args
    : /* Empty */
    | ':' expr expr_list
    ;

/* Expressions */
expr_list
    : /* Empty */
    | ',' expr expr_list
    ;

/* L-Values */
l_value
    : T_id
    | T_string
    | l_value '[' expr ']'
    ;

/* Expressions */
expr
    : T_number
    | T_char
    | l_value
    | '(' expr ')'
    | func_call
    | '+' expr %prec UMINUS
    | '-' expr %prec UMINUS
    | '!' expr
    | expr '+' expr
    | expr '-' expr
    | expr '*' expr
    | expr '/' expr
    | expr '%' expr
    | expr '&' expr
    | expr '|' expr
    | expr '=' expr
    | expr T_neq expr
    | expr '<' expr
    | expr '>' expr
    | expr T_lte expr
    | expr T_gte expr
    | "true"
    | "false"
    ;

/* Function calls */
func_call
    : T_id '(' call_args ')'
    ;

call_args
    : /* Empty */
    | expr call_arg_list
    ;

call_arg_list
    : /* Empty */
    | ',' expr call_arg_list
    ;

/* Conditions */
cond
    : expr
    | '(' cond ')'
    | T_not cond
    | cond T_and cond
    | cond T_or cond
    | expr '=' expr
    | expr T_neq expr
    | expr '<' expr
    | expr '>' expr
    | expr T_lte expr
    | expr T_gte expr
    ;


%%

/* ------- C/C++ Code Section ------- */

int main() {
    int result = yyparse();
    if (result ==0) printf("Success.\n");
}

void yyerror(const char *msg) {
    fprintf(stderr, "Syntax Error: %s at line %d (Unexpected token: \"%s\")\n", msg, yylineno, (char*) yytext);
    exit(1); /* You can change this to handle multiple errors gracefully */
}