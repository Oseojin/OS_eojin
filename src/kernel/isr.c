#include "../../includes/idt.h"
#include "../../includes/process.h"

// 외부 함수 선언
// ports.c
extern void outb(uint16_t port, uint8_t data);
// keyboard.c
extern void keyboard_handler();
// screen.c
extern void kprint(char* message);
extern void hex_to_ascii(uint64_t n, char* str); // Add extern
// timer.c
extern void timer_handler();

// 스택에 저장된 레지스터 상태
typedef struct
{
    uint64_t    ds;                                     // Data Segment Selector
    uint64_t    rax, rbx, rcx, rdx, rbp, rsi, rdi;
    uint64_t    r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t    int_no, err_code;                       // Interrupt Number, Error Code
    uint64_t    rip, cs, rflags, rsp, ss;               // Pushed by CPU automatically
} registers_t;

uint64_t isr_handler(registers_t* r)
{
    uint64_t next_rsp = (uint64_t)r;

    // 타이머 입력
    if (r->int_no == 32)
    {
        timer_handler();
        outb(0x20, 0x20); // EOI
        
        // 스케줄링
        next_rsp = schedule((uint64_t)r);
    }
    // 키보드 입력
    else if (r->int_no == 33)
    {
        keyboard_handler();
        // PIC에게 데이터를 받았다는 신호(EOI)를 보내야 다음 키 입력이 들어옴
        // Master PIC(0x20)에게 0x20(EOI) 전송
        outb(0x20, 0x20);
    }
    // 0으로 나누기
    else if (r->int_no == 0)
    {
        kprint("Divide by Zero!\n");
    }
    // 8: Double Fault
    else if (r->int_no == 8)
    {
        kprint("Double Fault! Halting.\n");
        __asm__ volatile("hlt");
    }
    // 13: General Protection Fault
    else if (r->int_no == 13)
    {
        kprint("GP Fault! Halting.\n");
        kprint("RIP: ");
        char buf[32];
        hex_to_ascii(r->rip, buf);
        kprint(buf);
        kprint("  Err: ");
        hex_to_ascii(r->err_code, buf);
        kprint(buf);
        kprint("\n");
        __asm__ volatile("hlt");
    }
    // 14: Page Fault
    else if (r->int_no == 14)
    {
        kprint("Page Fault! Halting.\n");
        __asm__ volatile("hlt");
    }
    // System Call (0x80)
    else if (r->int_no == 128)
    {
        // RAX: Syscall Number
        if (r->rax == 0) // Exit
        {
            // kprint("Syscall: Exit.\n");
            kill_current_process();
            next_rsp = schedule((uint64_t)r);
            /*
            kprint("Sch Ret: ");
            char buf[32];
            hex_to_ascii(next_rsp, buf);
            kprint(buf);
            kprint("\n");
            */
        }
        else if (r->rax == 1) // Print String
        {
            char* msg = (char*)r->rbx;
            kprint(msg);
        }
    }
    
    return next_rsp;
}
