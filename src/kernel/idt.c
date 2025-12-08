#include "../../includes/idt.h"

idt_gate_t      idt[IDT_ENTRIES];
idt_register_t  idt_reg;

void    set_idt_gate(int n, uint64_t handler, uint8_t flags)
{
    idt[n].low_offset = (uint16_t)handler;
    idt[n].sel = 0x08;
    idt[n].ist = 0;
    idt[n].flags = flags;
    idt[n].mid_offset = (uint16_t)(handler >> 16);
    idt[n].high_offset = (uint32_t)(handler >> 32);
    idt[n].reserved = 0;
}

void    set_idt()
{
    idt_reg.base = (uint64_t)&idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    // 어셈블리 명령어로 IDT 로드
    __asm__ volatile("lidt (%0)" : : "r" (&idt_reg));
}