%{
#include "lexer.hpp"
#include "parser.hpp"
#include <stack>

#define T_eof       0

int comment_depth = 0; /* nested comment handling */
%}

%option noyywrap yylineno
%x COMMENT 

%%
"as"        { return T_as; }       /*Keyword Matching*/
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

"!="        { return T_neq; }   /* Not Equal */
"<="        { return T_lte; }   /* Less than or equal */
">="        { return T_gte; }   /* Greater than or equal */
":="        { return T_asgn;}   /* Assign*/

"#".*           { /* Ignore single-line comments */ }

"(*"          { BEGIN(COMMENT); comment_depth = 1; }          /* Multi - line comment handling*/
<COMMENT>"(*" { comment_depth++; }                            /* In COMMENT state: Handle Nested Comments */
<COMMENT>"*)" { if (--comment_depth == 0) BEGIN(INITIAL);}    /* Exit comment state */
<COMMENT>.    {}                                              /* Consume characters inside comment */ 
<COMMENT>\n   {}                                              /* Keep track of new lines */ 
<COMMENT><<EOF>> {  /* --- ERROR: Unclosed Comment --- */
                    fprintf(stderr, "Lexer Error: Unclosed comment (missing '*)') at line %d\n", yylineno);
                    exit(1);
                 }


[\+\-\*\/\%\=\&\!\<\>\(\)\[\]\,\:]  { return yytext[0]; } /* Use ASCII values */

[0-9]+                  { return T_number; }              /* Digit */

[a-zA-Z][a-zA-Z0-9_]*  { return T_id; }                   /* Identifier */

"\""(\\[ntr0\'\"\\]|\\x[0-9a-fA-F]{2}|[^\"\n\\])*"\""  { return T_string; } /* --- String Literals (Single-line only, with escape sequences) --- */

"'"([^'\\]|\\[ntr0\'\"\\]|\\x[0-9a-fA-F]{2})"'" { return T_char; }          /* --- Character Constants (Supports all escape sequences) --- */



[ \t\n\r]+              { /* Ignore spaces, tabs, and newlines */ }         /* Whitespace */

.                       { 
                          fprintf(stderr, "Lexer Error: Illegal character '%c' (ASCII: %d) at line %d\n", yytext[0], yytext[0], yylineno); 
                          exit(1); 
                        } /* --- Catch-All for Unknown Characters --- */

%%

