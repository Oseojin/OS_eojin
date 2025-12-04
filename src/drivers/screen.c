#include <stdint.h>

// 외부 함수 선언
// ports.c
extern void     outb(uint16_t port, uint8_t data);
extern uint8_t  inb(uint16_t port);

// VGA 메모리 주소
#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80

// 속성: 흰 글자 + 검은 배경
#define WHITE_ON_BLACK 0x0f

// VGA 제어 포트
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

// 커서 위치 얻기
int     get_cursor_offset()
{
    outb(REG_SCREEN_CTRL, 14);
    int offset = inb(REG_SCREEN_DATA) << 8;
    outb(REG_SCREEN_CTRL, 15);
    offset += inb(REG_SCREEN_DATA);
    return offset * 2; // 문자(2바이트) + 속성(2바이트)
}

// 커서 위치 설정
void    set_cursor_offset(int offset)
{
    offset /= 2;
    outb(REG_SCREEN_CTRL, 14);
    outb(REG_SCREEN_DATA, (unsigned char)(offset >> 8));
    outb(REG_SCREEN_CTRL, 15);
    outb(REG_SCREEN_DATA, (unsigned char)(offset & 0xff));
}

// 문자 하나 출력
void    print_char(char c, int col, int row, char attribute_byte)
{
    unsigned char*  vidmem = (unsigned char*) VIDEO_ADDRESS;

    if (!attribute_byte)
    {
        attribute_byte = WHITE_ON_BLACK;
    }

    int offset;
    if (col >= 0 && row >= 0)
    {
        offset = (row * MAX_COLS + col) * 2;
    }
    else
    {
        offset = get_cursor_offset();
    }

    if (c == '\n')
    {
        int rows;
        offset = get_cursor_offset();
        rows = offset / (2 * MAX_COLS);
        offset = (rows + 1) * (2 * MAX_COLS);
    }
    else
    {
        vidmem[offset] = c;
        vidmem[offset+1] = attribute_byte;
        offset += 2;
    }

    set_cursor_offset(offset);
}

void    kprint_at(char* message, int col, int row)
{
    int i = 0;
    if (col >0 && row >= 0)
    {
        set_cursor_offset((row * MAX_COLS + col) * 2);
    }
    while (message[i] != 0)
    {
        print_char(message[i++], col, row, WHITE_ON_BLACK);
        // col, row가 -1이면 현재 커서 위치 사용
        if (col >= 0)
        {
            col++; // 절대 위치면 col 증가
        }
    }
}

void    kprint_backspace()
{
    int offset = get_cursor_offset() - 2; // 이전 글자 위치
    if (offset < 0)
    {
        offset = 0;
    }

    // 커서를 뒤로 이동
    set_cursor_offset(offset);

    // 공백 출력
    // 현재 커서 위치에 공백을 쓰면서 자동으로 커서가 +2 되므로 다시 -2 해줘야함.

    unsigned char*  vidmem = (unsigned char*) VIDEO_ADDRESS;
    vidmem[offset] = ' ';
    vidmem[offset+1] = 0x0f;
}

void    kprint(char* message)
{
    kprint_at(message, -1, -1);
}

void    clear_screen()
{
    int             screen_size = MAX_COLS * MAX_ROWS;
    unsigned char*  vidmem = (unsigned char*)VIDEO_ADDRESS;

    for (int i = 0; i < screen_size; i++)
    {
        vidmem[i * 2] = ' ';
        vidmem[i * 2 + 1] = WHITE_ON_BLACK;
    }

    set_cursor_offset(0);
}