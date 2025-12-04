#include "../../includes/keyboard.h"

// 외부 함수 선언
// ports.c
extern uint8_t  inb(uint16_t port);
// screen.c
extern void     kprint(char *message);
extern void     kprint_backspace();
// kernel.c
extern void     user_input(char* input);

// 상태 변수
static int  is_shift = 0;
static char key_buffer[256];
static int  buffer_index = 0;

// 소문자 테이블
const char  ascii_low[] =
{
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, // 0-14
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, // 15-28
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',    // 29-41
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// 대문자(Shift) 테이블
const char  ascii_shift[] =
{
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void    keyboard_handler()
{
    uint8_t scancode = inb(0x60);

    // Shift Key 처리
    // Shift Down
    if (scancode == 0x2a || scancode == 0x36)
    {
        is_shift = 1;
        return;
    }
    // Shift Up
    else if (scancode == 0xaa || scancode == 0xb6)
    {
        is_shift = 0;
        return;
    }

    // Break Code (키 뗌) 무시 (Shift 제외)
    if (scancode & 0x80)
    {
        return;
    }

    // 스캔 코드 범위 확인
    if (scancode > SC_MAX)
    {
        return;
    }

    // 문자 변환
    char    letter = is_shift ? ascii_shift[scancode] : ascii_low[scancode];

    if (letter != 0)
    {
        // 화면 출력
        char    str[2] = {letter, 0};
        kprint(str);

        // 버퍼 저장
        key_buffer[buffer_index++] = letter;
        key_buffer[buffer_index] = 0;
    }
    // 백스페이스 처리 (스캔코드 0x0e)
    else if (scancode == 0x0e)
    {
        if (buffer_index > 0)
        {
            buffer_index--;
            key_buffer[buffer_index] = 0;
            kprint_backspace();
        }
    }
    // 엔터 처리 (스캔코드 0x1c)
    else if (scancode == 0x1c)
    {
        kprint("\n");

        user_input(key_buffer);

        // 버퍼 초기화
        buffer_index = 0;
        key_buffer[0] = 0;
    }
}