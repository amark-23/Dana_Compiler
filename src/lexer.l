%{
#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <cstdio>
#include <cstring>

#define T_eof 0
#define RED "\033[1;31m"
#define RESET "\033[0m"

int comment_depth = 0; //nested comment handling

int leader = 0; // def, if, elif, else, loop define blocks

int current_ind_level = 0;
unsigned int *indent_stack = NULL;
size_t indent_stack_size = 0;
size_t level = 0;
unsigned int current_indent = 0;

unsigned int whitespace_cntr(char* line) { //Counts the number of leading whitespace characters in a given line.
    unsigned int count = 0;
    while (*line == ' ' || *line == '\t') count++, line++;
    return count;
}

int process_indent(char* line) {
    current_indent = whitespace_cntr(line);
    if (*line == '\n' || *line == '\r' || *line == '#' || *line == '\0') return 0;
    int dedents = 0;
    // Dynamically resize 
    if (level + 1 >= indent_stack_size) {
        indent_stack_size *= 2;  // Double the size
        indent_stack = (unsigned int*)realloc(indent_stack, indent_stack_size * sizeof(unsigned int));
        if (!indent_stack) {
            fprintf(stderr, "Error: Memory reallocation failed for indentation stack\n");
            exit(1);
        }
    }
    // Handle dedent
    while (level > 0 && current_indent <= indent_stack[level]) {
        --level;
        dedents++;
    }
    return dedents;
}
%}

L [A-Za-z]
D [0-9]

%option noyywrap yylineno noinput
%x COMMENT

%%

"def"  { leader = 1; indent_stack[++level] = current_indent; return T_def; } 
"elif" { leader = 1; indent_stack[++level] = current_indent; return T_elif; } 
"else" { leader = 1; indent_stack[++level] = current_indent; return T_else; } 
"if"   { leader = 1; indent_stack[++level] = current_indent; return T_if; }
"loop" { leader = 1; indent_stack[++level] = current_indent; return T_loop; } 

"and"      { return T_and; } 
"as"       { return T_as; } 
"begin"    { return T_begin; } 
"break"    { return T_break; } 
"byte"     { return T_byte; } 
"continue" { return T_continue; } 
"decl"     { return T_decl; }
"end"      { return T_end; } 
"exit"     { return T_exit; } 
"false"    { return T_false; } 
"is"       { return T_is; } 
"int"      { return T_int; } 
"not"      { return T_not; } 
"or"       { return T_or; } 
"ref"      { return T_ref; } 
"return"   { return T_return; }
"skip"     { return T_skip; } 
"true"     { return T_true; } 
"var"      { return T_var; }
":=" 	   { return T_asgn; }
">="       { return T_greq; }
"<="       { return T_leq; }
"<>"       { return T_neq; }

[\(\)\[\]\,\+\-\*\/\%\!\&\|\=\<\>\:]                            { yylval.op = yytext[0]; return yytext[0]; }
{L}({L}|{D}|_)*                                                 { yylval.var = yytext[0]; return T_id; }
{D}+                                                            { yylval.num = atoi(yytext); return T_number; }
\'([^\'\"\\]|\\[ntr0\\\'\"\\]|\\x[0-9a-fA-F]{2})\'              { return T_const; }
\"([^\"\n]|\\.)*\"                                              { return T_string; }

[ \t]+ { ; } 												    /* Ignore spaces and tabs */
^[ \t]*(#.*)?\n { ; }                                           /* Single-line comments, blank lines*/
"(*" { BEGIN(COMMENT); comment_depth = 1; }                     /* Multi-line comment handling */
<COMMENT>"(*" { comment_depth++; }                              /* Handle Nested Comments */
<COMMENT>")*" { if (--comment_depth == 0) BEGIN(INITIAL);}      /* Exit comment state */
<COMMENT>.    {}                                                /* Consume characters inside comment */ 
<COMMENT>\n   {}                                                /* Keep track of new lines */ 
<COMMENT><<EOF>> {                                              /* ERROR: Unclosed Comment */
    fprintf(stderr, RED "Lexer Error " RESET ": Unclosed comment (missing '*)') at line %d\n", yylineno);
    exit(1);
}

^[ \t]*[^ \t\n]+ {
    char *yynew;
    int last = yyleng - 1;
    int dedents = process_indent(yytext);

    yynew = strdup(yytext);

    while ((last >= 0) && (yynew[last] != ' ' && yynew[last] != '\t')) {
        unput(yynew[last]);
        last--;
    }
    free(yynew);

    if (dedents > 0) {
        for (int i = 1; i < dedents; i++) {
            unput('}');
        }
        return auto_end;
    }
}

[\n\r] { 
    leader = 0;
}

"}" {
    return auto_end;
}

<<EOF>> { 
    if (level > 0) {
        level--;
        return auto_end;
    }
    return 0; 
}

. { printf( RED "lexer error: " RESET "Unrecognized character: " RED "%s " RESET  "at line" RED "%d\n" RESET, yytext, yylineno); }

%%

void stackinit() {
    indent_stack_size = 10;  // Initial capacity
    indent_stack = (unsigned int*)malloc(indent_stack_size * sizeof(unsigned int));
    if (!indent_stack) {
        fprintf(stderr, "Error: Memory allocation failed for indentation stack\n");
        exit(1);
    }
    indent_stack[0] = 0;  
    level = 0;
    current_indent = 0;
    leader = 0;
}
