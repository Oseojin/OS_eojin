#include "../../includes/idt.h"

// video_memory와 print_string을 쓰기 위해 extern 선언 필요
extern void print_string(const char* str, int offset);

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
    print_string("Interrupt Received!", 40);
    // 인터럽트 번호에 따른 분기 처리
}
