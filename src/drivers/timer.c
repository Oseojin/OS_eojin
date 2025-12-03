#include <stdint.h>

// 외부 함수 선언
// ports.c
extern void     outb(uint16_t port, uint8_t data);
// screen.c
extern void     kprint(char* message);

void    init_timer(uint32_t freq)
{
    // 포트 설정
    // 0x43: Command, 0x40: Channel 0 Data

    uint32_t    divisor = 1193180 / freq;
    uint8_t     low = (uint8_t)(divisor & 0xff);
    uint8_t     high = (uint8_t)((divisor >> 8) & 0xff);

    // 0x36: Channel 0, Lob/Hibyte, Mode 3(Square Wave), Binary
    outb(0x43, 0x36);
    outb(0x40, low);
    outb(0x40, high);

    kprint("Timer initialized.\n");
}

uint32_t    tick = 0;

void    timer_handler()
{
    tick++;
    if (tick % 100 == 0)
    {
        kprint(".");
    }
}