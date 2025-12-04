#include "../../includes/idt.h"
#include "../../includes/utils.h"
#include "../../includes/memory.h"
#include "../../includes/pmm.h"

volatile char*  video_memory = (volatile char*)0xb8000;

// 외부 함수 선언
// idt.h
extern void     set_idt_gate(int n, uint64_t handler);
extern void     set_idt();
extern void     isr0();
// utils.h
extern int      strcmp(char s1[], char s2[]);
// memory.h
extern void     print_memory_map();
// pmm.h
extern void     init_pmm();
// ports.c
extern void     outb(uint16_t port, uint8_t data);
extern uint8_t  inb(uint16_t port);
// pic.c
extern void     pic_remap();
// screen.c
extern void     kprint_at(char* message, int col, int row);
extern void     kprint(char* message);
// interrupt.asm
extern void     irq0();
extern void     irq1();
// timer.c
extern void     init_timer(uint32_t freq);

void    user_input(char* input)
{
    if (strcmp(input, "help") == 0)
    {
        kprint("Available commands:\n");
        kprint("    help    - Show this list\n");
        kprint("    clear   - Clear the screen\n");
        kprint("    halt    - Halt the CPU\n");
        kprint("    memory  - Show Memory Map\n");
    }
    else if (strcmp(input, "clear") == 0)
    {
        clear_screen();
    }
    else if (strcmp(input, "halt") == 0)
    {
        kprint("Halting CPU. Bye!\n");
        __asm__ volatile("hlt");
    }
    else if (strcmp(input, "memory") == 0)
    {
        print_memory_map();
    }
    // alloc test
    else if (strcmp(input, "alloc") == 0)
    {
        void    *ptr = pmm_alloc_block();

        if (ptr == 0)
        {
            kprint("Allocation Failed (Out of Memory)\n");
        }
        else
        {
            char    buf[32];
            kprint("Allocated at: ");
            hex_to_ascii((uint64_t)ptr, buf);
            kprint(buf);
            kprint("\n");
        }
    }
    else
    {
        kprint("Unknown command: ");
        kprint(input);
        kprint("\n");
    }

    kprint("OS_eojin> ");
}

void    main()
{
    kprint_at("Kernel loaded.\n", 0, 4);

    set_idt_gate(0, (uint64_t)isr0);    // ISR 0번(Divide by Zero) 등록
    set_idt_gate(32, (uint64_t)irq0);   // 32번 (IRQ 0) 등록, Timer
    set_idt_gate(33, (uint64_t)irq1);   // 33번 (IRQ 1) 등록, Keyboard

    // IDT 로드
    set_idt();

    __asm__ volatile("int $0");

    kprint("After Interrupt.\n");

    outb(0x20, 0x11);

    kprint("outb\n");

    pic_remap(); // PIC 초기화 및 리매핑

    kprint("init & remap\n");

    // PMM 초기화
    init_pmm();

    // 타이머 인터럽트(IRQ 0) 마스크 해제
    // 키보드 인터럽트(IRQ 1) 마스크 해제
    // 1111 1100 = 0xfc
    outb(0x21, 0xfc);

    // CPU 인터럽트 허용 (STI)
    __asm__ volatile("sti");

    // 타이머 인터럽트
    init_timer(50);

    kprint("OS_eojin> ");

    while(1);
}