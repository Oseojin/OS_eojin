#include <stdint.h>

// 외부 함수 선언
// ports.c
extern unsigned char    inb(unsigned short port);
// screen.c
extern void             kprint(char* message);

// 스캔 코드 -> 아스키 변환 테이블
// 인덱스 = 스캔 코드, 값 = 아스키 문자
char    scancode_to_sacii[] = 
{
    0, // 인덱스 맞추기 용
    27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // 0~14
    '/t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', // 15~28
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', // 29~40
    '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, // 41~54
    0, 0, ' ', 0, // 55~58
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1~F10 (59~68)
    0, 0, // 69~70
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 71~83
    0, 0, 0, // 84~86 (얘네는 뭐임?)
    0, 0 // F11, F12 (87~88)
};

void    keyboard_handler()
{
    uint8_t scancode = inb(0x60); // 0x60 포트에서 스캔 코드 읽기

    // 키를 뗄 때(Break Code)는 최상위 비트가 1이 된다. ex) 0x81
    // 키를 누를 때(Make Code)만 처리 (최상위 비트가 0인 경우)
    if (scancode < 58)
    {
        char    ascii = scancode_to_sacii[scancode];
        if (ascii != 0)
        {
            char    str[2] = {ascii, 0}; // 문자열 변환
            kprint(str);
        }
    }
}