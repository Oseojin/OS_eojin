#include "../../includes/idt.h"

volatile char*  video_memory = (volatile char*)0xb8000;

// 외부 함수 선언
// idt.h
extern void     set_idt_gate(int n, uint32_t handler);
extern void     set_idt();
extern void     isr0();
// ports.c
extern void     outb(uint16_t port, uint8_t data);
extern uint8_t  inb(uint16_t port);
// pic.c
extern void     pic_remap();
// screen.c
extern void     kprint(char* message);
// interrupt.asm
extern void     irq0();
extern void     irq1();
// timer.c
extern void     init_timer(uint32_t freq);

void    main()
{
    kprint("Kernel loaded.\n");

    set_idt_gate(0, (uint32_t)isr0);    // ISR 0번(Divide by Zero) 등록
    set_idt_gate(32, (uint32_t)irq0);   // 32번 (IRQ 0) 등록, Timer
    set_idt_gate(33, (uint32_t)irq1);   // 33번 (IRQ 1) 등록, Keyboard

    // IDT 로드
    set_idt();

    __asm__ volatile("int $0");

    kprint("After Interrupt.\n");

    outb(0x20, 0x11);

    kprint("outb");

    pic_remap(); // PIC 초기화 및 리매핑

    kprint("init & remap\n");

    // 타이머 인터럽트(IRQ 0) 마스크 해제
    // 키보드 인터럽트(IRQ 1) 마스크 해제
    // 1111 1100 = 0xfc
    outb(0x21, 0xfc);

    // CPU 인터럽트 허용 (STI)
    __asm__ volatile("sti");

    kprint("Waiting for Keyboard...\n");

    // 타이머 인터럽트
    init_timer(50);

    while(1);
}