#include "../../includes/idt.h"

volatile char*  video_memory = (volatile char*)0xb8000;

// 외부 함수 선언
extern void set_idt_gate(int n, uint32_t handler);
extern void set_idt();
extern void isr0();

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
}