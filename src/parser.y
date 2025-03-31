%{
#include "ast.hpp"
#include "lexer.hpp"
#include <cstdio>
#include <cstdlib>

// ANSI color codes
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define RESET "\033[0m"

extern int yylineno;
extern char* yytext;
extern unsigned int *indent_stack;  
extern size_t indent_stack_size;
extern size_t level;
extern void stackinit();

%}
 
%token T_and "and"
%token T_as "as"
%token T_begin "begin"
%token T_break "break"
%token T_byte "byte"
%token T_continue "continue"
%token T_decl "decl"
%token T_def 
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

%token<var> T_id  
%token<num> T_number    
%token T_const
%token T_string
%token auto_end

%nonassoc "def" "if" "loop" "break" "continue" "return"
%nonassoc "not" '!'
%nonassoc '=' "<>" '<' '>' "<=" ">="
%left<op> '*' '/' '%' '&'
%left<op> '+' '-'
%left<op> '|'
%left "and"
%left "or"

%union{
  Stmt *stmt;
  Expr *expr;
  Block *blck;
  Func *fnc;
  char var;
  int  num;
  char op;
}

%type<stmt> stmt
%type<expr> expr
%type<blck> stmt_list

%expect 1

%%

program : func_def                                                                              {  }
        ;

func_def  : T_def header local_def_list auto_end                                                {  }
          ;

func_decl : T_decl header                                                                       {  }
          ;

header  : T_id "is" type ':' opt_fpar                                                           {  }
        | T_id "is" type                                                                        {  }
        | T_id ':' opt_fpar                                                                     {  }
        | T_id                                                                                  {  }
        ;

opt_fpar  : id_list "as" ref_data_type                                                          {  }
          | id_list "as" ref_data_type ',' opt_fpar                                             {  }
          | id_list "as" fpar_type                                                              {  }
          | id_list "as" fpar_type ',' opt_fpar                                                 {  }
          ;

fpar_type : "int"                                                                               {  }
          | "byte"                                                                              {  }
          | array_type                                                                          {  }
          ;

ref_data_type : "ref" "int"                                                                     {  }
              | "ref" "byte"                                                                    {  }
              ;

array_type  : "int" '[' ']'                                                                     {  }
            | "byte" '[' ']'                                                                    {  }
            | "int" '[' T_number ']'                                                            {  }
            | "byte" '[' T_number ']'                                                           {  }
            | array_type '[' T_number ']'                                                       {  }
            ;

stmt_list : stmt                                                                                {  }
          | stmt stmt_list                                                                      {  } 
          ;

data_type : "int"                                                                               {  }
          | "byte"                                                                              {  }
          ;

type  : type '[' T_number ']'                                                                   {  }
      | data_type                                                                               {  }
      ;

local_def_list  : "begin" stmt_list "end"                                                       {  }
                | stmt_list                                                                     {  }
                | local_def local_def_list                                                      {  }
                ;

local_def : func_def                                                                            {  }
          | func_decl                                                                           {  }
          | "var" id_list "is" type                                                             {  }
          ;

stmt  : "skip"                                                                                  {  }
      | l_value ":=" expr                                                                       {  }
      | func_call                                                                               {  }      
      | proc_call                                                                               {  }
      | "exit"                                                                                  {  }
      | "return" ':' expr                                                                       {  }
      | if_stmts                                                                                {  }
      | loop                                                                                    {  }
      | "break"                                                                                 {  }
      | "break" ':' T_id                                                                        {  }
      | "continue"                                                                              {  }
      | "continue" ':' T_id                                                                     {  }
      ;

if_stmts  : "if" cond ':' stmt_list auto_end "else" ':' stmt_list auto_end                      {  }
          | "if" cond ':' stmt_list auto_end "elif" cond ':' stmt_list auto_end opt_elif_else   {  }
          | "if" cond ':' stmt_list auto_end                                                    {  }
          ;

loop  : "loop" T_id ':' stmt_list auto_end                                                      {  }
      | "loop" ':' stmt_list auto_end                                                           {  }
      ;

opt_elif_else : /* empty */                                                                     {  }
              | "elif" cond ':' stmt_list auto_end opt_elif_else                                {  }
              | "else" ':' stmt_list auto_end                                                   {  }
              ;

proc_call : T_id                                                                                {  }
          | T_id ':' expr_list                                                                  {  }
          ;

func_call : T_id '('')'                                                                         {  }
          | T_id '(' expr_list ')'                                                              {  }
          ;

l_value : T_id                                                                                  {  }
        | T_string                                                                              {  }
        | l_value '[' expr ']'                                                                  {  }
        ;

expr  : T_number                                                                                {  }
      | T_const                                                                                 {  }
      | l_value                                                                                 {  }
      | func_call                                                                               {  }
      | '(' expr ')'                                                                            {  }
      | '+' expr                                                                                {  }
      | '-' expr                                                                                {  }
      | '!' expr                                                                                {  }
      | expr '+' expr                                                                           {  }
      | expr '-' expr                                                                           {  }
      | expr '*' expr                                                                           {  }
      | expr '/' expr                                                                           {  }
      | expr '%' expr                                                                           {  }
      | expr '&' expr                                                                           {  }
      | expr '|' expr                                                                           {  }
      | "true"                                                                                  {  }
      | "false"                                                                                 {  }
      ;

cond  : expr rel_op expr                                                                        {  }
      | cond "and" cond                                                                         {  }
      | cond "or" cond                                                                          {  }
      | "not" cond                                                                              {  }
      | '(' cond ')'                                                                            {  }
      | expr                                                                                    {  }
      ;

rel_op  : '>'                                                                                   {  }
        | '<'                                                                                   {  }
        | ">="                                                                                  {  }
        | "<="                                                                                  {  }
        | '='                                                                                   {  }
        | "<>"                                                                                  {  }
        ;

id_list : T_id                                                                                  {  }
        | id_list T_id                                                                          {  }
        ;

expr_list : expr                                                                                {  }
          | expr ',' expr_list                                                                  {  }
          ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, RED "%s" RESET " at line " RED "%d" RESET " : " RED "%s\n" RESET, msg, yylineno, yytext);
}

int main() {
    stackinit(); 
    int result = yyparse();
    // printf("\n");
    if (result == 0) printf("Parsing " GREEN "Successful.\n" RESET);
    else printf("Parsing " RED "failed.\n\n" RESET);

    free(indent_stack);
    return result;
}
