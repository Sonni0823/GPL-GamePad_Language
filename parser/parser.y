%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "codegen_x86.h"

// Lex에서 넘어오는 함수들
extern int yylex(void);
void yyerror(const char* s);

// main.c에 정의된 출력 파일 포인터
extern FILE* asm_out;

// A가 눌린 횟수를 세는 누적기
static long accumulator = 0;
%}

// 토큰 정의 (ABC만 사용)
%token TOKEN_A TOKEN_B TOKEN_C

%%

program
    : cmd_list TOKEN_C
      {
          // C를 만나면 지금까지 모은 숫자들(Subleq 코드)로 어셈블리 생성
          gen_x86_program(ast_get_root(), asm_out);
          ast_free_all();
      }
    ;

cmd_list
    : /* empty */
    | cmd_list cmd
    ;

cmd
    : TOKEN_A
      {
          accumulator++; // A: 값 1 증가
      }
    | TOKEN_B
      {
          // [핵심] 8-bit Signed Overflow 구현
          // A의 개수를 0~255 사이의 값으로 자름
          long val = accumulator % 256;
          
          // 128 이상(0x80 ~ 0xFF)은 음수(-128 ~ -1)로 해석
          // 예: A가 255번 -> 255 - 256 = -1 (출력 포트로 사용됨)
          if (val >= 128) {
              val = val - 256;
          }
          
          ast_add_value(val); // 리스트에 값 추가
          accumulator = 0;    // 누적기 초기화
      }
    ;

%%

void yyerror(const char* s) {
    fprintf(stderr, "Parser Error: %s\n", s);
}