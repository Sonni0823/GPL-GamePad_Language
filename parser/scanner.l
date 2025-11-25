%option noyywrap

%{
#include "ast.h"
#include "parser.tab.h" // Yacc/Bison에서 생성해주는 헤더
%}

%%

[aA]    { return TOKEN_A; }
[bB]    { return TOKEN_B; }
[cC]    { return TOKEN_C; }

[ \t\n\r]+ { /* 공백, 탭, 줄바꿈 무시 */ }

.          { /* 그 외의 문자는 무시 (주석 처럼 동작) */ }

%%