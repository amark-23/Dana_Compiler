%{
#include <cstdio>
#include <cstdlib>
#include <stack>
#include <queue>
#include "parser.hpp"
#include "lexer.hpp"

static std::stack<int> indentStack;
static bool blockOpener = false;
static bool insideExplicitBlock = false;
static int currentIndent = 0;
static bool eofTriggered = false;

std::queue<int> pendingTokens;

void pushIndent(int indent);
void popIndent();
void emitAutoEnd();
%}

%option noyywrap
%option yylineno

%x CHECK_INDENT

%%

"def"    { blockOpener = true; return T_def; }
"if"     { blockOpener = true; return T_if; }
"elif"   { blockOpener = true; return T_elif; }
"else"   { blockOpener = true; return T_else; }
"loop"   { blockOpener = true; return T_loop; }

"begin"  { insideExplicitBlock = true; return T_begin; }
"end"    { insideExplicitBlock = false; return T_end; }

"as"      { return T_as; }
"break"   { return T_break; }
"byte"    { return T_byte; }
"continue" { return T_continue; }
"decl"    { return T_decl; }
"exit"    { return T_exit; }
"false"   { return T_false; }
"is"      { return T_is; }
"int"     { return T_int; }
"not"     { return T_not; }
"or"      { return T_or; }
"return"  { return T_return; }
"skip"    { return T_skip; }
"true"    { return T_true; }
"var"     { return T_var; }
"and"     { return T_and; }
"ref"      { return T_ref; }

"!="      { return T_neq; }
"<="      { return T_lte; }
">="      { return T_gte; }
"<>"      { return T_asgn; }

[\+\-\*\/\%\=\&\!\<\>\(\)\[\]\,\:\{\}\|]  { return yytext[0]; }

[0-9]+                   { return T_number; }
[a-zA-Z][a-zA-Z0-9_]*    { return T_id; }
"\""(\\.|[^\"\\])*"\""   { return T_string; }
\'(\\.|[^\'\\])\'        { return T_char; }

[ \t]+   { }
(\r\n|\r|\n)      { if (!insideExplicitBlock) { BEGIN(CHECK_INDENT); } }

<CHECK_INDENT>[ \t]*   {
  int indentCount = 0;
  for (int i = 0; i < yyleng; i++) {
    if (yytext[i] == '\t') indentCount += 4;
    else indentCount++;
  }

  currentIndent = indentCount;
  int topIndent = indentStack.empty() ? 0 : indentStack.top();

  if (blockOpener) {
    blockOpener = false;
    if (currentIndent > topIndent) {
      pushIndent(currentIndent);
    }
  } else {
    while (!indentStack.empty() && currentIndent < topIndent) {
      emitAutoEnd();
      popIndent();
      topIndent = indentStack.empty() ? 0 : indentStack.top();
    }
  }

  BEGIN(INITIAL);
}

<CHECK_INDENT>.  {
  unput(yytext[0]);
  while (!indentStack.empty() && indentStack.top() > 0) {
    emitAutoEnd();
    indentStack.pop();
  }
  BEGIN(INITIAL);
}

<<EOF>> {
  fprintf(stderr, "Lexer: EOF detected, emitting T_auto_end\n");
  while (!indentStack.empty()) {
    emitAutoEnd();
    popIndent();
  }
  return 0;  // Return EOF
}

. {
   fprintf(stderr, "Lexer Error: Unexpected character '%s' at line %d\n", yytext, yylineno);
   exit(1);
}

%%

void pushIndent(int indent) {
  indentStack.push(indent);
}

void popIndent() {
  if (!indentStack.empty()) {
    indentStack.pop();
  }
}


void emitAutoEnd() {
    fprintf(stderr, "Lexer: Emitting T_auto_end at line %d\n", yylineno);
    pendingTokens.push(T_auto_end);
}
