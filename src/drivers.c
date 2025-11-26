#include "kernel.h"

// ----------------------------------------------------------------------------
// 1. I/O Port Communication
// ----------------------------------------------------------------------------
unsigned char port_byte_in(unsigned short port) {
    unsigned char result;
    __asm__("inb %1, %0" : "=a" (result) : "d" (port));
    return result;
}

void port_byte_out(unsigned short port, unsigned char data) {
    __asm__("outb %0, %1" : : "a" (data), "d" (port));
}

// ----------------------------------------------------------------------------
// 2. IDT Setup
// ----------------------------------------------------------------------------
typedef struct {
    unsigned short low_offset;
    unsigned short selector;
    unsigned char always0;
    unsigned char flags;
    unsigned short high_offset;
} __attribute__((packed)) idt_gate_t;

typedef struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256
idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;

extern void keyboard_handler(); 

void set_idt_gate(int n, unsigned int handler) {
    idt[n].low_offset = (unsigned short)(handler & 0xFFFF);
    idt[n].selector = 0x08; 
    idt[n].always0 = 0;
    idt[n].flags = 0x8E; 
    idt[n].high_offset = (unsigned short)((handler >> 16) & 0xFFFF);
}

void load_idt() {
    idt_reg.base = (unsigned int) &idt;
    idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    __asm__ volatile("lidt (%0)" : : "r" (&idt_reg));
}

// ----------------------------------------------------------------------------
// 3. PIC Initialization
// ----------------------------------------------------------------------------
void init_pic() {
    port_byte_out(0x20, 0x11);
    port_byte_out(0xA0, 0x11);

    port_byte_out(0x21, 0x20); // Master -> 32
    port_byte_out(0xA1, 0x28); // Slave  -> 40

    port_byte_out(0x21, 0x04);
    port_byte_out(0xA1, 0x02);

    port_byte_out(0x21, 0x01);
    port_byte_out(0xA1, 0x01);

    // 타이머(IRQ 0)는 끄고, 키보드(IRQ 1)만 허용
    port_byte_out(0x21, 0xFD); 
    port_byte_out(0xA1, 0xFF);
}

// ----------------------------------------------------------------------------
// 4. Keyboard Logic (수정됨)
// ----------------------------------------------------------------------------
char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0, 
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handler_main() {
    // [DEBUG] 키보드 인터럽트가 발생할 때마다 화면 우측 상단에 '*' 표시
    // 이것이 보이면 인터럽트 연결은 성공한 것입니다.
    print_char('*', 79, 0, 0x4F); // 빨간 배경(4) 흰 글씨(F)

    // [FIX] 상태 체크(0x64 포트) 제거하고 무조건 데이터 읽기(0x60 포트)
    // 버퍼에 데이터가 있든 없든 일단 읽어서 버퍼를 비워줘야 다음 인터럽트가 옵니다.
    unsigned char scancode = port_byte_in(0x60);
    
    if (scancode < 58) { // Make Code (키 누름)
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            static int col = 0;
            static int row = 5;
            
            print_char(c, col++, row, 0x0F);
            
            if (col >= 80) { 
                col = 0; 
                row++; 
                if (row >= 25) row = 0;
            }
        }
    }

    // EOI 전송
    port_byte_out(0x20, 0x20); 
}

// ----------------------------------------------------------------------------
// 5. Driver Init
// ----------------------------------------------------------------------------
void init_drivers() {
    init_pic();
    set_idt_gate(33, (unsigned int)keyboard_handler);
    load_idt();
    __asm__("sti"); 
}