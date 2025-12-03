#include "../../includes/idt.h"

// 외부 함수 선언
// kernel.c
extern void print_string(const char* str, int offset);
// ports.c
extern void outb(uint16_t port, uint8_t data);
// keyboard.c
extern void keyboard_handler();

// 스택에 저장된 레지스터 상태
typedef struct
{
    uint32_t    ds;                                     // Data Segment Selector
    uint32_t    edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by push
    uint32_t    int_no, err_code;                       // Interrupt Number, Error Code
    uint32_t    eip, cs, eflags, useresp, ss;           // Pushed by CPU automatically
} register_t;

void    isr_handler(register_t r)
{
    // 키보드 입력
    if (r.int_no == 33)
    {
        keyboard_handler();
        // PIC에게 데이터를 받았다는 신호(EOI)를 보내야 다음 키 입력이 들어옴
        // Master PIC(0x20)에게 0x20(EOI) 전송
        outb(0x20, 0x20);
    }
    // 0으로 나누기
    else if (r.int_no == 0)
    {
        print_string("Divide by Zero!", 800);
    }
}
