#include "../../includes/idt.h"

idt_gate_t      idt[IDT_ENTRIES];
idt_register_t  idt_reg;

void    set_idt_gate(int n, uint32_t handler)
{
    idt[n].low_offset = (uint16_t)((handler) & 0xffff);
    idt[n].sel = 0x08;  // GDT의 커널 코드 세그먼트 오프셋
    idt[n].always0 = 0;
    idt[n].flags = 0x8e; // 1(P) 00(DPL) 0 1110(32-bit Interrupt Gate)
    idt[n].high_offset = (uint16_t)(((handler) >> 16) & 0xffff);
}

void    set_idt()
{
    idt_reg.base = (uint32_t)&idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    // 어셈블리 명령어로 IDT 로드
    __asm__ volatile("lidt (%0)" : : "r" (&idt_reg));
}