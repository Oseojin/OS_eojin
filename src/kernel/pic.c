#include <stdint.h>

// 외부 함수 선언
// ports.c
extern void     outb(uint16_t port, uint8_t data);
extern uint8_t  inb(uint16_t port);

// PIC 포트 정의
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// 초기화 명령 (ICW, Initialization Command Word)
#define ICW1_INIT       0x11
#define ICW4_8086       0x01

void    pic_remap()
{
    uint8_t a1, a2;

    // 현재 마스크 저장
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    // 초기화 시작 (ICW1)
    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);

    // 벡터 오프셋 설정 (ICW2)
    outb(PIC1_DATA, 0x20); // Master    -> 0x20 (32)
    outb(PIC2_DATA, 0x28); // Slave     -> 0x28 (40)

    // 마스터-슬레이브 연결 설정 (ICW3)
    outb(PIC1_DATA, 0x04); // Master: Slave가 2번 핀에 있음
    outb(PIC2_DATA, 0x02); // Slave: 2번 핀에 연결됨

    // 모드 설정 (ICW4)
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    // 마스크 복구
    // 0xff를 보내면 모든 인터럽트를 무시(Mask)한다.
    outb(PIC1_DATA, 0xff);
    outb(PIC2_DATA, 0xff);
}