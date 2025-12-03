#include "../../includes/idt.h"

volatile char*  video_memory = (volatile char*)0xb8000;

// 외부 함수 선언
// idt.h
extern void set_idt_gate(int n, uint32_t handler);
extern void set_idt();
extern void isr0();
// ports.c
extern void     outb(uint16_t port, uint8_t data);
extern uint8_t  inb(uint16_t port);
// pic.c
extern void pic_remap();



void    print_string(const char* str, int offset)
{
    volatile char*  ptr = video_memory + offset * 2;
    int             i = 0;
    while (str[i] != 0)
    {
        *ptr = str[i];
        *(ptr + 1) = 0x0f;
        ptr += 2;
        i++;
    }
}

void    main()
{
    print_string("Kernel loaded.", 0);

    // ISR 0번(Divide by Zero) 등록
    set_idt_gate(0, (uint32_t)isr0);

    // IDT 로드
    set_idt();

    __asm__ volatile("int $0");

    print_string("After Interrupt.", 80);

    outb(0x20, 0x11);

    print_string("outb", 160);

    pic_remap(); // PIC 초기화 및 리매핑

    print_string("init & remap", 240);
}