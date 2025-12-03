#include <stdint.h>

// 외부 함수 선언
// ports.c
extern unsigned char    inb(unsigned short port);
// kernel.c
extern void             print_string(const char* str, int offset);

// 스캔 코드 -> 아스키 변환 테이블
// 인덱스 = 스캔 코드, 값 = 아스키 문자
char    scancode_to_sacii[] = 
{
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', // 0~14
    '/t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\', // 15~28
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '\n', // 29~41
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, // 42~54
    '*', 0, ' ', 0 // 55~58
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
            print_string(str, 400 + 2);
        }
    }
}