#include "../../includes/utils.h"

// 포트에 바이트 단위로 통신
void        outb(uint16_t port, uint8_t data)
{
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

uint8_t     inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// 포트에 워드 단위로 통신
void        outw(uint16_t port, uint16_t data)
{
    __asm__ volatile("outw %0, %1": : "a"(data), "Nd"(port));
}

uint16_t    inw(uint16_t port)
{
    uint16_t    result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}