#include <stdio.h>
#include "ast.h"
#include "codegen_x86.h"

void gen_x86_program(AST* root, FILE* out) {
    // ---------------------------------------------------------
    // 1. 데이터 섹션 (.data)
    // ---------------------------------------------------------
    fprintf(out, "    .section .data\n");
    
    // 메모리 배열 선언
    fprintf(out, "memory:\n");
    AST* cur = root;
    while (cur) {
        fprintf(out, "    .quad %ld\n", cur->value);
        cur = cur->next;
    }
    // 메모리 여유 공간 (Zero Padding)
    fprintf(out, "    .zero 65536\n"); 

    // ---------------------------------------------------------
    // 2. 텍스트 섹션 (.text)
    // ---------------------------------------------------------
    fprintf(out, "    .section .text\n");
    fprintf(out, "    .globl main\n");
    fprintf(out, "main:\n");

    /* 레지스터 용도:
     * %r12 : PC (Program Counter)
     * %rax : Operand A (Source Index)
     * %rbx : Operand B (Destination Index)
     * %rcx : Operand C (Jump Index)
     * %r11 : Memory Base Address (Windows Relocation 문제 해결용)
     */

    // 프로로그
    fprintf(out, "    pushq %%rbp\n");
    fprintf(out, "    movq %%rsp, %%rbp\n");

    // 초기화: PC = 0
    fprintf(out, "    movq $0, %%r12\n");

    // --- 실행 루프 (.Lloop) ---
    fprintf(out, ".Lloop:\n");

    // 1. [Windows Fix] 메모리 베이스 주소를 %r11에 로드 (RIP-relative)
    // 이렇게 하면 64비트 주소 공간 어디에 있든 안전하게 접근 가능
    fprintf(out, "    leaq memory(%%rip), %%r11\n");
    
    // 2. 종료 검사: PC < 0 이면 종료
    fprintf(out, "    cmpq $0, %%r12\n");
    fprintf(out, "    jl .Lexit\n");

    // 3. 명령어 인출 (Fetch): A, B, C 읽기
    // %r11(메모리 시작) + %r12(인덱스)*8
    
    // A = memory[PC]
    fprintf(out, "    movq (%%r11, %%r12, 8), %%rax\n");
    // B = memory[PC + 1]
    fprintf(out, "    movq 8(%%r11, %%r12, 8), %%rbx\n");
    // C = memory[PC + 2]
    fprintf(out, "    movq 16(%%r11, %%r12, 8), %%rcx\n");

    // 4. I/O 처리: B가 -1이면 출력
    fprintf(out, "    cmpq $-1, %%rbx\n");
    fprintf(out, "    je .Lprint_char\n");

    // 5. Subleq 연산: Mem[B] = Mem[B] - Mem[A]
    
    // rdx = Mem[A] (값 읽기) -> base(%r11) + index(%rax)*8
    fprintf(out, "    movq (%%r11, %%rax, 8), %%rdx\n");
    // rsi = Mem[B] (값 읽기) -> base(%r11) + index(%rbx)*8
    fprintf(out, "    movq (%%r11, %%rbx, 8), %%rsi\n");
    
    // 뺄셈 수행: rsi = rsi - rdx
    fprintf(out, "    subq %%rdx, %%rsi\n");
    
    // 결과 저장: Mem[B] = rsi
    fprintf(out, "    movq %%rsi, (%%r11, %%rbx, 8)\n");

    // 6. 분기 (Branch): 결과 <= 0 이면 C로 점프
    fprintf(out, "    cmpq $0, %%rsi\n");
    fprintf(out, "    jle .Ljump\n");

    // 조건 불만족 시: PC += 3
    fprintf(out, "    addq $3, %%r12\n");
    fprintf(out, "    jmp .Lloop\n");

    // 점프 수행 (.Ljump)
    fprintf(out, ".Ljump:\n");
    fprintf(out, "    movq %%rcx, %%r12\n"); // PC = C
    fprintf(out, "    jmp .Lloop\n");

    // --- 문자 출력 서브루틴 ---
    fprintf(out, ".Lprint_char:\n");
    // 출력할 값(Mem[A])을 rdi(Windows는 rcx지만 printf 계열은 ABI가 다를 수 있어 rdi/rsi 둘다 안전하게 처리하거나 표준 호출 사용)
    // System V ABI(리눅스)는 rdi가 첫 번째 인자, MS x64 ABI는 rcx가 첫 번째 인자입니다.
    // 하지만 MinGW gcc는 보통 printf 호출 시 System V 스타일을 내부적으로 처리해주거나 rcx를 씁니다.
    // 여기서는 표준적인 rdi/rcx 둘 다 커버하기 위해 값을 rcx(윈도우용)에 넣습니다. (MinGW는 똑똑해서 rdi에 넣어도 되지만 정석대로 갑니다)
    
    // Mem[A] 값 로드
    fprintf(out, "    movq (%%r11, %%rax, 8), %%rcx\n"); // Windows 1st arg
    fprintf(out, "    movq %%rcx, %%rdi\n");             // Linux 1st arg (just in case)

    // [Windows Fix] Shadow Space 확보 (32 bytes) 및 @PLT 제거
    fprintf(out, "    subq $32, %%rsp\n"); 
    fprintf(out, "    call putchar\n");    // @PLT 제거됨
    fprintf(out, "    addq $32, %%rsp\n");
    
    // 출력 후 PC += 3
    fprintf(out, "    addq $3, %%r12\n");
    fprintf(out, "    jmp .Lloop\n");

    // --- 종료 처리 (.Lexit) ---
    fprintf(out, ".Lexit:\n");
    fprintf(out, "    movl $0, %%eax\n");
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
}