%{
#include <cstdio>
#include <cstdlib>
#include "lexer.hpp"

// Bison declarations
extern int yylex();
extern int yylineno;
void yyerror(const char* msg);
%}

/** ----------------------------------------------------------------
 * Tokens
 * ----------------------------------------------------------------*/
%token T_def "def"
%token T_if "if"
%token T_elif "elif"
%token T_else "else"
%token T_loop "loop"
%token T_begin "begin"
%token T_end "end"
%token T_auto_end
%token T_id
%token T_number
%token T_char
%token T_string
%token T_as "as"
%token T_is "is"
%token T_byte "byte"
%token T_int "int"
%token T_var "var"
%token T_return "return"
%token T_true "true"
%token T_false "false"
%token T_skip "skip"
%token T_break "break"
%token T_continue "continue"
%token T_exit "exit"
%token T_decl "decl"
%token T_not "not"
%token T_or "or"
%token T_and "and"
%token T_ref "ref"
%token T_lte "<="
%token T_gte ">="
%token T_neq "<>"
%token T_asgn ":="

%left T_or
%left T_and
%left '=' T_neq T_lte T_gte '<' '>'
%left '+' '-' '|'
%left '*' '/' '%' '&'
%right '!' T_not
%right UMINUS

%start program

%debug
%defines

%%

/** ----------------------------------------------------------------
 * Grammar Rules
 * ----------------------------------------------------------------*/

/** Program = single func_def */
program
  : func_def
  ;

/** Function definition */
func_def
  : T_def header local_defs block
  ;

/**
 * Header can have:
 *   - just an id
 *   - id is data_type
 *   - id : fpar_def fpar_def_list
 *   - id is data_type : fpar_def fpar_def_list
 */
header
  : T_id
  | T_id T_is data_type
  | T_id ':' fpar_def fpar_def_list
  | T_id T_is data_type ':' fpar_def fpar_def_list
  ;

type    
  : data_type dims
  ;
dims
  : /* empty */
  | dims '[' ']'
  | dims '[' T_number ']' 
  ;

/** data_type -> just 'int' or 'byte' */
data_type
  : T_int
  | T_byte
  ;

/** fpar_type handles array-like syntax:  byte[] or byte[10], etc. */
fpar_type
  : type
  ;

/**
 * fpar_def matches (id1 id2 ...) as fpar_type
 * e.g. (source target auxiliary) as byte[]
 */
fpar_def
  : multi_id_list T_as maybe_ref fpar_type
  ;

maybe_ref
  : T_ref
  |
  ;  

/** multiple fpar_def separated by ',' */
fpar_def_list
  : /* empty */
  | ',' fpar_def fpar_def_list
  ;

/**
 * multi_id_list = 1 or more T_id in a row
 * e.g. source target auxiliary
 */
multi_id_list
  : T_id
  | multi_id_list T_id
  ;

/** local_def can be func_def, func_decl, or var_def */
local_defs
  : /* empty */
  | local_defs local_def
  ;

local_def
  : func_def
  | func_decl
  | var_def
  ;

/** function declaration:  decl header */
func_decl
  : T_decl header
  ;

/** var definition: var (id_list) is data_type */
var_def
  : T_var var_id_list T_is type
  ;

/** var_id_list = 1 or more T_id in a row */
var_id_list
  : T_id
  | var_id_list T_id
  ;

/**
 * block can be:
 *   - begin stmt_list end
 *   - stmt_list T_auto_end
 *   - or just stmt_list (indentation-based w/o final auto_end)
 */
block
  : T_begin stmt_list T_end
  | stmt_list T_auto_end
  | stmt_list
  ;

/** A statement list is one or more statements */
stmt_list
  : stmt
  | stmt_list stmt
  ;

/** Statements: skip, assignment, proc_call, exit, return, if, loop, break, continue */
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
  | func_def //THIS IS NOT ALLOWED
  ;

/** elif/else parts */
elif_list
  : /* empty */
  | elif_list T_elif cond ':' block
  ;

else_part
  : /* empty */
  | T_else ':' block
  ;

/** loop_id is optional */
loop_id
  : /* empty */
  | T_id
  ;

/** loop_id_opt is optional :id */
loop_id_opt
  : /* empty */
  | ':' T_id
  ;

/** procedure call: id [: expr expr_list] */
proc_call
  : T_id proc_call_args
  ;

proc_call_args
  : /* empty */
  | ':' expr expr_list
  ;

expr_list
  : /* empty */
  | ',' expr expr_list
  ;

/** l_value can be id, string, or array-subscripting */
l_value
  : T_id
  | T_string
  | l_value '[' expr ']'
  ;

/** expression rules */
expr
  : T_number
  | T_char
  | l_value
  | '(' expr ')' %prec UMINUS
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
  | T_true
  | T_false
  ;

/** function call: id(...) */
func_call
  : T_id '(' call_args ')'
  ;

call_args
  : /* empty */
  | expr call_arg_list
  ;

call_arg_list
  : /* empty */
  | ',' expr call_arg_list
  ;

/** condition can be expr or combos of cond with and/or */
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

/** ----------------------------------------------------------------
 * C++ Code Section
 * ----------------------------------------------------------------*/
int main() {
  yydebug = 1;  // Turn on parser debugging
  int res = yyparse();
  if (res == 0) {
    printf("Parsing Success.\n");
  } else {
    printf("Parsing Failed.\n");
  }
  return res;
}

void yyerror(const char* msg) {
  fprintf(stderr, "Syntax Error: %s at line %d\n", msg, yylineno);
}
