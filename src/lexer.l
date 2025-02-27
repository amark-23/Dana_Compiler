%{
#include <stdio.h>
#include <stdlib.h>
%}

%option noyywrap


DIGIT   [0-9]
ID      [a-zA-Z_][a-zA-Z0-9_]*

%%

{DIGIT}+            { printf("NUMBER(%s)\n", yytext); }
{ID}                { printf("IDENTIFIER(%s)\n", yytext); }
[\t\n\r ]+          /* Skip whitespace */
.                   { printf("UNKNOWN(%s)\n", yytext); }

%%

