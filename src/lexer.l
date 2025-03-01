%{
#define T_eof       0
#define T_as        1000
#define T_begin     1001
#define T_break     1002
#define T_byte      1003
#define T_continue  1004
#define T_decl      1005
#define T_def       1006
#define T_elif      1007
#define T_else      1008
#define T_end       1009
#define T_exit      1010
#define T_false     1011
#define T_if        1012
#define T_is        1013
#define T_int       1014
#define T_loop      1015
#define T_not       1016
#define T_or        1017
#define T_ref       1018
#define T_return    1019
#define T_skip      1020
#define T_true      1021
#define T_var       1022
#define T_number    1023
#define T_id        1024
#define T_UNKNOWN   1025
#define T_and       1026
#define T_eq        1027
#define T_neq       1028
#define T_lte       1029
#define T_gte       1030
#define T_char      1031
#define T_string    1032
#define T_asgn      1033
#define T_array     1034
%}

%option noyywrap

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

"(*"            { int nest_level = 1;  /* --- Multi-Line Comments (Supports Nesting) --- */
                  while (nest_level > 0) {
                      int c = yyinput();
                      if (c == EOF) break;
                      if (c == '*') {
                          int next = yyinput();
                          if (next == ')') nest_level--;  /* Decrease nesting */
                          else unput(next);  /* Put back char if not closing */
                      } 
                      else if (c == '(') {
                          int next = yyinput();
                          if (next == '*') nest_level++; /* Increase nesting */
                          else unput(next);
                      }
                  } 
                }

[\+\-\*\/\%\=\&\!\<\>\(\)\[\]\,\:]  { return yytext[0]; } /* Use ASCII values */

[a-zA-Z][a-zA-Z0-9_]*"["[0-9]+"]"  { return T_array; }

[0-9]+                  { return T_number; }   /* Digit */

[a-zA-Z][a-zA-Z0-9_]*  { return T_id; }        /* Identifier */

"\""(\\[ntr0\'\"\\]|\\x[0-9a-fA-F]{2}|[^\"\n\\])*"\""  { return T_string; } /* --- String Literals (Single-line only, with escape sequences) --- */

"'"([^'\\]|\\[ntr0\'\"\\]|\\x[0-9a-fA-F]{2})"'" { return T_char; } /* --- Character Constants (Supports all escape sequences) --- */

[ \t\n\r]+              { /* Ignore spaces, tabs, and newlines */ } /*Whitespace*/

.                       { fprintf(stderr, "error: illegal character\n"); exit(1); } /* --- Catch-All for Unknown Characters --- */

%%

int main() { //delete later
    int token;
    do {
        token = yylex();
        printf("token=%d, lexeme=\"%s\"\n", token, yytext);
    } while (token != T_eof);
}
