#include "../../includes/idt.h"

// 외부 함수 선언
// ports.c
extern void outb(uint16_t port, uint8_t data);
// keyboard.c
extern void keyboard_handler();
// screen.c
extern void kprint(char* message);
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

void    isr_handler(registers_t r)
{
    // 타이머 입력
    if (r.int_no == 32)
    {
        timer_handler();
        outb(0x20, 0x20); // EOI
    }
    // 키보드 입력
    else if (r.int_no == 33)
    {
        keyboard_handler();
        // PIC에게 데이터를 받았다는 신호(EOI)를 보내야 다음 키 입력이 들어옴
        // Master PIC(0x20)에게 0x20(EOI) 전송
        outb(0x20, 0x20);
    }
    // 0으로 나누기
    else if (r.int_no == 0)
    {
        kprint("Divide by Zero!");
    }
}
