%{
#include "lexer.hpp"
#include "parser.hpp"
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T_eof       0
#define T_INDENT    1001
#define T_DEDENT    1002

int comment_depth = 0; /* nested comment handling */

unsigned int indent_stack[72] = {0}; /* Max indentation depth, initialized */
unsigned int level = 0;
unsigned int indent = 0;
int pending_dedents = 0;

int calculate_dedents(int new_indent);
unsigned int count_leading_spaces(char* line);
%}

%option noyywrap yylineno
%x COMMENT 

%%

"as"        { return T_as; }
"begin"     { return T_begin; }
"break"     { return T_break; }
"byte"      { return T_byte; }
"continue"  { return T_continue; }
"decl"      { return T_decl; }
"def"       { return T_def; }
"elif"      { return T_elif; }
"else"      { return T_else; }
"end"       { return T_end; }
"exit"      { return T_exit; }
"false"     { return T_false; }
"if"        { return T_if; }
"is"        { return T_is; }
"int"       { return T_int; }
"loop"      { return T_loop; }
"not"       { return T_not; }
"or"        { return T_or; }
"ref"       { return T_ref; }
"return"    { return T_return; }
"skip"      { return T_skip; }
"true"      { return T_true; }
"var"       { return T_var; }
"and"       { return T_and; }

"!="        { return T_neq; }
"<="        { return T_lte; }
">="        { return T_gte; }
":="        { return T_asgn; }

"#".*           { /* Ignore single-line comments */ }

"(*"          { BEGIN(COMMENT); comment_depth = 1; }
<COMMENT>"(*" { comment_depth++; }
<COMMENT>")*" { if (--comment_depth == 0) BEGIN(INITIAL);} 
<COMMENT>.    {}
<COMMENT>\n   {}
<COMMENT><<EOF>> {
    fprintf(stderr, "Lexer Error: Unclosed comment (missing '*)') at line %d\n", yylineno);
    exit(1);
}

\r    {}
[ \t] {}

^[ ]*\n {
    while (pending_dedents > 0) {
        pending_dedents--;
        return T_DEDENT;
    }
}

^[ ]* {
    int new_indent = count_leading_spaces(yytext);

    while (pending_dedents > 0) {
        pending_dedents--;
        return T_DEDENT;
    }

    if (new_indent > indent_stack[level]) {
        indent_stack[++level] = new_indent;
        return T_INDENT;
    }

    int dedents = calculate_dedents(new_indent);
    if (dedents > 0) {
        pending_dedents = dedents - 1;
        return T_DEDENT;
    }
}

":"            { return ':'; } 

[\+\-\*\/\%\=\&\!\<\>\(\)\[\]\,]  { return yytext[0]; }
[0-9]+                  { return T_number; }
[a-zA-Z][a-zA-Z0-9_]*   { return T_id; }

"\""(\\[ntr0'"\\]|\\x[0-9a-fA-F]{2}|[^"\\\n])*"\""  { return T_string; }
"'"([^'\\]|\\[ntr0\'\"\\]|\\x[0-9a-fA-F]{2})"'" { return T_char; }

[ \t\r]+              { /* Ignore spaces, tabs, and carriage returns */ }
. {
    fprintf(stderr, "Lexer Error: Illegal character '%c' (ASCII: %d) at line %d\n", yytext[0], yytext[0], yylineno);
    exit(1);
}

%%

unsigned int count_leading_spaces(char* line) {
    unsigned int count = 0;
    while (*line == ' ')
        count++, line++;
    return count;
}

int calculate_dedents(int new_indent) {
    int dedents = 0;
    while (level > 0 && new_indent < indent_stack[level]) {
        level--;
        dedents++;
    }
    return dedents;
}

int main() {
    int tok;
    while ((tok = yylex())) {
        printf("Token: %d, Lexeme: %s\n", tok, yytext);
    }
    return 0;
}
