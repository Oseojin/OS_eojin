#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// IDT 엔트리 구조체 (8 byte)
typedef struct
{
    uint16_t    low_offset;     // 핸들러 주소 하위 16비트
    uint16_t    sel;            // 커널 코드 세그먼트 셀렉터
    uint8_t     ist;            // 인터럽트 스택 테이블 (0)
    uint8_t     flags;          // 타입 및 속성 (P, DPL, Type)
    uint16_t    mid_offset;     // 16-31 비트
    uint16_t    high_offset;    // 32-63 비트
    uint32_t    reserved;       // 0
} __attribute__((packed))   idt_gate_t;

// IDT 포인터 구조체 (lidt 명령어용)
typedef struct
{
    uint16_t    limit;
    uint64_t    base;
} __attribute__((packed))   idt_register_t;

// 총 256개 인터럽트 게이트
#define IDT_ENTRIES 256

extern idt_gate_t      idt[IDT_ENTRIES];
extern idt_register_t  idt_reg;

// 함수 선언
void    set_idt_gate(int n, uint64_t handler);
void    set_idt();

#endif