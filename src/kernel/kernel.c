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
extern void     irq1();

void    main()
{
    kprint("Kernel loaded.\n");

    set_idt_gate(0, (uint32_t)isr0);    // ISR 0번(Divide by Zero) 등록
    set_idt_gate(33, (uint32_t)irq1);   // 33번 (IRQ 1) 등록

    // IDT 로드
    set_idt();

    __asm__ volatile("int $0");

    kprint("After Interrupt.\n");

    outb(0x20, 0x11);

    kprint("outb");

    pic_remap(); // PIC 초기화 및 리매핑

    kprint("init & remap\n");

    // 키보드 인터럽트(IRQ 1) 마스크 해제
    // 0xfd = 1111 1101 (1번째 비트만 0으로 끄기 -> 허용)
    outb(0x21, 0xfd);

    // CPU 인터럽트 허용 (STI)
    __asm__ volatile("sti");

    kprint("Waiting for Keyboard...\n");

    while(1);
}