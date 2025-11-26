#ifndef KERNEL_H
#define KERNEL_H

// --- kmain.c에서 정의된 화면 출력 함수들 ---
#define VIDEO_MEMORY 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f

void print_char(char character, int col, int row, char attribute_byte);
void print_string(char* message, int row);

// --- drivers.c에서 정의된 하드웨어 제어 함수들 ---
void init_drivers();
unsigned char port_byte_in(unsigned short port);
void port_byte_out(unsigned short port, unsigned char data);

#endif